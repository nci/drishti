#include "graphcut.h"

#include <algorithm>
#include <array>
#include <QCoreApplication>
#include <QEventLoop>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>

namespace
{
const double c_infiniteCapacity = 100000000.0;
// Integrated GPUs share system RAM, so reject a solve before it can force the
// desktop into system-wide paging. Users can reduce the existing 2D box.
const quint64 c_graphCutMemoryLimitBytes = 512ULL*1024ULL*1024ULL;

bool checkedMultiply(quint64 lhs, quint64 rhs, quint64 &result)
{
  if (lhs != 0 && rhs > std::numeric_limits<quint64>::max()/lhs)
    return false;
  result = lhs*rhs;
  return true;
}

bool checkedAdd(quint64 lhs, quint64 rhs, quint64 &result)
{
  if (rhs > std::numeric_limits<quint64>::max()-lhs)
    return false;
  result = lhs+rhs;
  return true;
}

bool graphDimensions(int w, int h,
		     int &nodeCount, int &edgeCount,
		     QString &errorMessage)
{
  nodeCount = 0;
  edgeCount = 0;
  if (w <= 0 || h <= 0)
    {
      errorMessage = QStringLiteral("Graph Cut requires a non-empty image region.");
      return false;
    }

  quint64 nodes = 0;
  if (!checkedMultiply(static_cast<quint64>(w),
		       static_cast<quint64>(h), nodes) ||
      nodes > static_cast<quint64>(std::numeric_limits<int>::max()))
    {
      errorMessage = QStringLiteral("The Graph Cut region is too large to index safely.");
      return false;
    }

  const quint64 wm1 = static_cast<quint64>(w-1);
  const quint64 hm1 = static_cast<quint64>(h-1);
  quint64 vertical = 0;
  quint64 horizontal = 0;
  quint64 diagonal = 0;
  quint64 edges = 0;
  quint64 edgeSubtotal = 0;
  if (!checkedMultiply(static_cast<quint64>(w), hm1, vertical) ||
      !checkedMultiply(wm1, static_cast<quint64>(h), horizontal) ||
      !checkedMultiply(wm1, hm1, diagonal) ||
      !checkedMultiply(diagonal, 2, diagonal) ||
      !checkedAdd(vertical, horizontal, edgeSubtotal) ||
      !checkedAdd(edgeSubtotal, diagonal, edges) ||
      edges > static_cast<quint64>(std::numeric_limits<int>::max()/2))
    {
      errorMessage = QStringLiteral("The Graph Cut edge count is too large to represent safely.");
      return false;
    }

  nodeCount = static_cast<int>(nodes);
  edgeCount = static_cast<int>(edges);
  return true;
}

QString memoryAmount(quint64 bytes)
{
  return QStringLiteral("%1 MiB")
    .arg(static_cast<double>(bytes)/(1024.0*1024.0), 0, 'f', 1);
}

QString memoryBudget(bool checked, quint64 bytes)
{
  return checked ? memoryAmount(bytes) : QStringLiteral("unavailable");
}

QString memoryAdmissionMessage(
  quint64 requiredBytes,
  const PaintAlgorithmMemoryAdmission& admission,
  bool fixedLimitExceeded)
{
  QString reason;
  if (fixedLimitExceeded)
    reason = QStringLiteral("It exceeds the fixed %1 MiB working-memory limit.")
      .arg(c_graphCutMemoryLimitBytes/(1024ULL*1024ULL));
  else
    {
      switch (admission.reason)
        {
        case PaintAlgorithmMemoryAdmissionReason::InsufficientPhysicalMemory:
          reason = QStringLiteral("There is not enough physical-memory headroom.");
          break;
        case PaintAlgorithmMemoryAdmissionReason::InsufficientCommit:
          reason = QStringLiteral("There is not enough Windows Commit headroom.");
          break;
        case PaintAlgorithmMemoryAdmissionReason::MemoryStatusUnavailable:
          reason = QStringLiteral("Current system-memory status is unavailable.");
          break;
        case PaintAlgorithmMemoryAdmissionReason::AddressSpaceLimit:
          reason = QStringLiteral("The request exceeds the process address space.");
          break;
        case PaintAlgorithmMemoryAdmissionReason::InvalidRequest:
        case PaintAlgorithmMemoryAdmissionReason::ArithmeticOverflow:
          reason = QStringLiteral("The working-memory request cannot be represented safely.");
          break;
        case PaintAlgorithmMemoryAdmissionReason::Approved:
          break;
        }
    }

  return QStringLiteral("Graph Cut memory admission was rejected. "
                        "Required: %1; physical budget: %2; Commit budget: %3. "
                        "%4 Reduce the 2D box and try again.")
    .arg(memoryAmount(requiredBytes))
    .arg(memoryBudget(admission.physicalMemoryChecked,
                      admission.availablePhysicalBudgetBytes))
    .arg(memoryBudget(admission.commitMemoryChecked,
                      admission.availableCommitBudgetBytes))
    .arg(reason);
}

bool graphCutCancelled(void *context)
{
  std::atomic_bool *cancelRequested =
    static_cast<std::atomic_bool*>(context);
  if (!cancelRequested)
    return false;
  QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
  return cancelRequested->load();
}
}

MaxFlowMinCut::MaxFlowMinCut() {}
MaxFlowMinCut::~MaxFlowMinCut() {}

quint64
MaxFlowMinCut::memoryLimitBytes()
{
  return c_graphCutMemoryLimitBytes;
}

bool
MaxFlowMinCut::estimateMemoryBytes(int w, int h,
			   quint64 &bytes,
			   QString &errorMessage)
{
  bytes = 0;
  errorMessage.clear();

  int nodeCount = 0;
  int edgeCount = 0;
  if (!graphDimensions(w, h, nodeCount, edgeCount, errorMessage))
    return false;

  std::size_t graphBytes = 0;
  if (!GraphType::estimate_memory(nodeCount, edgeCount, graphBytes))
    {
      errorMessage = QStringLiteral("The Graph Cut allocation size cannot be represented safely.");
      return false;
    }

  // Include sigma/gradient buffers and a conservative one orphan-list entry
  // per node. The graph's node and arc arrays are included by estimate_memory.
  quint64 perNodeWorkspace = sizeof(double) + sizeof(ushort) + 2*sizeof(void *);
  quint64 workspaceBytes = 0;
  if (!checkedMultiply(static_cast<quint64>(nodeCount),
		       perNodeWorkspace, workspaceBytes) ||
      !checkedAdd(static_cast<quint64>(graphBytes), workspaceBytes, bytes))
    {
      errorMessage = QStringLiteral("The Graph Cut working-memory size overflowed.");
      return false;
    }

  return true;
}

bool
MaxFlowMinCut::estimateInvocationMemoryBytes(int roiWidth, int roiHeight,
				     int fullWidth, int fullHeight,
				     quint64 &bytes,
				     QString &errorMessage)
{
  bytes = 0;
  errorMessage.clear();
  if (fullWidth <= 0 || fullHeight <= 0 ||
      roiWidth <= 0 || roiHeight <= 0 ||
      roiWidth > fullWidth || roiHeight > fullHeight)
    {
      errorMessage = QStringLiteral("The selected Graph Cut region is invalid.");
      return false;
    }

  quint64 graphBytes = 0;
  if (!estimateMemoryBytes(roiWidth, roiHeight, graphBytes, errorMessage))
    return false;

  quint64 roiPixels = 0;
  quint64 fullPixels = 0;
  quint64 roiStagingBytes = 0;
  quint64 finalTagBytes = 0;
  quint64 graphPhaseBytes = 0;
  quint64 finalPhaseBytes = 0;
  const quint64 roiBytesPerPixel = sizeof(uchar) + 2*sizeof(ushort);
  if (!checkedMultiply(static_cast<quint64>(roiWidth),
		       static_cast<quint64>(roiHeight), roiPixels) ||
      !checkedMultiply(static_cast<quint64>(fullWidth),
		       static_cast<quint64>(fullHeight), fullPixels) ||
      !checkedMultiply(roiPixels, roiBytesPerPixel, roiStagingBytes) ||
      !checkedMultiply(fullPixels, sizeof(ushort), finalTagBytes) ||
      !checkedAdd(graphBytes, roiStagingBytes, graphPhaseBytes) ||
      !checkedAdd(roiStagingBytes, finalTagBytes, finalPhaseBytes))
    {
      errorMessage = QStringLiteral("The Graph Cut working-memory size overflowed.");
      return false;
    }

  if (roiPixels > static_cast<quint64>(
                    std::numeric_limits<std::size_t>::max()) ||
      fullPixels > static_cast<quint64>(
                     std::numeric_limits<std::size_t>::max()))
    {
      errorMessage = QStringLiteral("The Graph Cut buffers are too large to address safely.");
      return false;
    }

  // Count only allocations created by this invocation. The resident slice,
  // raw volume and mask are excluded. The phases do not coexist, so use peak.
  bytes = std::max(graphPhaseBytes, finalPhaseBytes);
  return true;
}

bool
MaxFlowMinCut::admitMemoryBytes(
  quint64 requiredBytes,
  QString &errorMessage,
  PaintAlgorithmMemoryAdmission *admissionResult,
  PaintMemoryStatusProvider provider,
  void *providerContext)
{
  errorMessage.clear();
  PaintAlgorithmMemoryAdmission admission =
    evaluatePaintAlgorithmMemoryAdmission(
      1, 1, 1, 0, static_cast<std::uint64_t>(requiredBytes),
      provider, providerContext);
  if (admissionResult)
    *admissionResult = admission;

  const bool fixedLimitExceeded = requiredBytes > memoryLimitBytes();
  if (!fixedLimitExceeded && admission.approved)
    return true;

  errorMessage = memoryAdmissionMessage(requiredBytes, admission,
                                        fixedLimitExceeded);
  return false;
}

void
MaxFlowMinCut::compute_sigmas(const uchar *image, int w, int h,
			      double *sigmas,
			      int boxSize, float lambda)
{
  const std::size_t pixelCount = static_cast<std::size_t>(w)*static_cast<std::size_t>(h);
  std::vector<ushort> imgtmp(pixelCount, 0);

  for (int i=1; i<w-1; ++i)
    for (int j=1; j<h-1; ++j)
      {
	const int a = image[j*w+(i+1)];
	const int b = image[j*w+(i-1)];
	const int c = image[(j+1)*w+i];
	const int d = image[(j-1)*w+i];
	imgtmp[j*w+i] = static_cast<ushort>(std::abs(a-b)+std::abs(c-d));
      }

  for (int i=0; i<w; ++i)
    for (int j=0; j<h; ++j)
      {
	const int xmin = boxSize > i ? 0 : i-boxSize;
	const int xmax = boxSize > w-1-i ? w-1 : i+boxSize;
	const int ymin = boxSize > j ? 0 : j-boxSize;
	const int ymax = boxSize > h-1-j ? h-1 : j+boxSize;

	double sum = 0;
	for (int x=xmin; x<=xmax; ++x)
	  for (int y=ymin; y<=ymax; ++y)
	    sum += imgtmp[y*w+x];

	const double sampleCount =
	  static_cast<double>(xmax-xmin+1)*static_cast<double>(ymax-ymin+1);
	sigmas[j*w+i] = sum/(2.0*sampleCount);
	sigmas[j*w+i] *= lambda;
      }
}

void
MaxFlowMinCut::draw_edges_image_data(GraphType &G,
			     const uchar *image, int w, int h,
			     int x1, int y1, int x2, int y2,
			     const double *sigmas,
			     double dist)
{
  if (x2 >= 0 && x2 < w &&
      y2 >= 0 && y2 < h)
    {
      const double sigma = sigmas[y1*w+x1];
      const double difference = static_cast<double>(image[y1*w+x1])-
	                        static_cast<double>(image[y2*w+x2]);

      double energy = 1.0;
      if (std::fabs(sigma) > 0.0)
	energy = std::exp(-(difference*difference)/(2*sigma*sigma));

      const double weight = energy/dist;
      G.add_edge(y1*w+x1, y2*w+x2, weight, weight);
    }
}

bool
MaxFlowMinCut::run(int w, int h,
		   int boxSize, float lambda,
		   bool tagSimilar,
		   const uchar *image,
		   const ushort *mask,
		   int tag, ushort *tags,
		   int &tagged,
		   QString &errorMessage,
		   std::atomic_bool *cancelRequested)
{
  tagged = 0;
  errorMessage.clear();

  if (!image || !mask || !tags)
    {
      errorMessage = QStringLiteral("Graph Cut received an unavailable image or label buffer.");
      return false;
    }
  if (boxSize < 0 || !std::isfinite(static_cast<double>(lambda)))
    {
      errorMessage = QStringLiteral("Graph Cut parameters are invalid.");
      return false;
    }
  if (tag <= 0 || tag >= 65535)
    {
      errorMessage = QStringLiteral("Graph Cut requires a tag value between 1 and 65534.");
      return false;
    }

  int nodeCount = 0;
  int edgeCount = 0;
  if (!graphDimensions(w, h, nodeCount, edgeCount, errorMessage))
    return false;

  quint64 estimatedBytes = 0;
  if (!estimateMemoryBytes(w, h, estimatedBytes, errorMessage))
    return false;
  PaintAlgorithmMemoryAdmission memoryAdmission;
  if (!admitMemoryBytes(estimatedBytes, errorMessage, &memoryAdmission) ||
      !reservePaintAlgorithmMemory(memoryAdmission))
    {
      if (errorMessage.isEmpty())
        errorMessage = QStringLiteral("Graph Cut memory reservation failed because another operation consumed the available budget.");
      return false;
    }

  try
    {
      std::vector<double> sigmas(static_cast<std::size_t>(nodeCount));
      compute_sigmas(image, w, h, sigmas.data(), boxSize, lambda);

      GraphType graph(nodeCount, edgeCount);
      graph.add_node(nodeCount);

      for (int i=0; i<w; ++i)
	for (int j=0; j<h; ++j)
	  {
	    if (((i*h+j) & 1023) == 0 &&
	        graphCutCancelled(cancelRequested))
	      throw std::runtime_error("Graph Cut cancelled");
	    graph.add_tweights(j*w+i, 0, 0);

	    draw_edges_image_data(graph, image, w, h,
				  i, j, i, j+1, sigmas.data(), 1);
	    draw_edges_image_data(graph, image, w, h,
				  i, j, i+1, j, sigmas.data(), 1);
	    draw_edges_image_data(graph, image, w, h,
				  i, j, i+1, j+1, sigmas.data(), std::sqrt(2.0));
	    draw_edges_image_data(graph, image, w, h,
				  i, j, i+1, j-1, sigmas.data(), std::sqrt(2.0));
	  }

      // Release the largest temporary before maxflow creates its orphan list.
      std::vector<double>().swap(sigmas);

      for (int i=0; i<w; ++i)
	for (int j=0; j<h; ++j)
	  {
	    if (((i*h+j) & 1023) == 0 &&
	        graphCutCancelled(cancelRequested))
	      throw std::runtime_error("Graph Cut cancelled");
	    if (mask[j*w+i] == 65535)
	      graph.add_tweights(j*w+i, c_infiniteCapacity, 0);
	    else if (mask[j*w+i] == tag)
	      graph.add_tweights(j*w+i, 0, c_infiniteCapacity);
	  }

      for (int i=0; i<w; ++i)
	for (int j=0; j<h; ++j)
	  if (image[j*w+i] == 0)
	    {
	      if (((i*h+j) & 1023) == 0 &&
	          graphCutCancelled(cancelRequested))
	        throw std::runtime_error("Graph Cut cancelled");
	      graph.add_tweights(j*w+i, c_infiniteCapacity, 0);
	    }

      if (tagSimilar)
	{
	  std::array<float, 256> objectHistogram = {};
	  std::array<float, 256> backgroundHistogram = {};
	  for (int i=0; i<nodeCount; ++i)
	    {
	      const uchar value = image[i];
	      if (mask[i] == 65535) backgroundHistogram[value]++;
	      if (mask[i] == tag) objectHistogram[value]++;
	    }

	  float totalObject = 0;
	  float totalBackground = 0;
	  for (int i=0; i<256; ++i)
	    {
	      totalObject += objectHistogram[i];
	      totalBackground += backgroundHistogram[i];
	    }
	  if (totalObject > 0)
	    for (int i=0; i<256; ++i)
	      objectHistogram[i] /= totalObject;
	  if (totalBackground > 0)
	    for (int i=0; i<256; ++i)
	      backgroundHistogram[i] /= totalBackground;

	  for (int i=0; i<w; ++i)
	    for (int j=0; j<h; ++j)
	      if (image[j*w+i] != 0 &&
		  mask[j*w+i] != 65535 &&
		  mask[j*w+i] != tag)
		{
		  const uchar value = image[j*w+i];
		  float objectProbability = objectHistogram[value];
		  float backgroundProbability = backgroundHistogram[value];
		  if (objectProbability > 0)
		    objectProbability = -std::log(1.0-objectProbability);
		  else
		    objectProbability = static_cast<float>(c_infiniteCapacity);
		  if (backgroundProbability > 0)
		    backgroundProbability = -std::log(1.0-backgroundProbability);
		  else
		    backgroundProbability = static_cast<float>(c_infiniteCapacity);
		  graph.add_tweights(j*w+i, objectProbability, backgroundProbability);
		}
	}

      graph.maxflow(false, 0, graphCutCancelled, cancelRequested);

      std::fill(tags, tags+nodeCount, static_cast<ushort>(0));
      for (int i=0; i<w; ++i)
	for (int j=0; j<h; ++j)
	  if (graph.what_segment(j*w+i) == GraphType::SINK)
	    {
	      tags[j*w+i] = static_cast<ushort>(tag);
	      tagged++;
	    }
    }
  catch (const std::bad_alloc &)
    {
      errorMessage = QStringLiteral("Graph Cut could not allocate enough working memory. "
				    "Reduce the 2D box and try again.");
      tagged = 0;
      return false;
    }
  catch (const std::exception &error)
    {
      errorMessage = QStringLiteral("Graph Cut failed: %1")
	.arg(QString::fromLocal8Bit(error.what()));
      tagged = 0;
      return false;
    }
  catch (...)
    {
      errorMessage = QStringLiteral("Graph Cut failed because of an unexpected internal error.");
      tagged = 0;
      return false;
    }

  return true;
}
