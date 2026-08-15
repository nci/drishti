#include "raw2pvl.h"
#include "samplingcontract.h"
//#include "vdbvolume.h"

#include "global.h"
#include "importmemoryadmission.h"
#include "metaimagepathutils.h"
#include "staticfunctions.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <memory>
#include <new>
#include <limits>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>

#include <QtXml>
#include <QFile>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QTextCodec>
#include <QUuid>
#include <QStorageInfo>

#include <QtConcurrentMap>
#include <QTableWidget>
#include <QPushButton>
#include <QSet>

#include "savepvldialog.h"
#include "volumefilemanager.h"
#include "propertyeditor.h"
#include "meshtools.h"
#include "../../common/src/recoveryjournal.h"


using namespace std;


namespace
{
const std::uint64_t kImportStageSafetyBytes = 64ULL*1024ULL*1024ULL;
const int kMaximumFilterSpread = 48;
QString g_pvlHeaderWriteError;

void calculateGaussianWeights(int spread, float (&weights)[100])
{
  std::fill(weights, weights+100, 0.0f);
  if (spread <= 0)
    {
      weights[0] = 1.0f;
      weights[2] = 1.0f;
      return;
    }

  const double variance = static_cast<double>(spread)*spread;
  float sum = 0.0f;
  for (int offset=-spread; offset<=spread; ++offset)
    {
      const double distance = static_cast<double>(offset)*offset;
      const float weight = static_cast<float>(qExp(-distance/(2.0*variance)));
      weights[offset+spread] = weight;
      sum += weight;
    }
  weights[2*spread+2] = sum;
}

QString importMemoryAmount(std::uint64_t bytes)
{
  const double mib = static_cast<double>(bytes)/(1024.0*1024.0);
  if (mib < 1024.0)
    return QStringLiteral("%1 MiB").arg(mib, 0, 'f', 1);
  return QStringLiteral("%1 GiB").arg(mib/1024.0, 0, 'f', 2);
}

QString importAdmissionError(const QString& operation,
                             const ImportMemoryAdmission& admission)
{
  QString reason;
  switch (admission.reason)
    {
    case ImportMemoryAdmissionReason::InvalidRequest:
      reason = QStringLiteral("The buffer request is invalid.");
      break;
    case ImportMemoryAdmissionReason::ArithmeticOverflow:
      reason = QStringLiteral("The buffer size calculation overflowed.");
      break;
    case ImportMemoryAdmissionReason::AddressSpaceLimit:
      reason = QStringLiteral("The buffer exceeds this process address space.");
      break;
    case ImportMemoryAdmissionReason::MemoryStatusUnavailable:
      reason = QStringLiteral(
        "Current physical-memory or Windows Commit headroom could not be read.");
      break;
    case ImportMemoryAdmissionReason::InsufficientPhysicalMemory:
      reason = QStringLiteral(
        "There is not enough physical-memory headroom without paging.");
      break;
    case ImportMemoryAdmissionReason::InsufficientCommit:
      reason = QStringLiteral("There is not enough Windows Commit headroom.");
      break;
    case ImportMemoryAdmissionReason::Approved:
      reason = QStringLiteral("The allocation was approved.");
      break;
    }

  return QStringLiteral(
    "%1 was stopped before allocating memory. Required peak increment: %2; "
    "usable physical budget: %3; usable Commit budget: %4. %5")
    .arg(operation,
         importMemoryAmount(admission.requiredBytes),
         admission.physicalMemoryChecked ?
           importMemoryAmount(admission.availablePhysicalBudgetBytes) :
           QStringLiteral("unavailable"),
         admission.commitMemoryChecked ?
           importMemoryAmount(admission.availableCommitBudgetBytes) :
           QStringLiteral("unavailable"),
         reason);
}

class BatchPathJournal
{
public:
  BatchPathJournal() : m_committed(false), m_started(false) {}

  ~BatchPathJournal()
  {
    if (!m_committed)
      rollback();
  }

  bool addPath(const QString& path, QString& error)
  {
    const QString clean = QFileInfo(path).absoluteFilePath();
    if (!m_started)
      {
        if (!RecoveryJournal::beginDirect(QFileInfo(clean).absolutePath(),
                                          QStringLiteral("import-batch"),
                                          m_state, &error))
          return false;
        m_started = true;
      }
    return RecoveryJournal::addDirectTarget(m_state, clean, &error);
  }

  bool commit()
  {
    if (!m_started)
      {
        m_committed = true;
        return true;
      }
    QString error;
    if (!RecoveryJournal::commitDirect(m_state, &error))
      {
        qWarning() << "Cannot commit batch recovery journal" << error;
        return false;
      }
    m_committed = true;
    return true;
  }

  void rollback()
  {
    if (!m_started)
      return;
    QString error;
    if (!RecoveryJournal::rollbackDirect(m_state, &error) && !error.isEmpty())
      qWarning() << "Cannot roll back batch recovery journal" << error;
    m_started = false;
  }

private:
  RecoveryJournalState m_state;
  bool m_committed;
  bool m_started;
};

bool recoverBatchJournals(const QString& directory, QString& error)
{
  return RecoveryJournal::recoverDirectory(directory,
                                           QStringLiteral("import-batch"),
                                           &error);
}

bool removeStaleNumberedSlabs(const QString& baseFilename, int slabCount,
                              QString& error)
{
  const QFileInfo baseInfo(baseFilename);
  const QDir directory = baseInfo.absoluteDir();
  const QString prefix = baseInfo.fileName() + ".";
  const QStringList entries = directory.entryList(
    QStringList() << (baseInfo.fileName() + ".???"),
    QDir::Files | QDir::NoSymLinks);
  for (const QString& entry : entries)
    {
      if (!entry.startsWith(prefix))
        continue;
      bool ok = false;
      const int number = entry.mid(prefix.size()).toInt(&ok);
      if (!ok || number <= slabCount)
        continue;
      const QString path = directory.absoluteFilePath(entry);
      if (!QFile::remove(path))
        {
          error = QString("Cannot remove stale slab '%1'").arg(path);
          return false;
        }
    }
  return true;
}

QStringList numberedOutputPaths(const QString& baseFilename)
{
  const QFileInfo baseInfo(baseFilename);
  const QDir directory = baseInfo.absoluteDir();
  const QString prefix = baseInfo.fileName() + ".";
  QStringList paths;
  const QStringList entries = directory.entryList(
    QStringList() << (baseInfo.fileName() + ".???"),
    QDir::Files | QDir::NoSymLinks);
  for (const QString& entry : entries)
    {
      if (!entry.startsWith(prefix))
        continue;
      bool ok = false;
      entry.mid(prefix.size()).toInt(&ok);
      if (ok)
        paths << directory.absoluteFilePath(entry);
    }
  return paths;
}

bool checkedAddBytes(qint64 a, qint64 b, qint64& result)
{
  if (a < 0 || b < 0 || a > std::numeric_limits<qint64>::max() - b)
    return false;
  result = a + b;
  return true;
}

bool checkedMulBytes(qint64 a, qint64 b, qint64& result)
{
  if (a < 0 || b < 0 ||
      (b != 0 && a > std::numeric_limits<qint64>::max()/b))
    return false;
  result = a*b;
  return true;
}

void fillVoxelBuffer(uchar *buffer, std::size_t voxelCount,
                     int bytesPerVoxel, int value)
{
  if (!buffer || bytesPerVoxel <= 0)
    return;
  if (bytesPerVoxel == 1)
    {
      std::memset(buffer, static_cast<unsigned char>(value), voxelCount);
      return;
    }
  if (bytesPerVoxel == 2)
    {
      const ushort voxel = static_cast<ushort>(value);
      ushort *out = reinterpret_cast<ushort*>(buffer);
      for (std::size_t i = 0; i < voxelCount; ++i)
        out[i] = voxel;
      return;
    }
  std::memset(buffer, 0, voxelCount*static_cast<std::size_t>(bytesPerVoxel));
}

bool checkedPlaneLayout(int width, int height,
                        std::uint64_t bytesPerElement,
                        std::uint64_t& pixels,
                        std::uint64_t& bytes)
{
  pixels = 0;
  bytes = 0;
  if (width <= 0 || height <= 0 || bytesPerElement == 0 ||
      !checkedImportMultiply(static_cast<std::uint64_t>(width),
                             static_cast<std::uint64_t>(height), pixels) ||
      pixels > static_cast<std::uint64_t>(
                 std::numeric_limits<int>::max()) ||
      !checkedImportMultiply(pixels, bytesPerElement, bytes))
    return false;
  return bytes <= static_cast<std::uint64_t>(
                    std::numeric_limits<std::size_t>::max());
}

bool addImportBytes(std::uint64_t bytes, std::uint64_t& total)
{
  return checkedImportAdd(total, bytes, total);
}

bool admitImportBuffers(const QString& operation,
                        std::uint64_t allocationBytes,
                        QString& error,
                        std::shared_ptr<ProcessMemoryReservation>& reservation)
{
  std::uint64_t requiredBytes = allocationBytes;
  if (!checkedImportAdd(requiredBytes, kImportStageSafetyBytes,
                        requiredBytes))
    {
      error = operation + QStringLiteral(
        " was stopped because its peak-memory calculation overflowed.");
      return false;
    }

  const ImportMemoryAdmission admission =
    evaluateImportMemoryAdmission(requiredBytes);
  if (!admission.approved)
    {
      error = importAdmissionError(operation, admission);
      return false;
    }
  if (!reserveImportMemory(admission, reservation))
    {
      error = operation + QStringLiteral(
        " was stopped because another Import/Paint task acquired the "
        "remaining memory budget before allocation.");
      return false;
    }
  return true;
}

template <typename T>
bool allocateImportArray(std::uint64_t count,
                         std::unique_ptr<T[]>& storage)
{
  if (count == 0 ||
      count > static_cast<std::uint64_t>(
                std::numeric_limits<std::size_t>::max()/sizeof(T)))
    return false;
  storage.reset(new (std::nothrow) T[static_cast<std::size_t>(count)]);
  return storage.get() != 0;
}

struct ConversionBuffers
{
  std::uint64_t rawPixels;
  std::uint64_t outputPixels;
  std::uint64_t finalPixels;
  std::uint64_t rawBytes;
  std::uint64_t filterBytes;
  std::uint64_t pvlBytes;
  std::uint64_t finalBytes;
  int filterSliceCount;
  std::unique_ptr<double[]> filter;
  std::unique_ptr<uchar[]> pvl;
  std::unique_ptr<uchar[]> raw;
  std::unique_ptr<uchar[]> finalSlice;
  std::unique_ptr<uchar[]> filterWindow;
  std::unique_ptr<uchar*[]> filterSlices;
  std::shared_ptr<ProcessMemoryReservation> reservation;

  ConversionBuffers()
    : rawPixels(0), outputPixels(0), finalPixels(0), rawBytes(0),
      filterBytes(0), pvlBytes(0), finalBytes(0), filterSliceCount(0)
  {
  }
};

struct IsosurfaceBuffers
{
  std::uint64_t rawBytes;
  std::uint64_t valuePixels;
  std::unique_ptr<uchar[]> raw;
  std::unique_ptr<float[]> values;
  std::shared_ptr<ProcessMemoryReservation> reservation;

  IsosurfaceBuffers() : rawBytes(0), valuePixels(0) {}
};

bool prepareIsosurfaceBuffers(const QString& operation,
                              int rawWidth, int rawHeight,
                              int rawBytesPerVoxel,
                              int depth, int width, int height,
                              float resample,
                              IsosurfaceBuffers& buffers,
                              QString& error)
{
  std::uint64_t rawPixels = 0;
  std::uint64_t valueBytes = 0;
  std::uint64_t sourceVoxels = 0;
  std::uint64_t resampledVoxels = 0;
  std::uint64_t vdbWorkingBytes = 0;
  std::uint64_t allocationBytes = 0;

  const double safeResample = static_cast<double>(resample);
  const std::uint64_t rd = safeResample > 0.0 ?
    static_cast<std::uint64_t>(std::ceil(
      static_cast<double>(depth)/safeResample)) : 0;
  const std::uint64_t rw = safeResample > 0.0 ?
    static_cast<std::uint64_t>(std::ceil(
      static_cast<double>(width)/safeResample)) : 0;
  const std::uint64_t rh = safeResample > 0.0 ?
    static_cast<std::uint64_t>(std::ceil(
      static_cast<double>(height)/safeResample)) : 0;

  if (!checkedPlaneLayout(rawWidth, rawHeight, rawBytesPerVoxel,
                          rawPixels, buffers.rawBytes) ||
      !checkedPlaneLayout(width, height, sizeof(float),
                          buffers.valuePixels, valueBytes) ||
      depth <= 0 || rd == 0 || rw == 0 || rh == 0 ||
      !checkedImportMultiply(static_cast<std::uint64_t>(depth),
                             static_cast<std::uint64_t>(width),
                             sourceVoxels) ||
      !checkedImportMultiply(sourceVoxels,
                             static_cast<std::uint64_t>(height),
                             sourceVoxels) ||
      !checkedImportMultiply(rd, rw, resampledVoxels) ||
      !checkedImportMultiply(resampledVoxels, rh, resampledVoxels) ||
      !checkedImportAdd(sourceVoxels, resampledVoxels, vdbWorkingBytes) ||
      !checkedImportMultiply(vdbWorkingBytes, 96, vdbWorkingBytes) ||
      !addImportBytes(buffers.rawBytes, allocationBytes) ||
      !addImportBytes(valueBytes, allocationBytes) ||
      !addImportBytes(vdbWorkingBytes, allocationBytes))
    {
      error = operation + QStringLiteral(
        " was stopped because its worst-case VDB/mesh working-set "
        "calculation overflowed or exceeds the supported slice layout.");
      return false;
    }

  if (!admitImportBuffers(operation, allocationBytes, error,
                          buffers.reservation))
    return false;
  if (!allocateImportArray(buffers.rawBytes, buffers.raw) ||
      !allocateImportArray(buffers.valuePixels, buffers.values))
    {
      error = operation + QStringLiteral(
        " could not allocate its admitted slice buffers. The system memory "
        "state changed; no VDB processing was started.");
      return false;
    }
  return true;
}

bool prepareMeshColorBuffer(const QString& operation,
                            int vertexCount,
                            QVector<QVector3D>& colors,
                            QString& error,
                            std::shared_ptr<ProcessMemoryReservation>& reservation)
{
  if (vertexCount < 0)
    {
      error = operation + QStringLiteral(
        " was stopped because the mesh vertex count is invalid.");
      return false;
    }
  if (vertexCount == 0)
    return true;

  std::uint64_t colorBytes = 0;
  if (!checkedImportMultiply(static_cast<std::uint64_t>(vertexCount),
                             sizeof(QVector3D), colorBytes) ||
      !admitImportBuffers(operation, colorBytes, error, reservation))
    {
      if (error.isEmpty())
        error = operation + QStringLiteral(
          " was stopped because the mesh-color buffer size overflowed.");
      return false;
    }

  try
    {
      colors.resize(vertexCount);
    }
  catch (const std::bad_alloc&)
    {
      error = operation + QStringLiteral(
        " could not allocate its admitted mesh-color buffer.");
      return false;
    }
  return true;
}

bool prepareConversionBuffers(const QString& operation,
                              int rawWidth, int rawHeight, int rawBytesPerVoxel,
                              int outputWidth, int outputHeight,
                              int pvlBytesPerVoxel,
                              int finalWidth, int finalHeight,
                              int spread, bool saveRawFile,
                              ConversionBuffers& buffers,
                              QString& error)
{
  if (rawBytesPerVoxel <= 0 || pvlBytesPerVoxel <= 0 ||
      spread < 0 || spread > kMaximumFilterSpread ||
      !checkedPlaneLayout(rawWidth, rawHeight, rawBytesPerVoxel,
                          buffers.rawPixels, buffers.rawBytes) ||
      !checkedPlaneLayout(outputWidth, outputHeight, sizeof(double),
                          buffers.outputPixels, buffers.filterBytes))
    {
      error = operation + QStringLiteral(
        " was stopped because dimensions, voxel size, or filter radius are "
        "outside the supported 32-bit slice layout.");
      return false;
    }

  std::uint64_t unusedPixels = 0;
  std::uint64_t managerRawBytes = 0;
  if (!checkedPlaneLayout(outputWidth, outputHeight, pvlBytesPerVoxel,
                           unusedPixels, buffers.pvlBytes) ||
      !checkedPlaneLayout(outputWidth, outputHeight, rawBytesPerVoxel,
                          unusedPixels, managerRawBytes))
    {
      error = operation + QStringLiteral(
        " was stopped because the processed slice size is invalid.");
      return false;
    }

  if (finalWidth > 0 || finalHeight > 0)
    {
      if (!checkedPlaneLayout(finalWidth, finalHeight, pvlBytesPerVoxel,
                              buffers.finalPixels, buffers.finalBytes))
        {
          error = operation + QStringLiteral(
            " was stopped because the padded slice size is invalid.");
          return false;
        }
    }

  std::uint64_t allocationBytes = 0;
  const std::uint64_t managerPvlBytes =
    buffers.finalBytes > 0 ? buffers.finalBytes : buffers.pvlBytes;
  if (!addImportBytes(buffers.rawBytes, allocationBytes) ||
      !addImportBytes(buffers.filterBytes, allocationBytes) ||
      !addImportBytes(buffers.pvlBytes, allocationBytes) ||
      !addImportBytes(buffers.finalBytes, allocationBytes) ||
      !addImportBytes(managerPvlBytes, allocationBytes) ||
      (saveRawFile &&
       !addImportBytes(managerRawBytes, allocationBytes)))
    {
      error = operation + QStringLiteral(
        " was stopped because its buffer total overflowed.");
      return false;
    }

  std::uint64_t filterWindowBytes = 0;
  std::uint64_t filterPointerBytes = 0;
  if (spread > 0)
    {
      buffers.filterSliceCount = 2*spread+1;
      if (!checkedImportMultiply(
            buffers.rawBytes,
            static_cast<std::uint64_t>(buffers.filterSliceCount),
            filterWindowBytes) ||
          !checkedImportMultiply(
            static_cast<std::uint64_t>(buffers.filterSliceCount),
            sizeof(uchar*), filterPointerBytes) ||
          !addImportBytes(filterWindowBytes, allocationBytes) ||
          !addImportBytes(filterPointerBytes, allocationBytes))
        {
          error = operation + QStringLiteral(
            " was stopped because the filter-window size overflowed.");
          return false;
        }
    }

  if (!admitImportBuffers(operation, allocationBytes, error,
                          buffers.reservation))
    return false;

  if (!allocateImportArray(buffers.outputPixels, buffers.filter) ||
      !allocateImportArray(buffers.pvlBytes, buffers.pvl) ||
      !allocateImportArray(buffers.rawBytes, buffers.raw) ||
      (buffers.finalBytes > 0 &&
       !allocateImportArray(buffers.finalBytes, buffers.finalSlice)) ||
      (filterWindowBytes > 0 &&
       !allocateImportArray(filterWindowBytes, buffers.filterWindow)) ||
      (buffers.filterSliceCount > 0 &&
       !allocateImportArray(
         static_cast<std::uint64_t>(buffers.filterSliceCount),
         buffers.filterSlices)))
    {
      error = operation + QStringLiteral(
        " could not allocate its admitted buffers. The system memory state "
        "changed; no processing was started.");
      return false;
    }

  for (int i=0; i<buffers.filterSliceCount; ++i)
    buffers.filterSlices[i] = buffers.filterWindow.get() +
      static_cast<std::size_t>(i*buffers.rawBytes);
  return true;
}

bool validVolumeRange(int depth, int width, int height,
                      int dmin, int dmax,
                      int wmin, int wmax,
                      int hmin, int hmax)
{
  return depth > 0 && width > 0 && height > 0 &&
    dmin >= 0 && dmax >= dmin && dmax < depth &&
    wmin >= 0 && wmax >= wmin && wmax < width &&
    hmin >= 0 && hmax >= hmin && hmax < height;
}

bool currentVolumeLayoutMatches(VolumeData *volume,
                                int depth, int width, int height,
                                int voxelType)
{
  if (!volume)
    return false;
  int currentDepth = 0;
  int currentWidth = 0;
  int currentHeight = 0;
  volume->gridSize(currentDepth, currentWidth, currentHeight);
  return currentDepth == depth && currentWidth == width &&
    currentHeight == height && volume->voxelType() == voxelType;
}

bool readExportSlice(VolumeData *volume, int sliceIndex,
                     uchar *destination, QString& error)
{
  error.clear();
  if (!volume || !destination)
    {
      error = QStringLiteral("The volume or output slice buffer is null.");
      return false;
    }

  try
    {
      if (volume->getDepthSlice(sliceIndex, destination))
        return true;
      error = volume->lastError();
      if (error.isEmpty())
        error = QStringLiteral("The volume decoder rejected the slice.");
    }
  catch (const std::exception& exception)
    {
      error = QStringLiteral("The volume decoder raised an exception: %1")
        .arg(QString::fromLocal8Bit(exception.what()));
    }
  catch (...)
    {
      error = QStringLiteral("The volume decoder raised an unknown exception.");
    }
  return false;
}
}


#ifdef Q_OS_WIN
#include <float.h>
#define ISNAN(v) _isnan(v)
#else
#define ISNAN(v) isnan(v)
#endif

#define REMAPVOLUME(pixelCount)						\
  {									\
    for(std::uint64_t j=0; j<(pixelCount); j++)				\
      {									\
	float v = ptr[j];						\
	int idx;							\
	float frc;							\
	if (v <= rawMap[0] || ISNAN(v))					\
	  {								\
	    idx = 0;							\
	    frc = 0;							\
	  }								\
	else if (v >= rawMap[rawSize])					\
	  {								\
	    idx = rawSize-1;						\
	    frc = 1;							\
	  }								\
	else								\
	  {								\
	    for(uint m=0; m<rawSize; m++)				\
	      {								\
		if (v >= rawMap[m] &&					\
		    v <= rawMap[m+1])					\
		  {							\
		    idx = m;						\
		    frc = ((float)v-rawMap[m])/				\
		      (rawMap[m+1]-rawMap[m]);				\
		  }							\
	      }								\
	  }								\
									\
	int pv = pvlMap[idx] + frc*(pvlMap[idx+1]-pvlMap[idx]);		\
	pvl[j] = pv;							\
      }									\
  }


void
Raw2Pvl::applyMapping(uchar *raw, int voxelType,
		      QList<float> rawMap,
		      uchar *pvlslice, int pvlbpv,
		      QList<int> pvlMap,
		      int width, int height)
{
  std::uint64_t pixelCount = 0;
  std::uint64_t outputBytes = 0;
  if (!raw || !pvlslice ||
      rawMap.count() < 2 || rawMap.count() != pvlMap.count() ||
      (pvlbpv != 1 && pvlbpv != 2) ||
      !checkedPlaneLayout(width, height, pvlbpv,
                          pixelCount, outputBytes))
    return;

  int rawSize = rawMap.size()-1;

  if (rawMap.count() == pvlMap.count())
    {
      bool same = true;
      for(int i=0; i<rawMap.count(); i++)
	if (rawMap[i] != pvlMap[i])
	  same = false;

      const bool storageCompatible =
	(voxelType == _UChar && pvlbpv == 1) ||
	(voxelType == _UShort && pvlbpv == 2);
      if (same && storageCompatible)
	{
	  memcpy(pvlslice, raw, static_cast<std::size_t>(outputBytes));
	  return;
	}
    }


  if (pvlbpv == 1)
    {
      uchar *pvl = (uchar*)pvlslice;
      if (voxelType == _UChar)
	{
	  uchar *ptr = raw;
	  REMAPVOLUME(pixelCount);
	}
      else if (voxelType == _Char)
	{
	  char *ptr = (char*)raw;
	  REMAPVOLUME(pixelCount);
	}
      else if (voxelType == _UShort)
	{
	  ushort *ptr = (ushort*)raw;
	  REMAPVOLUME(pixelCount);
	}
      else if (voxelType == _Short)
	{
	  short *ptr = (short*)raw;
	  REMAPVOLUME(pixelCount);
	}
      else if (voxelType == _Int)
	{
	  int *ptr = (int*)raw;
	  REMAPVOLUME(pixelCount);
	}
      else if (voxelType == _Float)
	{
	  float *ptr = (float*)raw;
	  REMAPVOLUME(pixelCount);
	}
    }
  else
    {
      ushort *pvl = (ushort*)pvlslice;
      if (voxelType == _UChar)
	{
	  uchar *ptr = raw;
	  REMAPVOLUME(pixelCount);
	}
      else if (voxelType == _Char)
	{
	  char *ptr = (char*)raw;
	  REMAPVOLUME(pixelCount);
	}
      else if (voxelType == _UShort)
	{
	  ushort *ptr = (ushort*)raw;
	  REMAPVOLUME(pixelCount);
	}
      else if (voxelType == _Short)
	{
	  short *ptr = (short*)raw;
	  REMAPVOLUME(pixelCount);
	}
      else if (voxelType == _Int)
	{
	  int *ptr = (int*)raw;
	  REMAPVOLUME(pixelCount);
	}
      else if (voxelType == _Float)
	{
	  float *ptr = (float*)raw;
	  REMAPVOLUME(pixelCount);
	}
    }
}

//-----------------------------
QString
getPvlNcFilename()
{
  QFileDialog fdialog(0,
		      "Save processed volume",
		      Global::previousDirectory(),
		      "Drishti (*.pvl.nc) ;; MetaImage (*.mhd) ;; VDB (*.vdb)");
  fdialog.setAcceptMode(QFileDialog::AcceptSave);

  if (!fdialog.exec() == QFileDialog::Accepted)
    return "";

  QString pvlFilename = fdialog.selectedFiles().value(0);
  if (fdialog.selectedNameFilter() == "VDB (*.vdb)")
    {
      if (!pvlFilename.endsWith(".vdb"))
	pvlFilename += ".vdb";
      return pvlFilename;
    }
  if (fdialog.selectedNameFilter() == "MetaImage (*.mhd)")
    {
      if (!pvlFilename.endsWith(".mhd"))
	pvlFilename += ".mhd";
      return pvlFilename;
    }
  if (fdialog.selectedNameFilter() == "Drishti (*.pvl.nc)")
    {
      if (pvlFilename.endsWith(".pvl.nc.pvl.nc"))
	  pvlFilename.chop(7);
      if (!pvlFilename.endsWith(".pvl.nc"))
	pvlFilename += ".pvl.nc";

      return pvlFilename;
    }

  return "";
}

bool
checkParIsoGen()
{
  // VolumeData and its decoder plugins are stateful and not thread-safe.
  // Sharing one instance across concurrent isosurface jobs can mix slices and
  // multiplies the admitted VDB working set by the worker count.
  return false;
}


bool
saveSliceZeroAtTop(bool *accepted = 0)
{
  bool save0attop = true;
  bool ok = false;
  QStringList slevels;
  slevels << "Yes - (default)";  
  slevels << "No - save slice 0 as bottom slice";
  QString option = QInputDialog::getItem(0,
		   "Save Data",
		   "Save slice 0 as top slice ?",
		    slevels,
			  0,
		      false,
		       &ok);
  if (accepted)
    *accepted = ok;
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "No")
	{
	  save0attop = false;
	  QMessageBox::information(0, "Save Data", "First slice is now bottom slice.");
	}
    }

  return save0attop;
}

bool
getSaveRawFile(bool *accepted = 0)
{
  bool saveRawFile = false;
  bool ok = false;
  QStringList slevels;
  slevels << "Yes - save raw file";
  slevels << "No";  
  QString option = QInputDialog::getItem(0,
		   "Save Processed Volume",
		   "Save RAW file along with preprocessed volume ?",
		    slevels,
			  1,
		      false,
		       &ok);
  if (accepted)
    *accepted = ok;
  if (ok)
    {
      QStringList op = option.split(' ');
      if (op[0] == "Yes")
	saveRawFile = true;
    }
  else if (!accepted)
    QMessageBox::information(0, "RAW Volume", "Will not save raw volume");

  return saveRawFile;
}

QString
getRawFilename(QString pvlFilename)
{
  QString rawfile = QFileDialog::getSaveFileName(0,
						 "Save processed volume",
						 QFileInfo(pvlFilename).absolutePath(),
						 "RAW Files (*.raw)");
//						 0,
//						 QFileDialog::DontUseNativeDialog);
  return rawfile;
}

int
getZSubsampling(int dsz, int wsz, int hsz, bool *accepted = 0)
{
  bool ok = false;
  QStringList slevels;

  slevels.clear();
  slevels << "No subsampling in Z";
  for (int factor=2; factor<=qMin(6, dsz); ++factor)
    slevels << QString("%1 [Z(%2) %3 %4]")
      .arg(factor).arg(dsz/factor).arg(wsz).arg(hsz);
  QString option = QInputDialog::getItem(0,
					 "Volume Size",
					 "Z subsampling",
					 slevels,
					 0,
					 false,
					 &ok);
  if (accepted)
    *accepted = ok;
  int svslz = 1;
  if (ok)
    {   
      QStringList op = option.split(' ');
      svslz = qMax(1, op[0].toInt());
    }
  return svslz;
}

int
getXYSubsampling(int svslz, int dsz, int wsz, int hsz,
		 bool *accepted = 0)
{
  bool ok = false;
  QStringList slevels;

  slevels.clear();
  slevels << "No subsampling in XY";
  const int maxFactor = qMin(6, qMin(wsz, hsz));
  for (int factor=2; factor<=maxFactor; ++factor)
    slevels << QString("%1 [%2 Y(%3) X(%4)]")
      .arg(factor).arg(dsz/qMax(1, svslz))
      .arg(wsz/factor).arg(hsz/factor);
  QString option = QInputDialog::getItem(0,
					 "Volume Size",
					 "XY subsampling",
					 slevels,
					 0,
					 false,
					 &ok);
  if (accepted)
    *accepted = ok;
  int svsl = 1;
  if (ok)
    {   
      QStringList op = option.split(' ');
      svsl = qMax(1, op[0].toInt());
    }
  return svsl;
}


#define AVERAGEFILTER(n)			\
  {						\
    for(int j=0; j<width; j++)			\
      for(int k=0; k<height; k++)		\
	{					\
	  float sum = 0;			\
	  for(int i=0; i<2*n+1; i++)		\
	    sum += weights[i]*pv[i][j*height+k]; \
	  p[j*height + k] = sum/wsum;		\
	}					\
  }

#define DILATEFILTER(n)					\
  {							\
    for(int j=0; j<width; j++)				\
      for(int k=0; k<height; k++)			\
	{						\
	  float avg = 0;				\
	  for(int i=0; i<2*n+1; i++)			\
	    avg = qMax(avg,(float)pv[i][j*height+k]);	\
	  p[j*height + k] = avg;			\
	}						\
  }

void
Raw2Pvl::applyMeanFilter(uchar **val, uchar *vg,
			 int voxelType,
			 int width, int height,
			 int spread, bool dilateFilter,
			 float *weights)
{
  float wsum = weights[2*spread+2];
 
  if (voxelType == _UChar)
    {
      uchar **pv = val;
      uchar *p  = vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Char)
    {
      char **pv = (char**)val;
      char *p  = (char*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _UShort)
    {
      ushort **pv = (ushort**)val;
      ushort *p  = (ushort*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Short)
    {
      short **pv = (short**)val;
      short *p  = (short*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Int)
    {
      int **pv = (int**)val;
      int *p  = (int*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
  else if (voxelType == _Float)
    {
      float **pv = (float**)val;
      float *p  = (float*)vg;
      if (dilateFilter)
	DILATEFILTER(spread)
      else 
        AVERAGEFILTER(spread)
    }
}


#define SLICEAVERAGEFILTER(n)					\
  {								\
    for(int i=0; i<height; i++)					\
      for(int j=0; j<width; j++)				\
	{							\
	  float pj = 0;						\
	  int jdx = 0;						\
	  for(int j1=j-n; j1<=j+n; j1++)			\
	    {							\
	      int idx = qBound(0, j1, width-1)*height+i;	\
	      pj += weights[jdx]*p[idx];			\
	      jdx ++;						\
	    }							\
	  pv[j*height+i] = pj/wsum;				\
	}							\
    								\
    for(int j=0; j<width; j++)					\
      for(int i=0; i<height; i++)				\
	{							\
	  float pj = 0;						\
	  int jdx = 0;						\
	  for(int i1=i-n; i1<=i+n; i1++)			\
	    {							\
	      int idx = j*height + qBound(0, i1, height-1);	\
	      pj += weights[jdx]*pv[idx];			\
	      jdx ++;						\
	    }							\
	  p[j*height+i] = pj/wsum;				\
	}							\
    								\
  }


#define SLICEDILATEFILTER(n)					\
  {								\
    for(int i=0; i<height; i++)					\
      for(int j=0; j<width; j++)				\
	{							\
	  float pj = 0;						\
	  int jst = qMax(0, j-n);				\
	  int jed = qMin(width-1, j+n);				\
	  for(int j1=jst; j1<=jed; j1++)			\
	    {							\
	      int idx = qBound(0, j1, width-1)*height+i;	\
	      pj = qMax((float)p[idx],pj);			\
	    }							\
	  pv[j*height+i] = pj;					\
	}							\
    								\
    for(int j=0; j<width; j++)					\
      for(int i=0; i<height; i++)				\
	{							\
	  float pi = 0;						\
	  int ist = qMax(0, i-n);				\
	  int ied = qMin(height-1, i+n);			\
	  for(int i1=ist; i1<=ied; i1++)			\
	    {							\
	      int idx = j*height+qBound(0, i1, height-1);	\
	      pi = qMax((float)pv[idx],pi);			\
	    }							\
	  p[j*height+i] = pi;					\
	}							\
  }

void
Raw2Pvl::applyMeanFilterToSlice(uchar *val, uchar *vg,
				int voxelType,
				int width, int height,
				int spread,
				bool dilateFilter,
				float *weights)
{
  float wsum = weights[2*spread+2];
 
  if (voxelType == _UChar)
    {
      uchar *p = val;
      uchar *pv  = vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Char)
    {
      char *p = (char*)val;
      char *pv = (char*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _UShort)
    {
      ushort *p = (ushort*)val;
      ushort *pv = (ushort*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Short)
    {
      short *p = (short*)val;
      short *pv = (short*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Int)
    {
      int *p = (int*)val;
      int *pv = (int*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
  else if (voxelType == _Float)
    {
      float *p = (float*)val;
      float *pv = (float*)vg;
      if (dilateFilter)
	SLICEDILATEFILTER(spread)
      else
	SLICEAVERAGEFILTER(spread)
    }
}

bool
Raw2Pvl::savePvlHeader(QString pvlFilename,
		       bool saveRawFile, QString rawfile,
		       int voxelType, int pvlVoxelType, int voxelUnit,
		       int d, int w, int h,
		       float vx, float vy, float vz,
		       QList<float> rawMap, QList<int> pvlMap,
		       QString description,
		       int slabSize,
		       QStringList sourceOrder)
{
  QString xmlfile = pvlFilename;
  g_pvlHeaderWriteError.clear();

  QDomDocument doc("Drishti_Header");

  QDomElement topElement = doc.createElement("PvlDotNcFileHeader");
  doc.appendChild(topElement);

  {      
    QString vstr;
    if (saveRawFile)
      {
	// save relative path for the rawfile
	QFileInfo fileInfo(pvlFilename);
	QDir direc = fileInfo.absoluteDir();
	vstr = direc.relativeFilePath(rawfile);
      }
    else
      vstr = "";

    QDomElement de0 = doc.createElement("rawfile");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
      
  {      
    QString vstr;
    if (voxelType == Raw2Pvl::_UChar)      vstr = "unsigned char";
    else if (voxelType == Raw2Pvl::_Char)  vstr = "char";
    else if (voxelType == Raw2Pvl::_UShort)vstr = "unsigned short";
    else if (voxelType == Raw2Pvl::_Short) vstr = "short";
    else if (voxelType == Raw2Pvl::_Int)   vstr = "int";
    else if (voxelType == Raw2Pvl::_Float) vstr = "float";
    
    QDomElement de0 = doc.createElement("voxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }


  {      
    QString vstr;
    if (pvlVoxelType == Raw2Pvl::_UChar)      vstr = "unsigned char";
    else if (pvlVoxelType == Raw2Pvl::_Char)  vstr = "char";
    else if (pvlVoxelType == Raw2Pvl::_UShort)vstr = "unsigned short";
    else if (pvlVoxelType == Raw2Pvl::_Short) vstr = "short";
    else if (pvlVoxelType == Raw2Pvl::_Int)   vstr = "int";
    else if (pvlVoxelType == Raw2Pvl::_Float) vstr = "float";
    
    QDomElement de0 = doc.createElement("pvlvoxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }


  {      
    QDomElement de0 = doc.createElement("gridsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1 %2 %3").arg(d).arg(w).arg(h));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QString vstr;
    if (voxelUnit == Raw2Pvl::_Nounit)         vstr = "no units";
    else if (voxelUnit == Raw2Pvl::_Angstrom)  vstr = "angstrom";
    else if (voxelUnit == Raw2Pvl::_Nanometer) vstr = "nanometer";
    else if (voxelUnit == Raw2Pvl::_Micron)    vstr = "micron";
    else if (voxelUnit == Raw2Pvl::_Millimeter)vstr = "millimeter";
    else if (voxelUnit == Raw2Pvl::_Centimeter)vstr = "centimeter";
    else if (voxelUnit == Raw2Pvl::_Meter)     vstr = "meter";
    else if (voxelUnit == Raw2Pvl::_Kilometer) vstr = "kilometer";
    else if (voxelUnit == Raw2Pvl::_Parsec)    vstr = "parsec";
    else if (voxelUnit == Raw2Pvl::_Kiloparsec)vstr = "kiloparsec";
    
    QDomElement de0 = doc.createElement("voxelunit");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QDomElement de0 = doc.createElement("voxelsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1 %2 %3").arg(vx).arg(vy).arg(vz));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  {
    QString vstr = description.trimmed();
    QDomElement de0 = doc.createElement("description");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {
    QDomElement names = doc.createElement("pvlnames");
    const int slabCount = slabSize > 0 ? 1 + (d-1)/slabSize : 0;
    QFileInfo headerInfo(pvlFilename);
    for (int i = 0; i < slabCount; ++i)
      {
        QDomElement name = doc.createElement("name");
        name.appendChild(doc.createTextNode(
          headerInfo.absoluteDir().relativeFilePath(
            QString("%1.%2").arg(headerInfo.fileName()).arg(i+1, 3, 10, QChar('0')))));
        names.appendChild(name);
      }
    topElement.appendChild(names);
  }

  {
    QDomElement de0 = doc.createElement("pvlheadersize");
    de0.appendChild(doc.createTextNode("13"));
    topElement.appendChild(de0);
  }

  // Keep the confirmed import order in structured XML.  Unlike the legacy
  // whitespace-separated slab list, each path remains unambiguous when it
  // contains spaces or non-ASCII characters.
  if (!sourceOrder.isEmpty())
    {
      QDomElement order = doc.createElement("sourceorder");
      for (const QString& source : sourceOrder)
        {
          if (source.trimmed().isEmpty())
            continue;
          QDomElement file = doc.createElement("file");
          file.appendChild(doc.createTextNode(source));
          order.appendChild(file);
        }
      if (!order.firstChildElement().isNull())
        topElement.appendChild(order);
    }
  {
    QDomElement de0 = doc.createElement("rawheadersize");
    de0.appendChild(doc.createTextNode("13"));
    topElement.appendChild(de0);
  }

  {
    QDomElement de0 = doc.createElement("slabsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(slabSize));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  {      
    QString vstr;
    for(int i=0; i<rawMap.size(); i++)
      vstr += QString("%1 ").arg(rawMap[i]);
    
    QDomElement de0 = doc.createElement("rawmap");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QString vstr;
    for(int i=0; i<pvlMap.size(); i++)
      vstr += QString("%1 ").arg(pvlMap[i]);
    
    QDomElement de0 = doc.createElement("pvlmap");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  QSaveFile f(xmlfile);
  if (!f.open(QIODevice::WriteOnly))
    {
      g_pvlHeaderWriteError =
        QString("Cannot open PVL header '%1': %2")
        .arg(xmlfile).arg(f.errorString());
      return false;
    }

  QTextStream out(&f);
  doc.save(out, 2);
  out.flush();
  if (out.status() != QTextStream::Ok)
    {
      g_pvlHeaderWriteError =
        QString("Cannot write PVL header '%1': %2")
        .arg(xmlfile).arg(f.errorString());
      f.cancelWriting();
      return false;
    }
  if (!f.commit())
    {
      g_pvlHeaderWriteError =
        QString("Cannot commit PVL header '%1': %2")
        .arg(xmlfile).arg(f.errorString());
      return false;
    }
  return true;
}

bool
Raw2Pvl::savePvl(VolumeData* volData,
		 int dmin, int dmax,
		 int wmin, int wmax,
		 int hmin, int hmax,
		 QStringList timeseriesFiles)
{
  if (!volData)
    {
      QMessageBox::warning(0, "Save", "No volume data is loaded.");
      return false;
    }
  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }

  //------------------------------------------------------
  int rvdepth, rvwidth, rvheight;
  volData->gridSize(rvdepth, rvwidth, rvheight);

  if (!validVolumeRange(rvdepth, rvwidth, rvheight,
                        dmin, dmax, wmin, wmax, hmin, hmax))
    {
      QMessageBox::warning(0, "Save",
                           "The selected volume range is invalid.");
      return false;
    }

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  uchar voxelType = volData->voxelType();  
  if (voxelType > _Float)
    {
      QMessageBox::warning(0, "Save",
                           "PVL, MHD, and VDB conversion support scalar "
                           "volumes only. Use the RGB/RGBA export instead.");
      return false;
    }

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  int slabSize = dsz;
//  if (slabSize < dsz)
//    {  
//      QStringList items;
//      items << "no" << "yes";
//      QString yn = QInputDialog::getItem(0, "Split Volume",
//					 "Split volume larger than 1Gb into multiple files ?",
//					 items,
//					 0,
//					 false);
//      //*** max 1Gb per slab
//      if (yn != "yes") // put all in a single file
//	slabSize = dsz+1;
//    }
  //------------------------------------------------------

  QString pvlFilename = getPvlNcFilename();
  if (pvlFilename.endsWith(".mhd"))
    {
      return saveMHD(pvlFilename,
		     volData,
		     dmin, dmax,
		     wmin, wmax,
		     hmin, hmax);
    }

    if (pvlFilename.endsWith(".vdb"))
    {
      int tsfcount = qMax(1, timeseriesFiles.count());
      if (tsfcount == 1)
	{
	  if (!saveVDB(-1, pvlFilename, volData))
	    return false;
	  QMessageBox::information(0, "Save VDB", "Volume save to "+pvlFilename);
	}
      else
	{
	  for (int tsf=0; tsf<tsfcount; tsf++)
	    {
	      QString pvlflnm = pvlFilename;
	      if (tsfcount > 1)
		{
		  QFileInfo ftpvl(pvlFilename);
		  QFileInfo ftraw(timeseriesFiles[tsf]);
		  pvlflnm = QFileInfo(ftpvl.absolutePath(),
				      ftraw.completeBaseName() + ".vdb").absoluteFilePath();
		  
		  if (!volData->replaceFile(timeseriesFiles[tsf]))
		    {
		      QMessageBox::warning(0, "Save VDB", volData->lastError());
		      return false;
		    }
		}
	      
	      if (!saveVDB(tsf, pvlflnm, volData))
		return false;
		  
	    }
	  QMessageBox::information(0, "Save VDB", "Volumes saved to VDB files");
	}
      return true;
    }

    
  if (pvlFilename.count() < 4)
    {
      QMessageBox::information(0, "pvl.nc", "No .pvl.nc filename chosen.");
      return false;
    }

  bool choiceAccepted = false;
  bool save0AtTop = saveSliceZeroAtTop(&choiceAccepted);
  if (!choiceAccepted)
    return false;

  bool saveRawFile = getSaveRawFile(&choiceAccepted);
  if (!choiceAccepted)
    return false;

  QString rawfile;
  if (saveRawFile)
    {
      rawfile = getRawFilename(pvlFilename);
      if (rawfile.isEmpty())
	return false;
      if (QFileInfo(rawfile).absolutePath().compare(
            QFileInfo(pvlFilename).absolutePath(), Qt::CaseInsensitive) != 0)
        {
          QMessageBox::warning(0, "Save",
                               "PVL and RAW outputs must be in the same directory for atomic Save As.");
          return false;
        }
    }

  int svslz = getZSubsampling(dsz, wsz, hsz, &choiceAccepted);
  if (!choiceAccepted)
    return false;
  int svsl = getXYSubsampling(svslz, dsz, wsz, hsz, &choiceAccepted);
  if (!choiceAccepted)
    return false;
  svslz = qBound(1, svslz, dsz);
  svsl = qBound(1, svsl, qMin(wsz, hsz));

  // Preserve partial trailing sampling blocks in the regular PVL Save As
  // path.  The conversion loops clamp each block to the selected ROI.
  const SamplingContract::Axis depthAxis(dsz, svslz);
  const SamplingContract::Axis widthAxis(wsz, svsl);
  const SamplingContract::Axis heightAxis(hsz, svsl);
  const int dsz2 = depthAxis.output;
  const int wsz2 = widthAxis.output;
  const int hsz2 = heightAxis.output;
  //------------------------------------------------------

  //------------------------------------------------------
  // get final volume size
  int final_dsz2 = dsz2;
  int final_wsz2 = wsz2;
  int final_hsz2 = hsz2;
  int pad_value = 0;
  int sfd = 0;
  int sfw = 0;
  int sfh = 0;
  int efd = 0;
  int efw = 0;
  int efh = 0;
  {
    bool ok;
    QString text;
    text = QInputDialog::getText(0,
				 "Final Volume Grid Size With Padding",
				 "Final Volume Grid Size With Padding",
				 QLineEdit::Normal,
				 QString("%1 %2 %3").\
				 arg(final_dsz2).\
				 arg(final_wsz2).\
				 arg(final_hsz2),
				 &ok);
    if (!ok)
      return false;
    if (text.trimmed().isEmpty())
      {
        QMessageBox::warning(0, "Save",
                             "The final volume grid size cannot be empty.");
        return false;
      }
    if (ok && !text.isEmpty())
      {
	QStringList list = text.split(" ", QString::SkipEmptyParts);
	if (list.count() != 3)
	  {
	    QMessageBox::warning(0, "Save",
	      "The final volume grid size must contain depth, width, and height.");
	    return false;
	  }
	if (list.count() == 3)
	  {
	    bool depthOk = false;
	    bool widthOk = false;
	    bool heightOk = false;
	    const int requestedDepth = list[0].toInt(&depthOk);
	    const int requestedWidth = list[1].toInt(&widthOk);
	    const int requestedHeight = list[2].toInt(&heightOk);
	    if (!depthOk || !widthOk || !heightOk)
	      {
		QMessageBox::warning(0, "Save",
		  "The final volume grid dimensions must be integers.");
		return false;
	      }
	    final_dsz2 = qMax(dsz2, requestedDepth);
	    final_wsz2 = qMax(wsz2, requestedWidth);
	    final_hsz2 = qMax(hsz2, requestedHeight);

	    int td = final_dsz2 - dsz2;
	    int tw = final_wsz2 - wsz2;
	    int th = final_hsz2 - hsz2;

	    sfd = td/2;
	    efd = td - sfd;

	    sfw = tw/2;
	    efw = tw - sfw;

	    sfh = th/2;
	    efh = th - sfh;

	    if (td != 0 || tw != 0 || th != 0)
	      {
		QString text;
		text = QInputDialog::getText(0,
					     "Pad volume With Value",
					     "Pad Volume With Value",
					     QLineEdit::Normal,
					     "0",
					     &ok);
		if (!ok)
		  return false;
		bool valueOk = false;
		const int value = text.toInt(&valueOk);
		if (!valueOk)
		  {
		    QMessageBox::warning(0, "Save",
		      "The padding value must be an integer.");
		    return false;
		  }
		pad_value = value;
	      }
	  }
      }

    slabSize = final_dsz2;
  }
  if (saveRawFile &&
      (final_dsz2 != dsz2 || final_wsz2 != wsz2 || final_hsz2 != hsz2))
    {
      QMessageBox::warning(0, "Save",
                           "RAW export with padding is not supported yet. "
                           "Choose PVL-only export or remove padding so the "
                           "RAW and PVL geometries remain identical.");
      return false;
    }
  //------------------------------------------------------

  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog;
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  int vu = volData->voxelUnit();
  savePvlDialog.setVoxelUnit(vu);
  // scale the voxelsize according to subsampling used
  savePvlDialog.setVoxelSize(vx*svsl, vy*svsl, vz*svslz);
  savePvlDialog.setDescription(desc);
  if (savePvlDialog.exec() != QDialog::Accepted)
    return false;

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  bool invertData = savePvlDialog.invertData();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);


  QList<float> rawMap = volData->rawMap();
  QList<int> pvlMap = volData->pvlMap();

  if (rawMap.count() < 2 || rawMap.count() != pvlMap.count())
    {
      QMessageBox::warning(0, "Save",
                           "The raw-to-PVL value map is invalid.");
      return false;
    }

  int pvlbpv = 1;
  if (pvlMap[pvlMap.count()-1] > 255)
    pvlbpv = 2;

  int pvlVoxelType = 0;
  if (pvlbpv == 2) pvlVoxelType = 2;

  bool subsample = (svsl > 1 || svslz > 1);

  //--------------------------
  int filterType = 0;
  if (subsample && spread > 0)
    {
      bool ok = true;
      
      QStringList items;
      items << "Tri-Linear Interpolation";
      items << "No Interpolation";
      QString item = QInputDialog::getItem(0,
					   QString("Subsampling Filter (%1)").arg(spread),
					   "FOR SEGMENTED DATA USE - NO INTERPOLATION",
					   items,
					   0,
					   false,
					   &ok);
      if (!ok)
	return false;
      if (ok && !item.isEmpty())
	{
	  QStringList op = item.split(' ');
	  if (op[0] == "No")
	    {
	      filterType = 1;
	      spread = 0;
	    }
	}
    }
  //--------------------------
  
  ConversionBuffers conversionBuffers;
  QString bufferError;
  if (!prepareConversionBuffers(
        QStringLiteral("PVL conversion"),
        rvwidth, rvheight, bpv,
        wsz2, hsz2, pvlbpv,
        final_wsz2, final_hsz2, spread, saveRawFile,
        conversionBuffers, bufferError))
    {
      QMessageBox::warning(0, "Save", bufferError);
      return false;
    }

  const std::size_t nbytes =
    static_cast<std::size_t>(conversionBuffers.rawBytes);
  double *filtervol = conversionBuffers.filter.get();
  uchar *pvlslice = conversionBuffers.pvl.get();
  uchar *raw = conversionBuffers.raw.get();
  uchar **val = conversionBuffers.filterSlices.get();
  int rawSize = rawMap.size()-1;
  int width = wsz2;
  int height = hsz2;
  bool trim = (dmin != 0 ||
	       wmin != 0 ||
	       hmin != 0 ||
	       dsz2 != rvdepth ||
	       wsz2 != rvwidth ||
	       hsz2 != rvheight);

  uchar *final_val = conversionBuffers.finalSlice.get();

  const auto readSlice = [volData](int sliceIndex, uchar *destination)
    {
      QString error;
      if (readExportSlice(volData, sliceIndex, destination, error))
	return true;
      QMessageBox::critical(0, "Save",
	QString("Cannot decode input slice %1: %2")
	.arg(sliceIndex).arg(error));
      return false;
    };

  // A Save As produces a header plus one or more PVL/RAW slabs.  Keep all
  // final paths in one journal so a failure in any later artifact restores the
  // previous generation instead of leaving a mixed header/data set.
  BatchPathJournal saveJournal;
  // Declare the managers after the journal so their destructors roll back
  // active VFM generations before the outer journal restores old paths.
  VolumeFileManager rawFileManager;
  VolumeFileManager pvlFileManager;
  const int plannedSlabCount = 1 + (final_dsz2-1)/slabSize;
  QString saveJournalError;
  QSet<QString> plannedBatchOutputs;
  qint64 voxelCount = 0;
  qint64 pvlDataBytes = 0;
  qint64 rawDataBytes = 0;
  qint64 pvlHeaderBytes = static_cast<qint64>(plannedSlabCount) * 13 +
                          1024*1024;
  const int plannedTimeSeriesCount = qMax(1, timeseriesFiles.count());
  if (!checkedMulBytes(static_cast<qint64>(final_dsz2), final_wsz2,
                       voxelCount) ||
      !checkedMulBytes(voxelCount, final_hsz2, voxelCount) ||
      !checkedMulBytes(voxelCount, pvlbpv, pvlDataBytes) ||
      (saveRawFile && !checkedMulBytes(voxelCount, bpv, rawDataBytes)))
    {
      QMessageBox::critical(0, "Save",
                            "The output volume size is too large to stage safely.");
      return false;
    }
  qint64 oneGeneration = 0;
  qint64 stagedRequirement = 0;
  if (!checkedAddBytes(pvlDataBytes, pvlHeaderBytes, oneGeneration) ||
      !checkedAddBytes(oneGeneration, rawDataBytes, oneGeneration) ||
      !checkedMulBytes(oneGeneration, 3, stagedRequirement) ||
      !checkedMulBytes(stagedRequirement, plannedTimeSeriesCount,
                       stagedRequirement) ||
      !checkedAddBytes(stagedRequirement, 64LL*1024LL*1024LL,
                       stagedRequirement))
    {
      QMessageBox::critical(0, "Save",
                            "The output volume staging requirement overflows.");
      return false;
    }
  // Recover an interrupted Save As before reserving any existing generation
  // for this run.  The journal is intentionally persisted in the output
  // directory so a later invocation can restore the previous header/slabs.
  QString recoveryError;
  if (!recoverBatchJournals(QFileInfo(pvlFilename).absolutePath(),
                            recoveryError))
    {
      QMessageBox::critical(0, "Save", QString("Cannot recover the previous batch: %1")
                             .arg(recoveryError));
      return false;
    }
  const QStorageInfo outputStorage(QFileInfo(pvlFilename).absolutePath());
  // Pre-register every output in a time-series batch before reading or
  // replacing any source frame.  This makes collisions and old-generation
  // preservation failures fail before the first output is touched.
  qint64 existingBatchBytes = 0;
  for (int tsf = 0; tsf < plannedTimeSeriesCount; ++tsf)
    {
      QString plannedPvl = pvlFilename;
      QString plannedRaw = rawfile;
      if (plannedTimeSeriesCount > 1)
        {
          QFileInfo ftpvl(pvlFilename);
          QFileInfo ftraw(timeseriesFiles[tsf]);
          plannedPvl = QFileInfo(ftpvl.absolutePath(),
                                 ftraw.completeBaseName() + ".pvl.nc")
                       .absoluteFilePath();
          plannedRaw = QFileInfo(ftpvl.absolutePath(),
                                 ftraw.completeBaseName() + ".raw")
                       .absoluteFilePath();
        }
      QStringList outputPaths;
      outputPaths << plannedPvl;
      for (int slab = 0; slab < plannedSlabCount; ++slab)
        outputPaths << QString("%1.%2").arg(plannedPvl)
                       .arg(slab+1, 3, 10, QChar('0'));
      outputPaths << numberedOutputPaths(plannedPvl);
      if (saveRawFile)
        {
          outputPaths << plannedRaw;
          for (int slab = 0; slab < plannedSlabCount; ++slab)
            outputPaths << QString("%1.%2").arg(plannedRaw)
                         .arg(slab+1, 3, 10, QChar('0'));
          outputPaths << numberedOutputPaths(plannedRaw);
        }
      QSet<QString> localManifestPaths;
      for (const QString& outputPath : outputPaths)
        {
          const QString cleanOutputPath = QFileInfo(outputPath).absoluteFilePath();
          if (localManifestPaths.contains(cleanOutputPath))
            continue;
          localManifestPaths.insert(cleanOutputPath);
          if (plannedBatchOutputs.contains(cleanOutputPath))
            {
              QMessageBox::critical(0, "Save",
                                     QString("Time-series outputs collide at '%1'.")
                                     .arg(cleanOutputPath));
              return false;
            }
          plannedBatchOutputs.insert(cleanOutputPath);
          const QFileInfo existingInfo(cleanOutputPath);
          if (existingInfo.exists() &&
              !checkedAddBytes(existingBatchBytes, existingInfo.size(),
                               existingBatchBytes))
            {
              QMessageBox::critical(0, "Save",
                                    "Existing output size exceeds the staging accounting range.");
              return false;
            }
          if (!saveJournal.addPath(cleanOutputPath, saveJournalError))
            {
              QMessageBox::critical(0, "Save", saveJournalError);
              return false;
            }
        }
    }
  if (outputStorage.isValid() &&
      (existingBatchBytes > outputStorage.bytesAvailable() ||
       outputStorage.bytesAvailable() - existingBatchBytes < stagedRequirement))
    {
      QMessageBox::critical(0, "Save",
                            "Insufficient disk space to preserve existing outputs and stage the complete Save As batch.");
      return false;
    }

  
  
  QProgressDialog progress("Saving processed volume",
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());
  
  //------------------------------------------------------
  int tsfcount = plannedTimeSeriesCount;
  for (int tsf=0; tsf<tsfcount; tsf++)
    {
      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  return false;
	}
	  
      QString pvlflnm = pvlFilename;
      QString rawflnm = rawfile;

      if (tsfcount > 1)
	{
	  QFileInfo ftpvl(pvlFilename);
	  QFileInfo ftraw(timeseriesFiles[tsf]);
	  pvlflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".pvl.nc").absoluteFilePath();

	  rawflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".raw").absoluteFilePath();

	  if (!volData->replaceFile(timeseriesFiles[tsf]))
	    {
	      QMessageBox::warning(0, "Save", volData->lastError());
	      return false;
	    }
	  if (!currentVolumeLayoutMatches(volData,
	                                  rvdepth, rvwidth, rvheight,
	                                  voxelType))
	    {
	      QMessageBox::warning(0, "Save",
	        "A time-series volume has a different grid or voxel type. "
	        "Conversion was stopped before reading into the fixed slice buffer.");
	      return false;
	    }
        }

      QStringList outputPaths;
      outputPaths << pvlflnm;
      for (int slab = 0; slab < plannedSlabCount; ++slab)
        outputPaths << QString("%1.%2").arg(pvlflnm).arg(slab+1, 3, 10, QChar('0'));
      outputPaths << numberedOutputPaths(pvlflnm);
      if (saveRawFile)
        {
          outputPaths << rawflnm;
          for (int slab = 0; slab < plannedSlabCount; ++slab)
            outputPaths << QString("%1.%2").arg(rawflnm).arg(slab+1, 3, 10, QChar('0'));
          outputPaths << numberedOutputPaths(rawflnm);
        }
      QSet<QString> localOutputPaths;
      for (const QString& outputPath : outputPaths)
        {
          const QString cleanOutputPath = QFileInfo(outputPath).absoluteFilePath();
          if (localOutputPaths.contains(cleanOutputPath))
            continue;
          localOutputPaths.insert(cleanOutputPath);
          if (!plannedBatchOutputs.contains(cleanOutputPath))
            {
              QMessageBox::critical(0, "Save",
                                     QString("Time-series output manifest changed unexpectedly at '%1'.")
                                     .arg(cleanOutputPath));
              return false;
            }
        }

      pvlFileManager.setBaseFilename(pvlflnm);
//      pvlFileManager.setDepth(dsz2);
//      pvlFileManager.setWidth(wsz2);
//      pvlFileManager.setHeight(hsz2);
      pvlFileManager.setDepth(final_dsz2);
      pvlFileManager.setWidth(final_wsz2);
      pvlFileManager.setHeight(final_hsz2);
      pvlFileManager.setVoxelType(pvlVoxelType);
      pvlFileManager.setHeaderSize(13);
      pvlFileManager.setSlabSize(slabSize);
      pvlFileManager.setSliceZeroAtTop(save0AtTop);
      if (!pvlFileManager.createFile(true))
	{
	  QMessageBox::critical(0, "Save", pvlFileManager.lastError());
	  return false;
	}
      
      if (saveRawFile)
	{
	  rawFileManager.setBaseFilename(rawflnm);
	  rawFileManager.setDepth(final_dsz2);
	  rawFileManager.setWidth(final_wsz2);
	  rawFileManager.setHeight(final_hsz2);
	  rawFileManager.setVoxelType(voxelType);
	  rawFileManager.setHeaderSize(13);
	  rawFileManager.setSlabSize(slabSize);
	  rawFileManager.setSliceZeroAtTop(save0AtTop);
	  if (rawFileManager.exists())
	    {
	      bool ok = false;
	      QStringList slevels;
	      slevels << "Yes - overwrite";
	      slevels << "No";  
	      QString option = QInputDialog::getItem(0,
						     "Save RAW Volume",
						     QString("%1 exists - Overwrite ?"). \
						     arg(rawFileManager.fileName()),
						     slevels,
						     0,
						     false,
						     &ok);
	      if (!ok)
		return false;
	      
	      QStringList op = option.split(' ');
	      if (op[0] != "Yes")
		{
		  QMessageBox::information(0, "Save",
	        QString("Please choose a different name for the preprocessed volume - RAW file not overwritten"));
		  return false;
		}
	    }
	  if (!rawFileManager.createFile(true))
	    {
	      QMessageBox::critical(0, "Save", rawFileManager.lastError());
	      return false;
	    }
	}
      //------------------------------------------------------


      // ------------------
      // add padding
      if (sfd > 0)
	{
	  fillVoxelBuffer(final_val,
                   static_cast<std::size_t>(final_wsz2)*final_hsz2,
                   pvlbpv, pad_value);
	  for(int esl=0; esl<sfd; esl++)
	    if (!pvlFileManager.setSlice(esl, final_val))
	      {
		QMessageBox::critical(0, "Save", pvlFileManager.lastError());
		return false;
	      }
	}
      // ------------------
	

      // ------------------
      // calculate weights for Gaussian filter
      float weights[100];
      calculateGaussianWeights(spread, weights);
      // ------------------
      
      
      for(int dd=0; dd<dsz2; dd++)
	{

	  if (progress.wasCanceled())
	    {
	      progress.setValue(100);  
	      QMessageBox::information(0, "Save", "-----Aborted-----");
	      return false;
	    }

	  int d0 = dmin + dd*svslz;
	  int d1 = qMin(dmax, d0 + svslz-1);

	  if (spread == 0) // No Filter - Nearest Neighbour
	    {
	      d0 = dmin + dd*svslz;
	      d1 = d0;
	    }
	  
	  progress.setValue((int)(100*(float)dd/(float)dsz2));
	  qApp->processEvents();
	  
	  memset(filtervol, 0,
                 static_cast<std::size_t>(conversionBuffers.filterBytes));
	  for (int d=d0; d<=d1; d++)
	    {
	      if (spread > 0)
		{
		  if (d == d0)
		    {
		      if (!readSlice(d, val[spread])) return false;
		      applyMeanFilterToSlice(val[spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);

		      for(int i=-spread; i<0; i++)
			{
			  if (d+i >= 0)
			    { if (!readSlice(d+i, val[spread+i])) return false; }
			  else
			    { if (!readSlice(0, val[spread+i])) return false; }

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter, weights);
			}
		      
		      for(int i=1; i<=spread; i++)
			{
			  if (d+i < rvdepth)
			    { if (!readSlice(d+i, val[spread+i])) return false; }
			  else
			    { if (!readSlice(rvdepth-1, val[spread+i])) return false; }

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter, weights);
			}
		    }
		  else if (d < rvdepth-spread)
		    {
		      if (!readSlice(d+spread, val[2*spread])) return false;
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);
		    }		  
		  else
		    {
		      if (!readSlice(rvdepth-1, val[2*spread])) return false;
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);
		    }		  
		  // smoothed data is now in val[2*spread]
		  // copy that into raw
		  memcpy(raw, val[2*spread], nbytes);
		}
	      else // spread == 0
		{ if (!readSlice(d, raw)) return false; }
	      
	      if (spread > 0)
		{
		  applyMeanFilter(val, raw,
				  voxelType, rvwidth, rvheight,
				  spread, dilateFilter, weights);
		  
		  // now shift the planes
		  uchar *tmp = val[0];
		  for(int i=0; i<2*spread; i++)
		    val[i] = val[i+1];
		  val[2*spread] = tmp;
		}
	      
	      if (trim || subsample)
		{
		  int fi = 0;
		  for(int j=0; j<wsz2; j++)
		    {
		      int y0 = wmin+j*svsl;
		      int y1 = qMin(wmax, y0+svsl-1);
		      for(int i=0; i<hsz2; i++)
			{
			  int x0 = hmin+i*svsl;
		      int x1 = qMin(hmax, x0+svsl-1);
			  for(int y=y0; y<=y1; y++)
			    for(int x=x0; x<=x1; x++)
			      {
				// The no-filter contract is nearest-neighbour anchored at
				// the first voxel of each source block.  Do not let later
				// pixels overwrite that anchor.
				if (spread == 0 && (y != y0 || x != x0))
				  continue;
				if (spread > 0)
				  {
				    if (voxelType == _UChar)
				      { uchar *ptr = raw; filtervol[fi] += ptr[y*rvheight+x]; }
				    else if (voxelType == _Char)
				      { char *ptr = (char*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				    else if (voxelType == _UShort)
				      { ushort *ptr = (ushort*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				    else if (voxelType == _Short)
				      { short *ptr = (short*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				    else if (voxelType == _Int)
				      { int *ptr = (int*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				    else if (voxelType == _Float)
				      { float *ptr = (float*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				  }
				else // no filter
				  {
				    if (voxelType == _UChar)
				      { uchar *ptr = raw; filtervol[fi] = ptr[y*rvheight+x]; }
				    else if (voxelType == _Char)
				      { char *ptr = (char*)raw; filtervol[fi] = ptr[y*rvheight+x]; }
				    else if (voxelType == _UShort)
				      { ushort *ptr = (ushort*)raw; filtervol[fi] = ptr[y*rvheight+x]; }
				    else if (voxelType == _Short)
				      { short *ptr = (short*)raw; filtervol[fi] = ptr[y*rvheight+x]; }
				    else if (voxelType == _Int)
				      { int *ptr = (int*)raw; filtervol[fi] = ptr[y*rvheight+x]; }
				    else if (voxelType == _Float)
				      { float *ptr = (float*)raw; filtervol[fi] = ptr[y*rvheight+x]; }
				  }
			      }
			  fi++;
			}
		    }
		} // trim || subsample
	    }
	  
	  if (trim || subsample)
	    {
	      if (spread > 0)
		{
		  int fi = 0;
		  for (int j=0; j<wsz2; ++j)
		    {
		      for (int i=0; i<hsz2; ++i, ++fi)
		        {
		          const double blockSamples = static_cast<double>(
		            SamplingContract::sampleCount(depthAxis, widthAxis,
		                                          heightAxis, dd, j, i));
		          const double safeBlockSamples = qMax(1.0, blockSamples);
		          filtervol[fi] /= safeBlockSamples;
		        }
		    }
		}
	      
	      if (voxelType == _UChar)
		{
		  uchar *ptr = raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Char)
		{
		  char *ptr = (char*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _UShort)
		{
		  ushort *ptr = (ushort*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Short)
		{
		  short *ptr = (short*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Int)
		{
		  int *ptr = (int*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Float)
		{
		  float *ptr = (float*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	    } // trim || subsample
	  
	  if (saveRawFile)
	    if (!rawFileManager.setSlice(dd, raw))
	      {
		QMessageBox::critical(0, "Save", rawFileManager.lastError());
		return false;
	      }
	  
	  applyMapping(raw, voxelType, rawMap,
		       pvlslice, pvlbpv, pvlMap,
		       width, height);

	  if (invertData)
	    {
	      if (pvlbpv == 1)
		{
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    pvlslice[fi] = 255-pvlslice[fi];
		}
	      else
		{
		  ushort *ptr = (ushort*)pvlslice;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = 65535-ptr[fi];
		}
	    }
	  
	  if (sfw == 0 && sfh == 0)
	    {
	      if (!pvlFileManager.setSlice(sfd+dd, pvlslice))
		{
		  QMessageBox::critical(0, "Save", pvlFileManager.lastError());
		  return false;
		}
	    }
	  else // add padding if required
	    {
	      fillVoxelBuffer(final_val,
                       static_cast<std::size_t>(final_wsz2)*final_hsz2,
                       pvlbpv, pad_value);
	      if (pvlbpv == 1)
		{
		  for(int wi=0; wi<wsz2; wi++)
		    for(int hi=0; hi<hsz2; hi++)
		      final_val[(wi+sfw)*final_hsz2+(hi+sfh)] = pvlslice[wi*hsz2+hi];
		}
	      else
		{
		  for(int wi=0; wi<wsz2; wi++)
		    for(int hi=0; hi<hsz2; hi++)
		      ((ushort*)final_val)[(wi+sfw)*final_hsz2+(hi+sfh)] = ((ushort*)pvlslice)[wi*hsz2+hi];
		}
	      if (!pvlFileManager.setSlice(sfd+dd, final_val))
		{
		  QMessageBox::critical(0, "Save", pvlFileManager.lastError());
		  return false;
		}
	    }
	}

      // -------------------------
      // add padding if required
      if (efd > 0)
	{
	  fillVoxelBuffer(final_val,
                   static_cast<std::size_t>(final_wsz2)*final_hsz2,
                   pvlbpv, pad_value);
	  for(int esl=0; esl<efd; esl++)
	    if (!pvlFileManager.setSlice(dsz2+sfd+esl, final_val))
	      {
		QMessageBox::critical(0, "Save", pvlFileManager.lastError());
		return false;
	      }
	}
      // -------------------------

      if (!savePvlHeader(pvlflnm,
			 saveRawFile, rawflnm+".001",
			 voxelType, pvlVoxelType, voxelUnit,
			 final_dsz2, final_wsz2, final_hsz2,
			 vx, vy, vz,
			 rawMap, pvlMap,
			 description,
			 slabSize,
                         volData->sourceFiles()))
	{
	  QMessageBox::critical(0, "Save", g_pvlHeaderWriteError);
	  return false;
	}
      const bool pvlCommitted = pvlFileManager.commitFileCreation();
      const bool rawCommitted = !saveRawFile ||
	                        rawFileManager.commitFileCreation();
      if (!pvlCommitted)
	{
	  QMessageBox::critical(0, "Save", pvlFileManager.lastError());
	  return false;
	}
      if (!rawCommitted)
	{
	  QMessageBox::critical(0, "Save", rawFileManager.lastError());
	  return false;
	}
      QString staleSlabError;
      const int committedSlabs = 1 + (final_dsz2-1)/slabSize;
      if (!removeStaleNumberedSlabs(pvlflnm, committedSlabs, staleSlabError) ||
          (saveRawFile && !removeStaleNumberedSlabs(rawflnm, committedSlabs,
                                                    staleSlabError)))
	{
	  QMessageBox::warning(0, "Save", staleSlabError);
	  return false;
	}
    }

  progress.setValue(100);

  if (!saveJournal.commit())
    {
      QMessageBox::critical(0, "Save",
                            "Cannot finalize the saved volume transaction.");
      return false;
    }


  QMessageBox mb;
  mb.setWindowTitle("Save");
  mb.setText("-----Done-----");
  mb.setWindowFlags(Qt::Dialog|Qt::WindowStaysOnTopHint);
  mb.exec();
  
//QMessageBox::information(0, "Save", "-----Done-----");

  return true;
}

void
saveSettings(int memGb,
	     int spareMb)
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".meshgenerator");

  QFile fin(settingsFile.absoluteFilePath());
  if (fin.open(QIODevice::WriteOnly | QIODevice::Text))
    {
      QTextStream out(&fin);
      out << "main memory :: " << memGb << "\n";
      out << "keep spare :: " << spareMb << "\n";
    }
}

bool
loadSettings(int &memGb,
	     int &spareMb)
{
  QString homePath = QDir::homePath();
  QFileInfo settingsFile(homePath, ".meshgenerator");

  memGb = 1;
  spareMb = 500;

  bool ok = false;
  if (settingsFile.exists())
    {
      QFile fin(settingsFile.absoluteFilePath());
      if (fin.open(QIODevice::ReadOnly | QIODevice::Text))
	{
	  QTextStream in(&fin);
	  if (!in.atEnd())
	    {
	      QString line = in.readLine();
	      QStringList words = line.split("::");
	      memGb = words[1].toInt();

	      if (!in.atEnd())
		{
		  line = in.readLine();
		  words = line.split("::");
		  spareMb = words[1].toInt();
		}
	      
	      ok = true;
	    }
	}
    }

  return ok;
}

bool
checkSettings(int memGb,
	     int spareMb)
{
  bool ok = true;

  QStringList items;
  items << "Do not change memory settings";
  items << "Change memory settings";
  QString item = QInputDialog::getItem(0,
				       "Memory settings",
				       QString("Main memory : %1 GB\nKeep spare : %2 Mb").\
				       arg(memGb).arg(spareMb),
				       items,
				       0,
				       false,
				       &ok);
  if (ok && !item.isEmpty())
    {
      if (item == "Change memory settings")
	ok = false;
    }
  else if (!ok)
    ok = true;

  return ok;
}

void
getSettings(int &memGb,
	    int &spareMb)
{
  bool ok = false;
  int mem = QInputDialog::getInt(0, "Main Memory Size in GB", "size (GB)", 1, 1, 1000, 1, &ok);
  if (!ok)
    return;
  memGb = mem;

  mem = QInputDialog::getInt(0, "Keep Spare Memory (in MB)", "size (MB)", 1, 1, 1000, 1, &ok);
  if (!ok)
    return;
  spareMb = mem;
}


void
Raw2Pvl::batchProcess(VolumeData* volData,
		      QStringList timeseriesFiles)
{
  QString pvlFilename = getPvlNcFilename();
  if (pvlFilename.count() < 4)
    {
      QMessageBox::information(0, "pvl.nc", "No .pvl.nc filename chosen.");
      return;
    }

  // Resolve the complete batch manifest before touching any output.  A
  // basename-only scheme silently aliases inputs from different directories;
  // reject that ambiguity rather than allowing later time points to overwrite
  // earlier results.
  const QFileInfo requestedPvl(pvlFilename);
  const QString outputDirectory = requestedPvl.absolutePath();
  // A previous process may have exited after moving an old output aside but
  // before committing the new batch. Restore that generation before starting
  // another batch in the same directory.
  QString recoveryError;
  if (!recoverBatchJournals(outputDirectory, recoveryError))
    {
      QMessageBox::warning(0, "Batch Processing",
                           QString("Cannot recover the previous batch: %1")
                           .arg(recoveryError));
      return;
    }
  const int requestedCount = qMax(1, timeseriesFiles.count());
  const auto outputStem = [](const QFileInfo& info) -> QString
    {
      QString name = info.fileName();
      if (name.endsWith(".pvl.nc", Qt::CaseInsensitive))
        name.chop(7);
      else if (name.endsWith(".pvl", Qt::CaseInsensitive))
        name.chop(4);
      else
        name = info.completeBaseName();
      return name;
    };
  bool choiceAccepted = false;
  bool saveRawFile = getSaveRawFile(&choiceAccepted);
  if (!choiceAccepted)
    return;
  QString rawfile;
  if (saveRawFile)
    rawfile = getRawFilename(pvlFilename);
  if (rawfile.isEmpty())
    saveRawFile = false;
  BatchPathJournal batchJournal;
  QStringList batchBaseNames;
  QSet<QString> outputBases;
  for (int index=0; index<requestedCount; ++index)
    {
      QString base = outputStem(requestedPvl);
      if (requestedCount > 1)
        {
          const QFileInfo source(timeseriesFiles.at(index));
          base = outputStem(source);
        }
      if (base.isEmpty())
        {
          QMessageBox::warning(0, "Batch Processing",
                               "A time-series input has no usable output basename.");
          return;
        }
      const QString normalized = QDir::cleanPath(
        QDir(outputDirectory).absoluteFilePath(base + ".pvl.nc")).toLower();
      if (outputBases.contains(normalized))
        {
          QMessageBox::warning(0, "Batch Processing",
                               QString("Time-series output name collision: %1")
                               .arg(base + ".pvl.nc"));
          return;
        }
      outputBases.insert(normalized);
      batchBaseNames << base;

      QString journalError;
      if (!batchJournal.addPath(QFileInfo(outputDirectory,
                                          base + ".pvl.nc").absoluteFilePath(),
                                journalError))
        {
          QMessageBox::warning(0, "Batch Processing", journalError);
          return;
        }
      if (saveRawFile)
        {
          const QString rawTarget = requestedCount == 1 ?
            QFileInfo(rawfile).absoluteFilePath() :
            QFileInfo(outputDirectory, base + ".raw").absoluteFilePath();
          if (!batchJournal.addPath(rawTarget,
                                    journalError))
            {
              QMessageBox::warning(0, "Batch Processing", journalError);
              return;
            }
        }
    }

  bool save0AtTop = saveSliceZeroAtTop(&choiceAccepted);
  if (!choiceAccepted)
    return;

  int svslz = getZSubsampling(1024, 1024, 1024, &choiceAccepted);
  if (!choiceAccepted)
    return;
  int svsl = getXYSubsampling(svslz, 1024, 1024, 1024, &choiceAccepted);
  if (!choiceAccepted)
    return;
  //------------------------------------------------------

  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog;
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  savePvlDialog.setVoxelUnit(Raw2Pvl::_Micron);
  savePvlDialog.setVoxelSize(vx, vy, vz);
  savePvlDialog.setDescription(desc);
  if (savePvlDialog.exec() != QDialog::Accepted)
    return;

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);

  bool subsample = (svsl > 1 || svslz > 1);
  bool trim = false;


  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }
  
  QProgressDialog progress("Saving processed volume",
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());

  //------------------------------
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);
  if (!validVolumeRange(rvdepth, rvwidth, rvheight,
                        0, rvdepth-1, 0, rvwidth-1, 0, rvheight-1))
    {
      QMessageBox::warning(0, "Batch Processing",
                           "The source volume dimensions are invalid.");
      return;
    }
  int dmin = 0;
  int wmin = 0;
  int hmin = 0;
  int dmax = rvdepth-1;
  int wmax = rvwidth-1;
  int hmax = rvheight-1;
  int dsz=rvdepth;
  int wsz=rvwidth;
  int hsz=rvheight;

  svslz = qBound(1, svslz, dsz);
  svsl = qBound(1, svsl, qMin(wsz, hsz));
  uchar voxelType = volData->voxelType();  

  if (voxelType > _Float)
    {
      QMessageBox::warning(0, "Batch Process",
                           "Batch conversion supports scalar volumes only.");
      return;
    }

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;
  
  // Preserve partial trailing sampling blocks instead of dropping the tail.
  const SamplingContract::Axis depthAxis(dsz, svslz);
  const SamplingContract::Axis widthAxis(wsz, svsl);
  const SamplingContract::Axis heightAxis(hsz, svsl);
  const int dsz2 = depthAxis.output;
  const int wsz2 = widthAxis.output;
  const int hsz2 = heightAxis.output;

  QList<float> rawMap = volData->rawMap();
  QList<int> pvlMap = volData->pvlMap();
  if (rawMap.count() < 2 || rawMap.count() != pvlMap.count())
    {
      QMessageBox::warning(0, "Batch Processing",
                           "The raw-to-PVL value map is invalid.");
      return;
    }
  
  int pvlbpv = 1;
  if (pvlMap[pvlMap.count()-1] > 255)
    pvlbpv = 2;
      
  int pvlVoxelType = 0;
  if (pvlbpv == 2) pvlVoxelType = 2;

  ConversionBuffers conversionBuffers;
  QString bufferError;
  if (!prepareConversionBuffers(
        QStringLiteral("Batch PVL conversion"),
        rvwidth, rvheight, bpv,
        wsz2, hsz2, pvlbpv,
        0, 0, spread, saveRawFile,
        conversionBuffers, bufferError))
    {
      QMessageBox::warning(0, "Batch Processing", bufferError);
      return;
    }

  double *filtervol = conversionBuffers.filter.get();
  uchar *pvlslice = conversionBuffers.pvl.get();
  uchar *raw = conversionBuffers.raw.get();
  uchar **val = conversionBuffers.filterSlices.get();

  const auto readSlice = [volData](int sliceIndex, uchar *destination)
    {
      QString error;
      if (readExportSlice(volData, sliceIndex, destination, error))
	return true;
      QMessageBox::critical(0, "Batch Processing",
	QString("Cannot decode input slice %1: %2")
	.arg(sliceIndex).arg(error));
      return false;
    };

  const std::uint64_t oneGiB = 1024ULL*1024ULL*1024ULL;
  const std::uint64_t slabCapacity = qMax<std::uint64_t>(
    1, oneGiB/conversionBuffers.rawBytes);
  const int slabSize = static_cast<int>(qMin<std::uint64_t>(
    static_cast<std::uint64_t>(dsz2), slabCapacity));
  const int batchSlabCount = 1 + (dsz2-1)/slabSize;
  for (int bi = 0; bi < batchBaseNames.count(); ++bi)
    {
      const QString pvlBase = QFileInfo(outputDirectory,
                                        batchBaseNames.at(bi) + ".pvl.nc")
        .absoluteFilePath();
      QString journalError;
      const auto protectExistingSlabs = [&](const QString& base) -> bool
        {
          const QFileInfo baseInfo(base);
          const QString prefix = baseInfo.fileName() + ".";
          const QStringList existing = baseInfo.absoluteDir().entryList(
            QStringList() << (baseInfo.fileName() + ".???"),
            QDir::Files | QDir::NoSymLinks);
          for (const QString& name : existing)
            {
              bool ok = false;
              name.mid(prefix.size()).toInt(&ok);
              if (ok && !batchJournal.addPath(
                    baseInfo.absoluteDir().absoluteFilePath(name),
                    journalError))
                return false;
            }
          return true;
        };
      if (!protectExistingSlabs(pvlBase))
        {
          QMessageBox::warning(0, "Batch Processing", journalError);
          return;
        }
      for (int slab = 1; slab <= batchSlabCount; ++slab)
        if (!batchJournal.addPath(QString("%1.%2")
                                  .arg(pvlBase)
                                  .arg(slab, 3, 10, QChar('0')),
                                  journalError))
          {
            QMessageBox::warning(0, "Batch Processing", journalError);
            return;
          }
      if (saveRawFile)
        {
          const QString rawBase = requestedCount == 1 ?
            QFileInfo(rawfile).absoluteFilePath() :
            QFileInfo(outputDirectory, batchBaseNames.at(bi) + ".raw")
              .absoluteFilePath();
          if (!protectExistingSlabs(rawBase))
            {
              QMessageBox::warning(0, "Batch Processing", journalError);
              return;
            }
          for (int slab = 1; slab <= batchSlabCount; ++slab)
            if (!batchJournal.addPath(QString("%1.%2")
                                      .arg(rawBase)
                                      .arg(slab, 3, 10, QChar('0')),
                                      journalError))
              {
                QMessageBox::warning(0, "Batch Processing", journalError);
                return;
              }
        }
    }
  int rawSize = rawMap.size()-1;
  int width = wsz2;
  int height = hsz2;
  //------------------------------

  
  //------------------------------------------------------
  int tsfcount = qMax(1, timeseriesFiles.count());
  for (int tsf=0; tsf<tsfcount; tsf++)
    {

      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  return;
	}

      QString pvlflnm = pvlFilename;
      QString rawflnm = rawfile;

      if (tsfcount > 1)
	{
	  QFileInfo ftpvl(pvlFilename);
	  QFileInfo ftraw(timeseriesFiles[tsf]);
	  pvlflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".pvl.nc").absoluteFilePath();

	  rawflnm = QFileInfo(ftpvl.absolutePath(),
			      ftraw.completeBaseName() + ".raw").absoluteFilePath();

	  if (!volData->replaceFile(timeseriesFiles[tsf]))
	    {
	      QMessageBox::warning(0, "Batch Processing", volData->lastError());
	      return;
	    }
	  if (!currentVolumeLayoutMatches(volData,
	                                  rvdepth, rvwidth, rvheight,
	                                  voxelType))
	    {
	      QMessageBox::warning(0, "Batch Processing",
	        "A time-series volume has a different grid or voxel type. "
	        "Batch conversion was stopped before decoding it.");
	      return;
	    }
	  //QStringList flnms;
	  //flnms << timeseriesFiles[tsf];
	  //volData->setFile(flnms, (tsf>0));
	  //volData->setFile(flnms, vol4d);
	}

      VolumeFileManager rawFileManager;
      VolumeFileManager pvlFileManager;

      pvlFileManager.setBaseFilename(pvlflnm);
      pvlFileManager.setDepth(dsz2);
      pvlFileManager.setWidth(wsz2);
      pvlFileManager.setHeight(hsz2);
      pvlFileManager.setVoxelType(pvlVoxelType);
      pvlFileManager.setHeaderSize(13);
      pvlFileManager.setSlabSize(slabSize);
      pvlFileManager.setSliceZeroAtTop(save0AtTop);
      if (!pvlFileManager.createFile(true))
	{
	  QMessageBox::critical(0, "Batch Processing",
				pvlFileManager.lastError());
	  return;
	}
      
      if (saveRawFile)
	{
	  rawFileManager.setBaseFilename(rawflnm);
	  rawFileManager.setDepth(dsz2);
	  rawFileManager.setWidth(wsz2);
	  rawFileManager.setHeight(hsz2);
	  rawFileManager.setVoxelType(voxelType);
	  rawFileManager.setHeaderSize(13);
	  rawFileManager.setSlabSize(slabSize);
	  rawFileManager.setSliceZeroAtTop(save0AtTop);
	  if (!rawFileManager.createFile(true))
	    {
	      QMessageBox::critical(0, "Batch Processing",
				    rawFileManager.lastError());
	      return;
	    }
	}
      //------------------------------------------------------

      progress.setLabelText(pvlflnm);
      
      // ------------------
      // calculate weights for Gaussian filter
      float weights[100];
      calculateGaussianWeights(spread, weights);
      // ------------------
      
      for(int dd=0; dd<dsz2; dd++)
	{

	  if (progress.wasCanceled())
	    {
	      progress.setValue(100);  
	      QMessageBox::information(0, "Save", "-----Aborted-----");
	      return;
	    }

	  int d0 = dmin + dd*svslz; 
	  int d1 = qMin(dmax, d0 + svslz-1);
	  if (spread == 0)
	    d1 = d0;
	  
	  progress.setValue((int)(100*(float)dd/(float)dsz2));
	  qApp->processEvents();
	  
	  memset(filtervol, 0,
                 static_cast<std::size_t>(conversionBuffers.filterBytes));
	  for (int d=d0; d<=d1; d++)
	    {
	      if (spread > 0)
		{
		  if (d == d0)
		    {
		      if (!readSlice(d, val[spread])) return;
		      applyMeanFilterToSlice(val[spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);

		      for(int i=-spread; i<0; i++)
			{
			  if (d+i >= 0)
			    { if (!readSlice(d+i, val[spread+i])) return; }
			  else
			    { if (!readSlice(0, val[spread+i])) return; }

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter, weights);
			}
		      
		      for(int i=1; i<=spread; i++)
			{
			  if (d+i < rvdepth)
			    { if (!readSlice(d+i, val[spread+i])) return; }
			  else
			    { if (!readSlice(rvdepth-1, val[spread+i])) return; }

			  applyMeanFilterToSlice(val[spread+i], raw,
						 voxelType, rvwidth, rvheight,
						 spread, dilateFilter, weights);
			}
		    }
		  else if (d < rvdepth-spread)
		    {
		      if (!readSlice(d+spread, val[2*spread])) return;
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);
		    }
		  else
		    {
		      if (!readSlice(rvdepth-1, val[2*spread])) return;
		      applyMeanFilterToSlice(val[2*spread], raw,
					     voxelType, rvwidth, rvheight,
					     spread, dilateFilter, weights);
		    }
		}
	      else
		{ if (!readSlice(d, raw)) return; }
	      
	      if (spread > 0)
		{
		  applyMeanFilter(val, raw,
				  voxelType, rvwidth, rvheight,
				  spread, dilateFilter, weights);
		  
		  // now shift the planes
		  uchar *tmp = val[0];
		  for(int i=0; i<2*spread; i++)
		    val[i] = val[i+1];
		  val[2*spread] = tmp;
		}
	      
	      if (trim || subsample)
		{
		  int fi = 0;
		  for(int j=0; j<wsz2; j++)
		    {
		      int y0 = wmin+j*svsl;
	      int y1 = qMin(wmax, y0+svsl-1);
		      for(int i=0; i<hsz2; i++)
			{
			  int x0 = hmin+i*svsl;
		  int x1 = qMin(hmax, x0+svsl-1);
			      for(int y=y0; y<=y1; y++)
				for(int x=x0; x<=x1; x++)
				  {
				    if (spread == 0 && (y != y0 || x != x0))
				      continue;
				    if (voxelType == _UChar)
				  { uchar *ptr = raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Char)
				  { char *ptr = (char*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _UShort)
				  { ushort *ptr = (ushort*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Short)
				  { short *ptr = (short*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Int)
				  { int *ptr = (int*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
				else if (voxelType == _Float)
				  { float *ptr = (float*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      }
			  fi++;
			}
		    }
		} // trim || subsample
	    }
	  
	  if (trim || subsample)
	    {
	      if (subsample)
		{
		  int fi = 0;
		  for (int j=0; j<wsz2; ++j)
		    {
		      const int y0 = wmin+j*svsl;
		      const int y1 = qMin(wmax, y0+svsl-1);
		      for (int i=0; i<hsz2; ++i, ++fi)
			{
			  const std::uint64_t samples =
			    SamplingContract::sampleCount(depthAxis, widthAxis,
			                                 heightAxis, dd, j, i);
			  filtervol[fi] /= static_cast<double>(qMax<std::uint64_t>(1, samples));
			}
		    }
		}
	      
	      if (voxelType == _UChar)
		{
		  uchar *ptr = raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Char)
		{
		  char *ptr = (char*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _UShort)
		{
		  ushort *ptr = (ushort*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Short)
		{
		  short *ptr = (short*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Int)
		{
		  int *ptr = (int*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	      else if (voxelType == _Float)
		{
		  float *ptr = (float*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	    } // trim || subsample
	  
	  if (saveRawFile)
	    if (!rawFileManager.setSlice(dd, raw))
	      {
		QMessageBox::critical(0, "Batch Processing",
				      rawFileManager.lastError());
		return;
	      }
	  
	  applyMapping(raw, voxelType, rawMap,
		       pvlslice, pvlbpv, pvlMap,
		       width, height);
	  
	  if (!pvlFileManager.setSlice(dd, pvlslice))
	    {
	      QMessageBox::critical(0, "Batch Processing",
				    pvlFileManager.lastError());
	      return;
	    }
	} // end of dd loop

      if (!savePvlHeader(pvlflnm,
			 saveRawFile, rawflnm+".001",
			 voxelType, pvlVoxelType, voxelUnit,
			 dsz2, wsz2, hsz2,
			 vx, vy, vz,
			 rawMap, pvlMap,
			 description,
			 slabSize))
	{
	  QMessageBox::critical(0, "Batch Processing",
				g_pvlHeaderWriteError);
	  return;
	}
      const bool pvlCommitted = pvlFileManager.commitFileCreation(false);
      const bool rawCommitted = !saveRawFile ||
	                        rawFileManager.commitFileCreation(false);
      if (!pvlCommitted)
	{
	  pvlFileManager.rollbackFileCreation();
	  if (saveRawFile && rawCommitted)
	    rawFileManager.rollbackFileCreation();
	  QMessageBox::critical(0, "Batch Processing",
				pvlFileManager.lastError());
	  return;
	}
      if (!rawCommitted)
	{
	  pvlFileManager.rollbackFileCreation();
	  QMessageBox::critical(0, "Batch Processing",
				 rawFileManager.lastError());
	  return;
	}
	      if (!pvlFileManager.finalizeFileCreation() ||
	          (saveRawFile && !rawFileManager.finalizeFileCreation()))
	        {
	          QMessageBox::critical(0, "Batch Processing",
                                 "The output was written but old-generation cleanup failed.");
	          return;
	        }

      progress.setLabelText(QString("Processed %1 of %2").arg(tsf).arg(tsfcount));
    }

  progress.setValue(100);
  if (!batchJournal.commit())
    {
      QMessageBox::critical(0, "Batch Processing",
                            "The batch completed but its recovery journal "
                            "could not be finalized. Existing outputs were "
                            "left recoverable; retry after resolving the "
                            "directory permission problem.");
      return;
    }
  QMessageBox::information(0, "Batch Processing", "-----Done-----");
}

bool
Raw2Pvl::saveMHD(QString mhdFilename,
		 VolumeData* volData,
		 int dmin, int dmax,
		 int wmin, int wmax,
		 int hmin, int hmax)
{
  if (!volData)
    {
      QMessageBox::warning(0, "Save MetaImage", "No volume data is loaded.");
      return false;
    }

  bool saveByteData = false;
  bool ok = false;
  QStringList slevels;
  slevels << "Yes - (default)";  
  slevels << "No - save byte-mapped data";
  QString option = QInputDialog::getItem(0,
		   "Save Original Data",
		   "Save Original Data in MetaImage Format  ?",
		    slevels,
			  0,
		      false,
		       &ok);
  if (!ok)
    return false;
  QStringList op = option.split(' ');
  if (op[0] == "No")
    saveByteData = true;
  

  //------------------------------------------------------
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);

  if (!validVolumeRange(rvdepth, rvwidth, rvheight,
                        dmin, dmax, wmin, wmax, hmin, hmax))
    {
      QMessageBox::warning(0, "Save MetaImage",
                           "The selected volume range is invalid.");
      return false;
    }

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  bool choiceAccepted = false;
  int svslz = getZSubsampling(dsz, wsz, hsz, &choiceAccepted);
  if (!choiceAccepted)
    return false;
  int svsl = getXYSubsampling(svslz, dsz, wsz, hsz, &choiceAccepted);
  if (!choiceAccepted)
    return false;
  svslz = qBound(1, svslz, dsz);
  svsl = qBound(1, svsl, qMin(wsz, hsz));

  // MHD uses the same edge-block contract as PVL: every source voxel is
  // represented, including partial blocks at the selected ROI boundary.
  const SamplingContract::Axis depthAxis(dsz, svslz);
  const SamplingContract::Axis widthAxis(wsz, svsl);
  const SamplingContract::Axis heightAxis(hsz, svsl);
  const int dsz2 = depthAxis.output;
  const int wsz2 = widthAxis.output;
  const int hsz2 = heightAxis.output;

  uchar voxelType = volData->voxelType();  
  if (voxelType > _Float)
    {
      QMessageBox::warning(0, "Save MHD",
                           "MHD conversion supports scalar volumes only.");
      return false;
    }

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;
  //------------------------------------------------------

  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog;
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  savePvlDialog.setVoxelUnit(Raw2Pvl::_Micron);
  savePvlDialog.setVoxelSize(vx, vy, vz);
  savePvlDialog.setDescription(desc);
  if (savePvlDialog.exec() != QDialog::Accepted)
    return false;

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);
  //------------------------------------------------------


  QString zrawFilename = mhdFilename;
  zrawFilename.chop(3);
  zrawFilename += "raw";
  
  if (QFile::exists(zrawFilename))
    {
      QString zfl = QFileDialog::getSaveFileName(0,
						 "Save raw volume",
						 Global::previousDirectory(),
						 "File (*.raw)");
//						 0,
//						 QFileDialog::DontUseNativeDialog);

      if (zfl.isEmpty())
	{
	  QStringList items;
	  items << "No";
	  items << "Yes";
	  QString item = QInputDialog::getItem(0,
					       "Overwrite existing file ?",
					       QString("Overwrite %1 ").arg(zrawFilename),
					       items,
					       0,
					       false,
					       &ok);
	  if (item == "No" || !ok)
	    return false;
	}
      else
	zrawFilename = zfl;
      
      if (!zrawFilename.endsWith(".raw"))
	zrawFilename += ".raw";
    }

  // The header and RAW data are one logical MHD output.  Register both
  // paths before opening QSaveFile so an interrupted pair can be restored by
  // the next import operation.
  QString recoveryError;
  if (!recoverBatchJournals(QFileInfo(mhdFilename).absolutePath(),
                            recoveryError))
    {
      QMessageBox::critical(0, "Save MetaImage",
                            QString("Cannot recover the previous batch: %1")
                            .arg(recoveryError));
      return false;
    }
  BatchPathJournal mhdJournal;
  QString mhdJournalError;
  if (!mhdJournal.addPath(QFileInfo(mhdFilename).absoluteFilePath(),
                          mhdJournalError) ||
      !mhdJournal.addPath(QFileInfo(zrawFilename).absoluteFilePath(),
                          mhdJournalError))
    {
      QMessageBox::critical(0, "Save MetaImage", mhdJournalError);
      return false;
    }

  QList<float> rawMap = volData->rawMap();
  QList<int> pvlMap = volData->pvlMap();
  if (rawMap.count() < 2 || rawMap.count() != pvlMap.count())
    {
      QMessageBox::warning(0, "Save MetaImage",
                           "The raw-to-PVL value map is invalid.");
      return false;
    }

  ConversionBuffers conversionBuffers;
  QString bufferError;
  if (!prepareConversionBuffers(
        QStringLiteral("MetaImage conversion"),
        rvwidth, rvheight, bpv,
        wsz2, hsz2, 1,
        0, 0, spread, false,
        conversionBuffers, bufferError))
    {
      QMessageBox::warning(0, "Save MetaImage", bufferError);
      return false;
    }

  std::uint64_t outputPixels = 0;
  std::uint64_t outputRawBytes = 0;
  if (!checkedPlaneLayout(wsz2, hsz2, bpv,
                          outputPixels, outputRawBytes))
    {
      QMessageBox::warning(0, "Save MetaImage",
                           "The output slice size is invalid.");
      return false;
    }

  double *filtervol = conversionBuffers.filter.get();
  uchar *pvl = conversionBuffers.pvl.get();
  uchar *raw = conversionBuffers.raw.get();
  uchar **val = conversionBuffers.filterSlices.get();
  const int rawSize = rawMap.size()-1;

  const auto readSlice = [volData](int sliceIndex, uchar *destination)
    {
      QString error;
      if (readExportSlice(volData, sliceIndex, destination, error))
	return true;
      QMessageBox::critical(0, "Save MetaImage",
	QString("Cannot decode input slice %1: %2")
	.arg(sliceIndex).arg(error));
      return false;
    };

  
  QSaveFile mhd(mhdFilename);
  if (!mhd.open(QFile::WriteOnly | QFile::Text))
    {
      QMessageBox::critical(0, "Save MetaImage",
	QString("Cannot open temporary MHD output for '%1': %2")
	.arg(mhdFilename).arg(mhd.errorString()));
      return false;
    }
  {
    QTextStream out(&mhd);
    out.setCodec(QTextCodec::codecForLocale());
    out << "ObjectType = Image\n";
    out << "NDims = 3\n";
    out << "BinaryData = True\n";
    out << "BinaryDataByteOrderMSB = False\n";
    out << "CompressedData = False\n";
    out << "TransformMatrix = 1 0 0 0 1 0 0 0 1\n";
    out << "Offset = 0 0 0\n";
    out << "CenterOfRotation = 0 0 0\n";
    out << QString("ElementSpacing = %1 %2 %3\n").arg(vz).arg(vy).arg(vx);
    out << QString("DimSize = %1 %2 %3\n").arg(hsz2).arg(wsz2).arg(dsz2);
    out << "HeaderSize = 0\n";
    out << "AnatomicalOrientation = ???\n";
    if (saveByteData)
      out << "ElementType = MET_UCHAR\n";
    else
      {
	if (voxelType == _UChar)      out << "ElementType = MET_UCHAR\n";
	else if (voxelType == _Char)  out << "ElementType = MET_CHAR\n";
	else if (voxelType == _UShort)out << "ElementType = MET_USHORT\n";
	else if (voxelType == _Short) out << "ElementType = MET_SHORT\n";
	else if (voxelType == _Int)   out << "ElementType = MET_INT\n";
	else if (voxelType == _Float) out << "ElementType = MET_FLOAT\n";
      }
    const QString rflnm = MetaImagePathUtils::elementDataFileReference(
      mhdFilename, zrawFilename);
    out << QString("ElementDataFile = %1\n").arg(rflnm);
    out.flush();
    if (out.status() != QTextStream::Ok)
      {
	mhd.cancelWriting();
	QMessageBox::critical(0, "Save MetaImage",
	  QString("Cannot write MHD header '%1': %2")
	  .arg(mhdFilename).arg(mhd.errorString()));
	return false;
      }
  }

  QSaveFile zraw(zrawFilename);
  if (!zraw.open(QFile::WriteOnly))
    {
      mhd.cancelWriting();
      QMessageBox::critical(0, "Save MetaImage",
	QString("Cannot open temporary RAW output for '%1': %2")
	.arg(zrawFilename).arg(zraw.errorString()));
      return false;
    }

    int width = wsz2;
    int height = hsz2;
    bool subsample = (svsl > 1 || svslz > 1);
    bool trim = (dmin != 0 ||
		 wmin != 0 ||
		 hmin != 0 ||
		 dsz2 != rvdepth ||
		 wsz2 != rvwidth ||
		 hsz2 != rvheight);

    QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
    QWidget *mainWidget = 0;
    for(QWidget *w : topLevelWidgets)
      {
	if (w->isWindow())
	  {
	    mainWidget = w;
	    break;
	  }
      }
  

    QProgressDialog progress("Saving MetaImage volume",
			     "Cancel",
			     0, 100,
			     mainWidget,
			     Qt::Dialog|Qt::WindowStaysOnTopHint);
    progress.setMinimumDuration(0);
    progress.resize(500, 100);
    progress.move(QCursor::pos());
    
    // ------------------
    // calculate weights for Gaussian filter
    float weights[100];
    calculateGaussianWeights(spread, weights);
    // ------------------
      
    for(int dd=0; dd<dsz2; dd++)
      {

	if (progress.wasCanceled())
	  {
	    zraw.cancelWriting();
	    mhd.cancelWriting();
	    QMessageBox::information(0, "Save", "-----Aborted-----");
	    return false;
	  }

	int d0 = dmin + dd*svslz; 
	int d1 = qMin(dmax, d0 + svslz-1);
	if (spread == 0)
	  d1 = d0;
	  
	progress.setValue((int)(100*(float)dd/(float)dsz2));
	qApp->processEvents();
	  
	memset(filtervol, 0,
               static_cast<std::size_t>(conversionBuffers.filterBytes));
	for (int d=d0; d<=d1; d++)
	  {
	    if (spread > 0)
	      {
		if (d == d0)
		  {
		    if (!readSlice(d, val[spread])) return false;
		    applyMeanFilterToSlice(val[spread], raw,
					   voxelType, rvwidth, rvheight,
					   spread, dilateFilter, weights);
		    
		    for(int i=-spread; i<0; i++)
		      {
			if (d+i >= 0)
			  { if (!readSlice(d+i, val[spread+i])) return false; }
			else
			  { if (!readSlice(0, val[spread+i])) return false; }
			
			applyMeanFilterToSlice(val[spread+i], raw,
					       voxelType, rvwidth, rvheight,
					       spread, dilateFilter, weights);
		      }
		    
		    for(int i=1; i<=spread; i++)
		      {
			if (d+i < rvdepth)
			  { if (!readSlice(d+i, val[spread+i])) return false; }
			else
			  { if (!readSlice(rvdepth-1, val[spread+i])) return false; }
			
			applyMeanFilterToSlice(val[spread+i], raw,
					       voxelType, rvwidth, rvheight,
					       spread, dilateFilter, weights);
		      }
		  }
		else if (d < rvdepth-spread)
		  {
		    if (!readSlice(d+spread, val[2*spread])) return false;
		    applyMeanFilterToSlice(val[2*spread], raw,
					   voxelType, rvwidth, rvheight,
					   spread, dilateFilter, weights);
		  }		  
		else
		  {
		    if (!readSlice(rvdepth-1, val[2*spread])) return false;
		    applyMeanFilterToSlice(val[2*spread], raw,
					   voxelType, rvwidth, rvheight,
					   spread, dilateFilter, weights);
		  }		  
	      }
	    else
	      { if (!readSlice(d, raw)) return false; }
	    
	    if (spread > 0)
	      {
		applyMeanFilter(val, raw,
				voxelType, rvwidth, rvheight,
				spread, dilateFilter, weights);
		
		// now shift the planes
		uchar *tmp = val[0];
		for(int i=0; i<2*spread; i++)
		  val[i] = val[i+1];
		val[2*spread] = tmp;
	      }
	    
	    if (trim || subsample)
	      {
		int fi = 0;
		for(int j=0; j<wsz2; j++)
		  {
		    int y0 = wmin+j*svsl;
		    int y1 = qMin(wmax, y0+svsl-1);
		    for(int i=0; i<hsz2; i++)
		      {
			int x0 = hmin+i*svsl;
			  int x1 = qMin(hmax, x0+svsl-1);
			  for(int y=y0; y<=y1; y++)
			    for(int x=x0; x<=x1; x++)
			      {
				if (spread == 0 && (y != y0 || x != x0))
				  continue;
				if (voxelType == _UChar)
				{ uchar *ptr = raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Char)
				{ char *ptr = (char*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _UShort)
				{ ushort *ptr = (ushort*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Short)
				{ short *ptr = (short*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Int)
				{ int *ptr = (int*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			      else if (voxelType == _Float)
				{ float *ptr = (float*)raw; filtervol[fi] += ptr[y*rvheight+x]; }
			    }
			fi++;
		      }
		  }
	      } // trim || subsample
	  }
	
	if (trim || subsample)
	  {
	    if (subsample)
	      {
		int fi = 0;
		for (int j=0; j<wsz2; ++j)
		  {
		    const int y0 = wmin+j*svsl;
		    const int y1 = qMin(wmax, y0+svsl-1);
		    for (int i=0; i<hsz2; ++i, ++fi)
		      {
		    const std::uint64_t samples =
			  SamplingContract::sampleCount(depthAxis, widthAxis,
			                               heightAxis, dd, j, i);
			filtervol[fi] /= static_cast<double>(qMax<std::uint64_t>(1, samples));
		      }
		  }
	      }
	    
	    if (voxelType == _UChar)
	      {
		uchar *ptr = raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _Char)
	      {
		char *ptr = (char*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _UShort)
		{
		  ushort *ptr = (ushort*)raw;
		  for(int fi=0; fi<wsz2*hsz2; fi++)
		    ptr[fi] = filtervol[fi];
		}
	    else if (voxelType == _Short)
	      {
		short *ptr = (short*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _Int)
	      {
		int *ptr = (int*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	    else if (voxelType == _Float)
	      {
		float *ptr = (float*)raw;
		for(int fi=0; fi<wsz2*hsz2; fi++)
		  ptr[fi] = filtervol[fi];
	      }
	  } // trim || subsample
	
	if (!saveByteData) // save original volume
	  {
	    const qint64 requested = static_cast<qint64>(outputRawBytes);
	    if (zraw.write((char*)raw, requested) != requested)
	      {
		zraw.cancelWriting();
		mhd.cancelWriting();
		QMessageBox::critical(0, "Save MetaImage",
		  QString("Cannot write RAW output '%1': %2")
		  .arg(zrawFilename).arg(zraw.errorString()));
		return false;
	      }
	  }
	else
	  {
	    if (voxelType == _UChar)
	      {
		uchar *ptr = raw;
		REMAPVOLUME(outputPixels);
	      }
	    else if (voxelType == _Char)
	      {
		char *ptr = (char*)raw;
		REMAPVOLUME(outputPixels);
	      }
	    else if (voxelType == _UShort)
	      {
		ushort *ptr = (ushort*)raw;
		REMAPVOLUME(outputPixels);
	      }
	    else if (voxelType == _Short)
	      {
		short *ptr = (short*)raw;
		REMAPVOLUME(outputPixels);
	      }
	    else if (voxelType == _Int)
	      {
		int *ptr = (int*)raw;
		REMAPVOLUME(outputPixels);
	      }
	    else if (voxelType == _Float)
	      {
		float *ptr = (float*)raw;
		REMAPVOLUME(outputPixels);
	      }
	  
	    const qint64 requested =
	      static_cast<qint64>(conversionBuffers.pvlBytes);
	    if (zraw.write((char*)pvl, requested) != requested)
	      {
		zraw.cancelWriting();
		mhd.cancelWriting();
		QMessageBox::critical(0, "Save MetaImage",
		  QString("Cannot write RAW output '%1': %2")
		  .arg(zrawFilename).arg(zraw.errorString()));
		return false;
	      }
	  }
      }
    progress.setValue(100);

  if (!zraw.commit())
    {
      const QString commitError = zraw.errorString();
      mhd.cancelWriting();
      QMessageBox::critical(0, "Save MetaImage",
	QString("Cannot commit RAW output '%1': %2")
	.arg(zrawFilename).arg(commitError));
      return false;
    }

  if (!mhd.commit())
    {
      const QString commitError = mhd.errorString();
      QString message = QString("Cannot commit MHD header '%1': %2")
	                  .arg(mhdFilename).arg(commitError);
      QMessageBox::critical(0, "Save MetaImage", message);
      return false;
    }

  if (!mhdJournal.commit())
    {
      QMessageBox::critical(0, "Save MetaImage",
                            "The MHD/RAW pair could not be finalized; "
                            "the recovery journal was retained.");
      return false;
    }
  
  QMessageBox::information(0, "Save MetaImage Volume", "-----Done-----");
  return true;
}
//================================
//================================
bool
Raw2Pvl::mergeVolumes(VolumeData* volData,
		      int dmin, int dmax,
		      int wmin, int wmax,
		      int hmin, int hmax,
		      QStringList timeseriesFiles)
{
  //------------------------------------------------------
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);

  if (!validVolumeRange(rvdepth, rvwidth, rvheight,
                        dmin, dmax, wmin, wmax, hmin, hmax) ||
      timeseriesFiles.isEmpty())
    {
      QMessageBox::warning(0, "Merge Volumes",
                           "The selected range or input volume list is invalid.");
      return false;
    }

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  uchar voxelType = volData->voxelType();  
  if (voxelType > _Float)
    {
      QMessageBox::warning(0, "Merge Volumes",
                           "Volume merge supports scalar volumes only.");
      return false;
    }

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  int slabSize = dsz;
  //------------------------------------------------------

  QString pvlFilename = getPvlNcFilename();
  if (pvlFilename.endsWith(".mhd"))
    {
      QMessageBox::warning(0, "Merge Volumes",
	"MetaImage output does not support multi-volume mask merging. "
	"Choose a .pvl.nc output file.");
      return false;
    }

  if (pvlFilename.count() < 4)
    {
      QMessageBox::information(0, "pvl.nc", "No .pvl.nc filename chosen.");
      return false;
    }

  //------------------------------------------------------
  // -- get saving parameters for processed file
  SavePvlDialog savePvlDialog;
  float vx, vy, vz;
  volData->voxelSize(vx, vy, vz);
  QString desc = volData->description();
  int vu = volData->voxelUnit();
  savePvlDialog.setVoxelUnit(vu);
  savePvlDialog.setVoxelSize(vx, vy, vz);
  savePvlDialog.setDescription(desc);
  if (savePvlDialog.exec() != QDialog::Accepted)
    return false;

  int spread = savePvlDialog.volumeFilter();
  bool dilateFilter = savePvlDialog.dilateFilter();
  bool invertData = savePvlDialog.invertData();
  int voxelUnit = savePvlDialog.voxelUnit();
  QString description = savePvlDialog.description();
  savePvlDialog.voxelSize(vx, vy, vz);


  QList<float> rawMap = volData->rawMap();
  QList<int> pvlMap = volData->pvlMap();

  if (rawMap.count() < 2 || rawMap.count() != pvlMap.count())
    {
      QMessageBox::warning(0, "Merge Volumes",
                           "The raw-to-PVL value map is invalid.");
      return false;
    }

  QString recoveryError;
  if (!recoverBatchJournals(QFileInfo(pvlFilename).absolutePath(),
                            recoveryError))
    {
      QMessageBox::warning(0, "Merge Volumes",
                           QString("Cannot recover the previous batch: %1")
                           .arg(recoveryError));
      return false;
    }
  BatchPathJournal mergeJournal;
  QString mergeJournalError;
  if (!mergeJournal.addPath(QFileInfo(pvlFilename).absoluteFilePath(),
                            mergeJournalError))
    {
      QMessageBox::warning(0, "Merge Volumes", mergeJournalError);
      return false;
    }
  const QFileInfo mergeHeaderInfo(pvlFilename);
  const QString mergePrefix = mergeHeaderInfo.fileName() + ".";
  const QStringList existingMergeSlabs = mergeHeaderInfo.absoluteDir().entryList(
    QStringList() << (mergeHeaderInfo.fileName() + ".???"),
    QDir::Files | QDir::NoSymLinks);
  for (const QString& name : existingMergeSlabs)
    {
      bool numberOk = false;
      name.mid(mergePrefix.size()).toInt(&numberOk);
      if (numberOk && !mergeJournal.addPath(
            mergeHeaderInfo.absoluteDir().absoluteFilePath(name),
            mergeJournalError))
        {
          QMessageBox::warning(0, "Merge Volumes", mergeJournalError);
          return false;
        }
    }

  int pvlbpv = 1;
  if (pvlMap[pvlMap.count()-1] > 255)
    pvlbpv = 2;

  int pvlVoxelType = 0;
  if (pvlbpv == 2) pvlVoxelType = 2;

  
  std::uint64_t rawPixels = 0;
  std::uint64_t rawBytes = 0;
  std::uint64_t pvlPixels = 0;
  std::uint64_t pvlBytes = 0;
  std::uint64_t allocationBytes = 0;
  std::shared_ptr<ProcessMemoryReservation> mergeReservation;
  QString bufferError;
  if (!checkedPlaneLayout(rvwidth, rvheight, bpv,
                          rawPixels, rawBytes) ||
      !checkedPlaneLayout(wsz, hsz, pvlbpv,
                          pvlPixels, pvlBytes) ||
      !addImportBytes(rawBytes, allocationBytes) ||
      !addImportBytes(pvlBytes, allocationBytes) ||
      !addImportBytes(pvlBytes, allocationBytes) ||
      !admitImportBuffers(QStringLiteral("Volume merge"),
                          allocationBytes, bufferError,
                          mergeReservation))
    {
      if (bufferError.isEmpty())
        bufferError = QStringLiteral(
          "Volume merge was stopped because a slice size overflowed.");
      QMessageBox::warning(0, "Merge Volumes", bufferError);
      return false;
    }

  std::unique_ptr<uchar[]> pvlStorage;
  std::unique_ptr<uchar[]> mergedPvlStorage;
  std::unique_ptr<uchar[]> rawStorage;
  if (!allocateImportArray(pvlBytes, pvlStorage) ||
      !allocateImportArray(pvlBytes, mergedPvlStorage) ||
      !allocateImportArray(rawBytes, rawStorage))
    {
      QMessageBox::warning(0, "Merge Volumes",
                           "Volume merge could not allocate its admitted "
                           "buffers. No processing was started.");
      return false;
    }

  const std::size_t nbytes = static_cast<std::size_t>(rawBytes);
  uchar *pvlslice = pvlStorage.get();
  uchar *Mpvlslice = mergedPvlStorage.get();
  uchar *raw = rawStorage.get();
  int rawSize = rawMap.size()-1;
  int width = wsz;
  int height = hsz;

  VolumeFileManager rawFileManager;
  VolumeFileManager pvlFileManager;



  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }
  
  QProgressDialog progress("Saving processed volume",
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());


  pvlFileManager.setBaseFilename(pvlFilename);
  pvlFileManager.setDepth(dsz);
  pvlFileManager.setWidth(wsz);
  pvlFileManager.setHeight(hsz);
  pvlFileManager.setVoxelType(pvlVoxelType);
  pvlFileManager.setHeaderSize(13);
  pvlFileManager.setSlabSize(slabSize);
  pvlFileManager.setSliceZeroAtTop(false);
  if (!pvlFileManager.createFile(true))
    {
      QMessageBox::critical(0, "Merge Volumes",
			    pvlFileManager.lastError());
      return false;
    }


  //------------------------------------------------------
  int tsfcount = qMax(1, timeseriesFiles.count());

  int tagF = 255/tsfcount;
  if (pvlbpv == 2)
    tagF = 65535/tsfcount;

  QList<int> tagValues;
  for(int i=0; i<timeseriesFiles.count(); i++)
    {
      tagValues << (i+1)*tagF;
    }
  
  //-----------------------
  // decide tag values
  QTableWidget *tw = new QTableWidget();
  tw->setRowCount(timeseriesFiles.count());
  tw->setColumnCount(2);
  QStringList item;
  item.clear();
  item << "Mask";
  item << "Value";
  tw->setHorizontalHeaderLabels(item);

  for (int i=0; i<timeseriesFiles.count(); i++)
    {
      QFileInfo fi(timeseriesFiles[i]);
      QTableWidgetItem *n0 = new QTableWidgetItem(fi.baseName());
      n0->setFlags(n0->flags() ^ Qt::ItemIsEditable);
      tw->setItem(i, 0, n0);

      QTableWidgetItem *n1 = new QTableWidgetItem(QString("%1").arg(tagValues[i]));
      tw->setItem(i, 1, n1);
    }

  QPushButton *ok = new QPushButton("OK");
  QDialog *dg = new QDialog();
  dg->setModal(true);
  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(ok);
  layout->addWidget(tw);
  dg->setLayout(layout);
  QObject::connect(ok, SIGNAL(clicked()),
		   dg, SLOT(accept()));
  if (dg->exec() != QDialog::Accepted)
    {
      delete dg;
      return false;
    }
  
  for (int i=0; i<timeseriesFiles.count(); i++)
    {
      tagValues[i] = tw->item(i, 1)->text().toInt();

    }
  delete dg;
  //-----------------------
  
  for(int dd=0; dd<dsz; dd++)
    {

      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  return false;
	}

      progress.setValue((int)(100*(float)dd/(float)dsz));
      qApp->processEvents();
      
      memset(Mpvlslice, 0, static_cast<std::size_t>(pvlBytes));
      
      for (int tsf=0; tsf<tsfcount; tsf++)
	{
	  if (!volData->replaceFile(timeseriesFiles[tsf]))
	    {
	      QMessageBox::warning(0, "Merge Volumes", volData->lastError());
	      return false;
	    }
	  if (!currentVolumeLayoutMatches(volData,
	                                  rvdepth, rvwidth, rvheight,
	                                  voxelType))
	    {
	      QMessageBox::warning(0, "Merge Volumes",
	        "A source volume has a different grid or voxel type. "
	        "Merge was stopped before decoding it.");
	      return false;
	    }
	  
	  memset(raw, 0, nbytes);
	  QString sliceError;
	  if (!readExportSlice(volData, dd, raw, sliceError))
	    {
	      QMessageBox::critical(0, "Merge Volumes",
		QString("Cannot decode input slice %1: %2")
		.arg(dd).arg(sliceError));
	      return false;
	    }
	  
	  applyMapping(raw, voxelType, rawMap,
		       pvlslice, pvlbpv, pvlMap,
		       width, height);

	  
	  //-----------------
	  //merge data
	  if (pvlbpv == 1)
	    {
	      for(int fi=0; fi<wsz*hsz; fi++)
		{
		  if (pvlslice[fi] > 10)
		    {
		      Mpvlslice[fi] = tagValues[tsf];
		    }
		}
	    }
	  else
	    {
	      ushort *Mptr = (ushort*)Mpvlslice;
	      ushort *ptr = (ushort*)pvlslice;
	      for(int fi=0; fi<wsz*hsz; fi++)
		{
		  if (ptr[fi] > 10)
		    {
		      Mptr[fi] = tagValues[tsf];
		    }
		}
	    }
	  //-----------------
	}
	if (!pvlFileManager.setSlice(dd, Mpvlslice))
	  {
	    QMessageBox::critical(0, "Merge Volumes",
				  pvlFileManager.lastError());
	    return false;
	  }
    }

  if (!savePvlHeader(pvlFilename,
		     false, "",
		     voxelType, pvlVoxelType, voxelUnit,
		     dsz, wsz, hsz,
		     vx, vy, vz,
		     rawMap, pvlMap,
		     description,
		     slabSize))
    {
      QMessageBox::critical(0, "Merge Volumes", g_pvlHeaderWriteError);
      return false;
    }
  if (!pvlFileManager.commitFileCreation(false))
    {
      QMessageBox::critical(0, "Merge Volumes",
			    pvlFileManager.lastError());
      return false;
    }

  if (!mergeJournal.commit())
    {
      pvlFileManager.rollbackFileCreation();
      QMessageBox::critical(0, "Merge Volumes",
                            "The merged volume was written, but its "
                            "recovery journal could not be finalized.");
      return false;
    }

  pvlFileManager.finalizeFileCreation();

  progress.setValue(100);
  
  QMessageBox::information(0, "Save", "-----Done-----");
  return true;
}

//================================
//================================
bool
Raw2Pvl::quickRaw(VolumeData* volData,
		  QStringList fileNames)
{
  //------------------------------------------------------  
  int rvdepth, rvwidth, rvheight;    
  volData->gridSize(rvdepth, rvwidth, rvheight);

  if (!validVolumeRange(rvdepth, rvwidth, rvheight,
                        0, rvdepth-1, 0, rvwidth-1, 0, rvheight-1) ||
      fileNames.isEmpty())
    {
      QMessageBox::warning(0, "Quick RAW",
                           "The source dimensions or file list is invalid.");
      return false;
    }

  int dsz=rvdepth;
  int wsz=rvwidth;
  int hsz=rvheight;

  uchar voxelType = volData->voxelType();  
  int headerBytes = volData->headerBytes();

  if (voxelType > _Float)
    {
      QMessageBox::warning(0, "Quick RAW",
                           "Quick RAW conversion supports scalar volumes only. "
                           "Use the RGB/RGBA trimmed-volume export instead.");
      return false;
    }

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  QList<float> rawMap = volData->rawMap();
  QList<int> pvlMap = volData->pvlMap();

  if (rawMap.count() < 2 || rawMap.count() != pvlMap.count())
    {
      QMessageBox::warning(0, "Quick RAW",
                           "The raw-to-PVL value map is invalid.");
      return false;
    }

  int pvlbpv = 1;
  if (pvlMap[pvlMap.count()-1] > 255)
    pvlbpv = 2;

  int pvlVoxelType = 0;
  if (pvlbpv == 2) pvlVoxelType = 2;

  
  std::uint64_t rawPixels = 0;
  std::uint64_t rawBytes = 0;
  std::uint64_t pvlPixels = 0;
  std::uint64_t pvlBytes = 0;
  std::uint64_t outputBytes = 0;
  std::uint64_t allocationBytes = 0;
  std::shared_ptr<ProcessMemoryReservation> quickRawReservation;
  QString bufferError;
  if (!checkedPlaneLayout(rvwidth, rvheight, bpv,
                          rawPixels, rawBytes) ||
      !checkedPlaneLayout(wsz, hsz, pvlbpv,
                          pvlPixels, pvlBytes) ||
      !checkedImportMultiply(static_cast<std::uint64_t>(dsz),
                             pvlBytes, outputBytes) ||
      outputBytes > static_cast<std::uint64_t>(
                      std::numeric_limits<qint64>::max()-13) ||
      !addImportBytes(rawBytes, allocationBytes) ||
      !addImportBytes(pvlBytes, allocationBytes) ||
      !admitImportBuffers(QStringLiteral("Quick RAW conversion"),
                          allocationBytes, bufferError,
                          quickRawReservation))
    {
      if (bufferError.isEmpty())
        bufferError = QStringLiteral(
          "Quick RAW conversion was stopped because its size overflowed.");
      QMessageBox::warning(0, "Quick RAW", bufferError);
      return false;
    }

  std::unique_ptr<uchar[]> pvlStorage;
  std::unique_ptr<uchar[]> rawStorage;
  if (!allocateImportArray(pvlBytes, pvlStorage) ||
      !allocateImportArray(rawBytes, rawStorage))
    {
      QMessageBox::warning(0, "Quick RAW",
                           "Quick RAW conversion could not allocate its "
                           "admitted buffers. No output was opened.");
      return false;
    }

  const std::size_t nbytes = static_cast<std::size_t>(rawBytes);
  uchar *pvlslice = pvlStorage.get();
  uchar *raw = rawStorage.get();
  int rawSize = rawMap.size()-1;
  int width = wsz;
  int height = hsz;

  VolumeFileManager rawFileManager;
  QFileInfo fraw(fileNames[0]);
  QString rawflnm = QFileInfo(fraw.absolutePath(),
			      fraw.baseName() + ".raw").absoluteFilePath();
  
  QSaveFile m_qfile(rawflnm);
  if (!m_qfile.open(QFile::WriteOnly))
    {
      QMessageBox::warning(0, "Quick RAW",
                           "Cannot open the RAW output file for writing.");
      return false;
    }
  if (m_qfile.write((char*)&pvlVoxelType, 1) != 1 ||
      m_qfile.write((char*)&dsz, 4) != 4 ||
      m_qfile.write((char*)&wsz, 4) != 4 ||
      m_qfile.write((char*)&hsz, 4) != 4)
    {
      m_qfile.cancelWriting();
      QMessageBox::critical(0, "Quick RAW",
	QString("Cannot write the RAW header: %1").arg(m_qfile.errorString()));
      return false;
    }


  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }
  
  QProgressDialog progress("Saving "+rawflnm,
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());


  //------------------------------------------------------

  const qint64 sliceSize = static_cast<qint64>(pvlBytes);
  for(qint64 dd=0; dd<dsz; dd++)
    {
      if (progress.wasCanceled())
	{
	  m_qfile.cancelWriting();
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  return false;
	}
      
      progress.setValue((int)(100*(float)dd/(float)dsz));
      qApp->processEvents();
      
      memset(raw, 0, nbytes);
      QString sliceError;
      if (!readExportSlice(volData, static_cast<int>(dd), raw, sliceError))
	{
	  m_qfile.cancelWriting();
	  QMessageBox::critical(0, "Quick RAW",
	    QString("Cannot decode input slice %1: %2")
	    .arg(dd).arg(sliceError));
	  return false;
	}
	  
      applyMapping(raw, voxelType, rawMap,
		   pvlslice, pvlbpv, pvlMap,
		   width, height);

	  
      const qint64 offset = 13 + (dsz-1-dd)*sliceSize;
      if (!m_qfile.seek(offset) ||
	  m_qfile.write((char*)pvlslice, sliceSize) != sliceSize)
	{
	  m_qfile.cancelWriting();
	  QMessageBox::critical(0, "Quick RAW",
	    QString("Cannot write output slice %1: %2")
	    .arg(dd).arg(m_qfile.errorString()));
	  return false;
	}
    }

  const qint64 expectedSize = 13 + static_cast<qint64>(outputBytes);
  if (m_qfile.size() != expectedSize || !m_qfile.commit())
    {
      QMessageBox::critical(0, "Quick RAW",
	QString("Cannot commit RAW output '%1': %2")
	.arg(rawflnm).arg(m_qfile.errorString()));
      return false;
    }
  progress.setValue(100);
  
  //QMessageBox::information(0, "Save", "-----Done-----");
  return true;
}



void
Raw2Pvl::getBackgroundValues(int &bType, float &bValue1, float &bValue2)
{
  bType = -2;  
  bValue1 = 0;
  bValue2 = 0;

  bool ok;
  QString mtext;
  mtext += "Background Value\n";
  mtext += " <Val - all voxels below Val will be treated as background\n";
  mtext += "        example : <100 - treat all voxels below 100 as background voxels\n";
  mtext += "                  that means only consider voxels above 100\n\n";
  mtext += " =Val - all voxels not equal to Val will be treated as background\n";
  mtext += "        example : =100 - treat all voxels equal to 100 as background voxels\n";
  mtext += "                  that means only consider voxels not equal to 100\n\n";
  mtext += " >Val - all voxels above Val will be treated as background\n";
  mtext += "        example : >100 - treat all voxels above 100 as background voxels\n";
  mtext += "                  that means only consider voxels below 100\n\n";
  mtext += " >Val1 <Val2 - all voxels between Val1 and Val2 will be treated as background\n";
  mtext += "        example : >100 <200 - treat all voxels above 100 and below 200 as background voxels\n";
  mtext += "                  that means only consider voxels outside of 100 and 200\n\n";
  mtext += " <Val1 >Val2 - all voxels below Val1 or above Val2 will be treated as background\n";
  mtext += "        example : <100 >200 - treat all voxels below 100 or above 200 as background voxels\n";
  mtext += "                  that means only consider voxels between and including 100 and 200\n\n";
  QString text = QInputDialog::getText(0,
				       "Background Value",
				       mtext,
				       QLineEdit::Normal,
				       "=0",
				       &ok);  
  if (ok && !text.isEmpty())
    {
      QStringList list = text.split(" ", QString::SkipEmptyParts);
      if (list.count() == 2)
	{
	  if (list[0].left(1) == ">" && list[1].left(1) == "<")
	    {
	      bType = 2;
	      bValue1 = list[0].mid(1).toFloat();
	      bValue2 = list[1].mid(1).toFloat();
	    }
	  else if (list[0].left(1) == "<" && list[1].left(1) == ">")
	    {
	      bType = 3;
	      bValue1 = list[0].mid(1).toFloat();
	      bValue2 = list[1].mid(1).toFloat();
	    }
	}
      else if (list.count() == 1)
	{
	  if (list[0].left(1) == "<")
	    {
	      bType = -1;
	      bValue1 = list[0].mid(1).toInt();
	    }
	  if (list[0].left(2) == "!=")
	    {
	      bType = 0;
	      bValue1 = list[0].mid(2).toInt();
	    }
	  if (list[0].left(1) == ">")
	    {
	      bType = 1;
	      bValue1 = list[0].mid(1).toInt();
	    }
	}
    }
  if (bType == -2)
    {
      QMessageBox::information(0, "Background Value", QString("<Val,   !=Val,   >Val,   >Val1 <Val2,   <Val1 >Val2  expected.\nGot %1").arg(text));
      return;
    }
}

// Not storing uchar or ushort because Houdini/Omniverse cannot handle it without modifications
//using MyTree = openvdb::tree::Tree4<half, 5, 4, 3>::Type;
//using MyGrid = Grid<MyTree>;
int Raw2Pvl::m_vdb_bType;
float Raw2Pvl::m_vdb_bValue1;
float Raw2Pvl::m_vdb_bValue2;
float Raw2Pvl::m_vdb_resample;
bool
Raw2Pvl::saveVDB(int volIdx,
		 QString vdbFileName,
		 VolumeData* volData)
{
  if (!volData)
    {
      QMessageBox::warning(0, "Save VDB", "No volume data is loaded.");
      return false;
    }
  if (vdbFileName.trimmed().isEmpty())
    {
      QMessageBox::warning(0, "Save VDB", "No VDB output filename was chosen.");
      return false;
    }

  int bType = -2;  
  float bValue1 = 0;
  float bValue2 = 0;
  
  if (volIdx <= 0)
    {
      getBackgroundValues(bType, bValue1, bValue2);
      
      if (bType == -2)
	return false;

      Raw2Pvl::m_vdb_bType   = bType;
      Raw2Pvl::m_vdb_bValue1 = bValue1;
      Raw2Pvl::m_vdb_bValue2 = bValue2;
    }
  else
    {
      bType   = Raw2Pvl::m_vdb_bType;
      bValue1 = Raw2Pvl::m_vdb_bValue1;
      bValue2 = Raw2Pvl::m_vdb_bValue2;
    }
  
  
  int dsz, wsz, hsz;
  volData->gridSize(dsz, wsz, hsz);
  if (!validVolumeRange(dsz, wsz, hsz,
                        0, dsz-1, 0, wsz-1, 0, hsz-1))
    {
      QMessageBox::warning(0, "Save VDB",
                           "The source volume dimensions are invalid.");
      return false;
    }
  
  float resample;
  if (volIdx <= 0)
    {
      bool ok;
      resample = QInputDialog::getDouble(0, "Resampling",
					 "Resample\nValues greater than 1.0 means downsampling.\nValues less than 1.0 means upsampling.",
					 1, 0.1, 10, 2, &ok, Qt::WindowFlags(), 0.1);
      if (!ok)
	return false;
      Raw2Pvl::m_vdb_resample = resample;
    }
  else
    resample = Raw2Pvl::m_vdb_resample;


  uchar voxelType = volData->voxelType();  
  if (voxelType > _Float)
    {
      QMessageBox::warning(0, "Save VDB",
                           "VDB conversion supports scalar volumes only.");
      return false;
    }

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  std::uint64_t rawPixels = 0;
  std::uint64_t rawBytes = 0;
  std::uint64_t sourceVoxels = 0;
  std::uint64_t resampledVoxels = 0;
  std::uint64_t vdbWorkingBytes = 0;
  std::uint64_t allocationBytes = 0;
  std::shared_ptr<ProcessMemoryReservation> vdbReservation;
  const double safeResample = static_cast<double>(resample);
  const std::uint64_t rd = safeResample > 0.0 ?
    static_cast<std::uint64_t>(std::ceil(
      static_cast<double>(dsz)/safeResample)) : 0;
  const std::uint64_t rw = safeResample > 0.0 ?
    static_cast<std::uint64_t>(std::ceil(
      static_cast<double>(wsz)/safeResample)) : 0;
  const std::uint64_t rh = safeResample > 0.0 ?
    static_cast<std::uint64_t>(std::ceil(
      static_cast<double>(hsz)/safeResample)) : 0;
  QString bufferError;
  if (!checkedPlaneLayout(wsz, hsz, bpv, rawPixels, rawBytes) ||
      rd == 0 || rw == 0 || rh == 0 ||
      !checkedImportMultiply(static_cast<std::uint64_t>(dsz),
                             static_cast<std::uint64_t>(wsz), sourceVoxels) ||
      !checkedImportMultiply(sourceVoxels,
                             static_cast<std::uint64_t>(hsz), sourceVoxels) ||
      !checkedImportMultiply(rd, rw, resampledVoxels) ||
      !checkedImportMultiply(resampledVoxels, rh, resampledVoxels) ||
      !checkedImportAdd(sourceVoxels, resampledVoxels, vdbWorkingBytes) ||
      !checkedImportMultiply(vdbWorkingBytes, 32, vdbWorkingBytes) ||
      !addImportBytes(rawBytes, allocationBytes) ||
      !addImportBytes(vdbWorkingBytes, allocationBytes) ||
      !admitImportBuffers(QStringLiteral("VDB conversion"),
                          allocationBytes, bufferError,
                          vdbReservation))
    {
      if (bufferError.isEmpty())
        bufferError = QStringLiteral(
          "VDB conversion was stopped because its worst-case working-set "
          "calculation overflowed.");
      QMessageBox::warning(0, "Save VDB", bufferError);
      return false;
    }

  std::unique_ptr<uchar[]> rawStorage;
  if (!allocateImportArray(rawBytes, rawStorage))
    {
      QMessageBox::warning(0, "Save VDB",
                           "VDB conversion could not allocate its admitted "
                           "slice buffer.");
      return false;
    }
  uchar *raw = rawStorage.get();

  std::unique_ptr<VdbVolume> vdb;
  try
    {
      vdb.reset(new VdbVolume);
    }
  catch (const std::exception& error)
    {
      QMessageBox::critical(0, "Save VDB",
	QString("Cannot initialize OpenVDB output: %1")
	.arg(QString::fromLocal8Bit(error.what())));
      return false;
    }
  catch (...)
    {
      QMessageBox::critical(0, "Save VDB",
	"Cannot initialize OpenVDB output: unknown OpenVDB error.");
      return false;
    }
  
  
  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }
  
  QProgressDialog progress("Saving "+vdbFileName,
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());

  for(int d = 0; d<dsz; d++)
    {
      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  return false;
	}
      
      progress.setValue((int)(100*(float)d/(float)dsz));
      qApp->processEvents();
      
	QString sliceError;
	if (!readExportSlice(volData, d, raw, sliceError))
	{
	  QMessageBox::critical(0, "Save VDB",
	    QString("Cannot decode input slice %1: %2")
	    .arg(d).arg(sliceError));
	  return false;
	}

      try
	{
	  if (voxelType == _UChar)
	    vdb->addSliceToVDB(reinterpret_cast<uchar*>(raw),
			       d, wsz, hsz,
			       bType, bValue1, bValue2);
	  else if (voxelType == _Char)
	    vdb->addSliceToVDB(reinterpret_cast<char*>(raw),
			       d, wsz, hsz,
			       bType, bValue1, bValue2);
	  else if (voxelType == _UShort)
	    vdb->addSliceToVDB(reinterpret_cast<unsigned short*>(raw),
			       d, wsz, hsz,
			       bType, bValue1, bValue2);
	  else if (voxelType == _Short)
	    vdb->addSliceToVDB(reinterpret_cast<short*>(raw),
			       d, wsz, hsz,
			       bType, bValue1, bValue2);
	  else if (voxelType == _Int)
	    vdb->addSliceToVDB(reinterpret_cast<int*>(raw),
			       d, wsz, hsz,
			       bType, bValue1, bValue2);
	  else if (voxelType == _Float)
	    vdb->addSliceToVDB(reinterpret_cast<float*>(raw),
			       d, wsz, hsz,
			       bType, bValue1, bValue2);
	  else
	    {
	      QMessageBox::critical(0, "Save VDB",
		"The source voxel type cannot be written to OpenVDB.");
	      return false;
	    }
	}
      catch (const std::exception& error)
	{
	  QMessageBox::critical(0, "Save VDB",
	    QString("OpenVDB failed while adding slice %1: %2")
	    .arg(d).arg(QString::fromLocal8Bit(error.what())));
	  return false;
	}
      catch (...)
	{
	  QMessageBox::critical(0, "Save VDB",
	    QString("OpenVDB failed while adding slice %1: unknown error.")
	    .arg(d));
	  return false;
	}
    }

  if (progress.wasCanceled())
    {
      progress.setValue(100);
      QMessageBox::information(0, "Save", "-----Aborted-----");
      return false;
    }

  if (qAbs(resample-1.0)>0.001)
    {
      try
	{
	  vdb->resample(resample);
	}
      catch (const std::exception& error)
	{
	  QMessageBox::critical(0, "Save VDB",
	    QString("OpenVDB resampling failed: %1")
	    .arg(QString::fromLocal8Bit(error.what())));
	  return false;
	}
      catch (...)
	{
	  QMessageBox::critical(0, "Save VDB",
	    "OpenVDB resampling failed: unknown error.");
	  return false;
	}
    }

  
  progress.setLabelText("Writing to disk - " + vdbFileName); 
  progress.setValue(50);
  qApp->processEvents();

  if (progress.wasCanceled())
    {
      progress.setValue(100);
      QMessageBox::information(0, "Save", "-----Aborted-----");
      return false;
    }

  const QString targetFile = QFileInfo(vdbFileName).absoluteFilePath();
  QTemporaryFile temporaryVdb(
    QDir(QFileInfo(targetFile).absolutePath()).filePath(
      ".drishti-vdb-XXXXXX.vdb"));
  temporaryVdb.setAutoRemove(true);
  if (!temporaryVdb.open())
    {
      QMessageBox::critical(0, "Save VDB",
	QString("Cannot create a temporary VDB output beside '%1': %2")
	.arg(targetFile, temporaryVdb.errorString()));
      return false;
    }
  const QString temporaryFile = temporaryVdb.fileName();
  temporaryVdb.close();

  try
    {
      vdb->save(temporaryFile);
    }
  catch (const std::exception& error)
    {
      QMessageBox::critical(0, "Save VDB",
	QString("Cannot write VDB output '%1': %2")
	.arg(targetFile, QString::fromLocal8Bit(error.what())));
      return false;
    }
  catch (...)
    {
      QMessageBox::critical(0, "Save VDB",
	QString("Cannot write VDB output '%1': unknown OpenVDB error.")
	.arg(targetFile));
      return false;
    }
  const QFileInfo output(temporaryFile);
  if (!output.exists() || !output.isFile() || output.size() <= 0)
    {
      QMessageBox::critical(0, "Save VDB",
	QString("VDB output was not created or is empty: %1")
	.arg(targetFile));
      return false;
    }

  const bool hadTarget = QFileInfo::exists(targetFile);
  const QString backupFile = targetFile + ".drishti-backup-" +
    QUuid::createUuid().toString(QUuid::WithoutBraces);
  if (hadTarget && !QFile::rename(targetFile, backupFile))
    {
      QMessageBox::critical(0, "Save VDB",
	QString("Cannot preserve the existing VDB output '%1'.")
	.arg(targetFile));
      return false;
    }
  if (!QFile::rename(temporaryFile, targetFile))
    {
      const bool restored = !hadTarget || QFile::rename(backupFile, targetFile);
      QMessageBox::critical(0, "Save VDB",
	restored ?
	  QString("Cannot commit VDB output '%1'; the previous file was restored.")
	    .arg(targetFile) :
	  QString("Cannot commit VDB output '%1', and the previous file remains "
	          "at '%2'.").arg(targetFile, backupFile));
      return false;
    }
  temporaryVdb.setAutoRemove(false);
  if (hadTarget && !QFile::remove(backupFile))
    QMessageBox::warning(0, "Save VDB",
	QString("The VDB output is complete, but the previous-file backup could "
	        "not be removed: %1").arg(backupFile));
  
  progress.setValue(100);

  return true;
}





void
Raw2Pvl::saveIsosurface(VolumeData* volData,
			int dmin, int dmax,
			int wmin, int wmax,
			int hmin, int hmax,
			QStringList timeseriesFiles)
{
  bool ok;
  QString meshFilename = QFileDialog::getSaveFileName(0,
						     "Export Mesh to file",
						      Global::previousDirectory(),
						      "Surface Mesh (*.ply *.obj *.stl) ;; Tetrahedral Mesh (*.msh)");
  if (meshFilename.isEmpty())
    {
      QMessageBox::information(0, "Error", "No OBJ filename specified");
      return;
    }
  
  
  if (!StaticFunctions::checkExtension(meshFilename, ".ply") &&
      !StaticFunctions::checkExtension(meshFilename, ".obj") &&
      !StaticFunctions::checkExtension(meshFilename, ".stl") &&
      !StaticFunctions::checkExtension(meshFilename, ".msh"))
    meshFilename += ".ply";

  
  bool tetMesh = false;
  if (StaticFunctions::checkExtension(meshFilename, ".msh"))
    tetMesh = true;    

  bool choiceAccepted = false;
  bool save0AtTop = saveSliceZeroAtTop(&choiceAccepted);
  if (!choiceAccepted)
    return;
  
  int ivType = -2;
  int bType = -2;
  float isoValue, bValue1, bValue2;
  float adaptivity;
  float resample;
  int dataSmooth;
  int meshSmooth;
  int morphoType;
  int morphoRadius;
  QColor meshColor;
  bool applyVoxelScaling;
  if (!getValues(ivType, isoValue,
		 bType, bValue1, bValue2,
		 adaptivity, resample,
		 morphoType, morphoRadius,
		 dataSmooth, meshSmooth,
		 meshColor, applyVoxelScaling,
		 tetMesh))
    return;
  // return if the parameters are not correct



  // identify voxelType and set how many bytes per voxel to read
  uchar voxelType = volData->voxelType();  
  if (voxelType > _Float)
    {
      QMessageBox::warning(0, "Export Mesh",
                           "Isosurface generation supports scalar volumes only.");
      return;
    }

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;


  int rvdepth, rvwidth, rvheight;
  volData->gridSize(rvdepth, rvwidth, rvheight);
  if (!validVolumeRange(rvdepth, rvwidth, rvheight,
                        dmin, dmax, wmin, wmax, hmin, hmax))
    {
      QMessageBox::warning(0, "Export Mesh",
                           "The selected volume range is invalid.");
      return;
    }



  if (bType == 10)
    {
      if (voxelType != _Float)
	Raw2Pvl::saveIsosurfaceRange(volData,
				     dmin, dmax,
				     wmin, wmax,
				     hmin, hmax,
				     timeseriesFiles,
				     meshFilename,
				     save0AtTop,
				     bValue1, bValue2,
				     adaptivity, resample,
				     dataSmooth, meshSmooth,
				     morphoType, morphoRadius,
				     meshColor,
				     applyVoxelScaling);
      else
	QMessageBox::information(0, "Error",
				 "Isosurfaces not generated.\nIsosurface over range of values only works for non FLOAT datatypes");
      
      return;
    }
  

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  
  IsosurfaceBuffers isosurfaceBuffers;
  QString bufferError;
  if (!prepareIsosurfaceBuffers(
        QStringLiteral("Isosurface generation"),
        rvwidth, rvheight, bpv,
        dsz, wsz, hsz, resample,
        isosurfaceBuffers, bufferError))
    {
      QMessageBox::warning(0, "Export Mesh", bufferError);
      return;
    }
  uchar *raw = isosurfaceBuffers.raw.get();
  float *val = isosurfaceBuffers.values.get();

  bool trim = (dmin != 0 ||
	       wmin != 0 ||
	       hmin != 0 ||
	       dsz != rvdepth ||
	       wsz != rvwidth ||
	       hsz != rvheight);


  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }

  QProgressDialog progress("Exporting Mesh",
			   "Cancel",
			   0, 100,
			   mainWidget,
			   Qt::Dialog|Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.resize(500, 100);
  progress.move(QCursor::pos());

  int tsfcount = qMax(1, timeseriesFiles.count());
  QChar fillChar = '0';
  int fieldWidth = 2;
  fieldWidth = tsfcount/10+2;
  for (int tsf=0; tsf<tsfcount; tsf++)
    {
      VdbVolume vdb;

      QString meshflnm;
      if (tsfcount == 1)
	meshflnm = meshFilename;
      else
	meshflnm = meshFilename.chopped(4) +
	           QString("_%1").arg((int)tsf, fieldWidth, 10, fillChar) +
	           meshFilename.right(4);
      
      if (progress.wasCanceled())
	{
	  progress.setValue(100);  
	  QMessageBox::information(0, "Save", "-----Aborted-----");
	  return;
	}
      
      if (tsfcount > 1)
	{
	  if (!volData->replaceFile(timeseriesFiles[tsf]))
	    {
	      QMessageBox::warning(0, "Export Mesh", volData->lastError());
	      return;
	    }
	  if (!currentVolumeLayoutMatches(volData,
	                                  rvdepth, rvwidth, rvheight,
	                                  voxelType))
	    {
	      QMessageBox::warning(0, "Export Mesh",
	        "A time-series volume has a different grid or voxel type. "
	        "Mesh generation was stopped before decoding it.");
	      return;
	    }
	}

      for(int d=dmin; d<=dmax; d++)
	{
	  
	  if (progress.wasCanceled())
	    {
	      progress.setValue(100);  
	      QMessageBox::information(0, "Save", "-----Aborted-----");
	      return;
	    }
	  	  
	  progress.setValue((int)(100*(float)d/(float)dsz));
	  qApp->processEvents();
	  
	  QString sliceError;
	  if (!readExportSlice(volData, d, raw, sliceError))
	    {
	      QMessageBox::critical(0, "Export Mesh",
		QString("Cannot decode input slice %1: %2")
		.arg(d).arg(sliceError));
	      return;
	    }
	  int vi = 0;
	  for(int w=wmin; w<=wmax; w++)
	    {
	      for(int h=hmin; h<=hmax; h++)
		{
		  if (voxelType == _UChar)
		    {
		      uchar *ptr = raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  else if (voxelType == _Char)
		    {
		      char *ptr = (char*)raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  else if (voxelType == _UShort)
		    {
		      ushort *ptr = (ushort*)raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  else if (voxelType == _Short)
		    {
		      short *ptr = (short*)raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  else if (voxelType == _Int)
		    {
		      int *ptr = (int*)raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  else if (voxelType == _Float)
		    {
		      float *ptr = (float*)raw;
		      val[vi] = ptr[w*rvheight+h];
		    }
		  vi++;
		} // loop i
	    } // loop j

	  if (save0AtTop)
	    vdb.addSliceToVDB(val,
			      rvdepth-1-d, wsz, hsz,
			      bType, bValue1, bValue2);
	  else
	    vdb.addSliceToVDB(val,
			      d, wsz, hsz,
			      bType, bValue1, bValue2);
	} // loop dd - slices

      
      // resample is required
      if (qAbs(resample-1.0) > 0.001)
	{
	  progress.setLabelText("Downsampling Voxel Volume");
	  progress.setValue(80);  
	  qApp->processEvents();
	  vdb.resample(resample);
	}

      
      // convert to level set
      progress.setLabelText("Converting to levelset");
      progress.setValue(50);  
      qApp->processEvents();
      vdb.convertToLevelSet(isoValue, ivType);
      
      
      // Apply Morphological Operations
      if (morphoType > 0 && morphoRadius > 0)
	{
	  float offset = morphoRadius;
	  if (morphoType == 1)
	    {
	      progress.setLabelText("Applying Morphological Dilation");
	      vdb.offset(-offset); // dilate
	    }
	  if (morphoType == 2)
	    {
	      progress.setLabelText("Applying Morphological Erosion");
	      vdb.offset(offset); // erode
	    }
	  if (morphoType == 3)
	    {
	      progress.setLabelText("Applying Morphological Closing");
	      vdb.offset(-offset); // dilate
	      vdb.offset(offset); // erode
	    }
	  if (morphoType == 4)
	    {
	      progress.setLabelText("Applying Morphological Opening");
	      vdb.offset(offset); // erode
	      vdb.offset(-offset); // dilate
	    }

	  progress.setValue(60);  
	  qApp->processEvents();
	  
	}
            
      
      // smoothing if required  
      if (dataSmooth > 0)
	{
	  progress.setLabelText("Smoothing Voxel Volume");
	  progress.setValue(70);  
	  qApp->processEvents();
	  vdb.mean(0.1, dataSmooth); // width, iterations
	}


      
      Global::statusBar()->showMessage("Generating Mesh");
      qApp->processEvents();
      
      progress.setLabelText("Generating Isosurface Mesh");
      progress.setValue(90);  
      QVector<QVector3D> V;
      QVector<QVector3D> VN;
      QVector<int> T;
      if (!tetMesh)
	vdb.generateMesh(0, 0, adaptivity, V, VN, T);
      else
	vdb.generateMesh(0, 0, 0, V, VN, T);

      progress.setLabelText("Saving Mesh to "+QFileInfo(meshflnm).fileName());
      Global::statusBar()->showMessage("Saving Mesh to "+QFileInfo(meshflnm).fileName());
      qApp->processEvents();
      
      if (applyVoxelScaling) // take voxel size into account
	{
	  float vx, vy, vz;
	  volData->voxelSize(vx, vy, vz);
	  for(int i=0; i<V.count(); i++)
	    V[i] *= QVector3D(vx, vy, vz);
	}      
      
      if (meshSmooth > 0)  
	MeshTools::smoothMesh(V, VN, T, 5*meshSmooth);

      bool meshSaved = false;
      if (tetMesh)
	{
	  meshSaved = MeshTools::saveToTetrahedralMesh(meshflnm, V, T);
	}
      else if (meshflnm.right(3).toLower() == "obj")
	{
	  QVector<QVector3D> C;
	  std::shared_ptr<ProcessMemoryReservation> meshReservation;
	  if (!prepareMeshColorBuffer(QStringLiteral("OBJ mesh export"),
	                              V.count(), C, bufferError,
	                              meshReservation))
	    {
	      QMessageBox::warning(0, "Export Mesh", bufferError);
	      return;
	    }
	  C.fill(QVector3D(meshColor.red(),
			   meshColor.green(),
			   meshColor.blue()));			   
	  meshSaved = MeshTools::saveToOBJ(meshflnm, V, VN, C, T);
	}
      else if (meshflnm.right(3).toLower() == "ply")
	{
	  QVector<QVector3D> C;
	  std::shared_ptr<ProcessMemoryReservation> meshReservation;
	  if (!prepareMeshColorBuffer(QStringLiteral("PLY mesh export"),
	                              V.count(), C, bufferError,
	                              meshReservation))
	    {
	      QMessageBox::warning(0, "Export Mesh", bufferError);
	      return;
	    }
	  C.fill(QVector3D(meshColor.red(),
			   meshColor.green(),
			   meshColor.blue()));			   
	  meshSaved = MeshTools::saveToPLY(meshflnm, V, VN, C, T);
	}
      else if (meshflnm.right(3).toLower() == "stl")
	meshSaved = MeshTools::saveToSTL(meshflnm, V, VN, T);

      if (!meshSaved)
	{
	  Global::statusBar()->clearMessage();
	  QMessageBox::critical(0, "Export Mesh",
	    QString("Mesh output could not be written completely: %1")
	      .arg(meshflnm));
	  return;
	}
      
      Global::statusBar()->clearMessage();
    } // loop timeseries

  progress.setValue(100);  
  QMessageBox::information(0, "Export Mesh", "Save Done");
}



bool
Raw2Pvl::parIsoGen(VolumeData* volData,
		   uchar voxelType,
		   int rvheight, int rvwidth, int rvdepth,
		   int dmin, int dmax,
		   int wmin, int wmax,
		   int hmin, int hmax,
		   bool save0AtTop,
		   int iso,
		   QString meshflnm,
		   float adaptivity,
		   bool applyVoxelScaling,
		   int dataSmooth, int meshSmooth,
		   int morphoType, float morphoRadius,
		   float resample,
		   bool showProgress)
{
  QProgressDialog progress;
  if (!showProgress)
    progress.close();
  else
    {
      progress.setLabelText("Isosurface generation");
      progress.setRange(0, 100);
      progress.setWindowFlags(Qt::Dialog|Qt::WindowStaysOnTopHint);
      progress.setMinimumDuration(0);
    }



  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;
  
  if (voxelType > _Float)
    {
      qWarning() << "Isosurface generation rejected a non-scalar volume";
      return false;
    }

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  
  if (!validVolumeRange(rvdepth, rvwidth, rvheight,
                        dmin, dmax, wmin, wmax, hmin, hmax))
    {
      qWarning() << "Isosurface generation rejected an invalid volume range";
      return false;
    }

  IsosurfaceBuffers isosurfaceBuffers;
  QString bufferError;
  if (!prepareIsosurfaceBuffers(
        QStringLiteral("Isosurface generation"),
        rvwidth, rvheight, bpv,
        dsz, wsz, hsz, resample,
        isosurfaceBuffers, bufferError))
    {
      if (showProgress)
        QMessageBox::warning(0, "Export Mesh", bufferError);
      else
        qWarning() << bufferError;
      return false;
    }
  uchar *raw = isosurfaceBuffers.raw.get();
  float *val = isosurfaceBuffers.values.get();

  VdbVolume vdb;

  
  //------------------------------------
  for(int d=dmin; d<=dmax; d++)
    {      
      if (showProgress)
	{
	  progress.setValue((int)(100*(float)(d-dmin)/(float)(dmax+1-dmin)));
	  qApp->processEvents();
	}
	  
      QString sliceError;
      if (!readExportSlice(volData, d, raw, sliceError))
	{
	  const QString error = QString("Cannot decode input slice %1: %2")
	    .arg(d).arg(sliceError);
	  if (showProgress)
	    QMessageBox::critical(0, "Export Mesh", error);
	  else
	    qWarning() << error;
	  return false;
	}
      int vi = 0;
      for(int w=wmin; w<=wmax; w++)
	{
	  for(int h=hmin; h<=hmax; h++)
	    {
	      if (voxelType == _UChar)
		{
		  uchar *ptr = raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      else if (voxelType == _Char)
		{
		  char *ptr = (char*)raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      else if (voxelType == _UShort)
		{
		  ushort *ptr = (ushort*)raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      else if (voxelType == _Short)
		{
		  short *ptr = (short*)raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      else if (voxelType == _Int)
		{
		  int *ptr = (int*)raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      else if (voxelType == _Float)
		{
		  float *ptr = (float*)raw;
		  val[vi] = ptr[w*rvheight+h];
		}
	      vi++;
	    } // loop h
	} // loop w
      
      if (save0AtTop)
	vdb.addSliceToVDB(val,
			  rvdepth-1-d, wsz, hsz,
			  4, iso, 0);
      else
	vdb.addSliceToVDB(val,
			  d, wsz, hsz,
			  4, iso, 0);
      
    } // loop d - slices
  //------------------------------------


  //------------------------------------
  // create mesh filename
  QString iso_meshflnm;
  QChar fillChar = '0';
  iso_meshflnm = meshflnm.chopped(4) +
    QString("_%1").arg((int)iso, 5, 10, fillChar) +
    meshflnm.right(4);
  //------------------------------------
  

  if (showProgress)
    {
      progress.setLabelText("Downsampling Voxel Volume");
      progress.setValue(50);
      qApp->processEvents();
    }
  
  
  //------------------------------------
  // resample is required
  if (qAbs(resample-1.0) > 0.001)
    vdb.resample(resample);
  //------------------------------------
 
 
  if (showProgress)
    {
      progress.setLabelText("Converting to levelset");
      progress.setValue(60);
      qApp->processEvents();
    }
  

  //------------------------------------
  // convert to level set
  vdb.convertToLevelSet(iso, 0);
  //------------------------------------
	  

  if (showProgress)
    {
      progress.setLabelText("Applying Morphological Operation");
      progress.setValue(70);
      qApp->processEvents();
    }
  

  //------------------------------------
  // Apply Morphological Operations
  if (morphoType > 0 && morphoRadius > 0)
    {
      float offset = morphoRadius;
      if (morphoType == 1) vdb.offset(-offset); // dilate
      else if (morphoType == 2) vdb.offset(offset); // erode
      else if (morphoType == 3)
	{
	  vdb.offset(-offset); // dilate
	  vdb.offset(offset); // erode
	}
      else if (morphoType == 4)
	{
	  vdb.offset(offset); // erode
	  vdb.offset(-offset); // dilate
	}
    }
  //------------------------------------


  if (showProgress)
    {
      progress.setLabelText("Smoothing Voxel Volume");
      progress.setValue(80);
      qApp->processEvents();
    }
  
  
  //------------------------------------
  // smoothing if required  
  if (dataSmooth > 0)
    vdb.mean(0.1, dataSmooth); // width, iterations
  //------------------------------------


  
  //------------------------------------
  // color, smooth and save mesh
  QVector<QVector3D> V;
  QVector<QVector3D> VN;
  QVector<int> T;
  vdb.generateMesh(0, 0, adaptivity, V, VN, T);

  // don't generate file if no vertices found
  if (V.count() == 0)
    return false;
  

  // take voxel size into account
  if (applyVoxelScaling)
    {
      float vx, vy, vz;
      volData->voxelSize(vx, vy, vz);
      for(int i=0; i<V.count(); i++)
	V[i] *= QVector3D(vx, vy, vz);
    }      

  // smoothing
  if (meshSmooth > 0)  
    MeshTools::smoothMesh(V, VN, T, 5*meshSmooth);

  // default color
  QColor meshColor = QColor(Qt::white);

 
  //------------------------------------
  // save mesh

  if (showProgress)
    {
      progress.setLabelText("Saving Mesh to "+QFileInfo(iso_meshflnm).fileName());
      progress.setValue(90);
      qApp->processEvents();
    }
  
  bool meshSaved = false;
  if (iso_meshflnm.right(3).toLower() == "obj")
    {
      QVector<QVector3D> C;
      std::shared_ptr<ProcessMemoryReservation> meshReservation;
      if (!prepareMeshColorBuffer(QStringLiteral("OBJ mesh export"),
                                  V.count(), C, bufferError,
                                  meshReservation))
        {
          if (showProgress)
            QMessageBox::warning(0, "Export Mesh", bufferError);
          else
            qWarning() << bufferError;
          return false;
        }
      C.fill(QVector3D(meshColor.red(),
		       meshColor.green(),
		       meshColor.blue()));			   
      meshSaved = MeshTools::saveToOBJ(iso_meshflnm, V, VN, C, T, false);
    }
  else if (iso_meshflnm.right(3).toLower() == "ply")
    {
      QVector<QVector3D> C;
      std::shared_ptr<ProcessMemoryReservation> meshReservation;
      if (!prepareMeshColorBuffer(QStringLiteral("PLY mesh export"),
                                  V.count(), C, bufferError,
                                  meshReservation))
        {
          if (showProgress)
            QMessageBox::warning(0, "Export Mesh", bufferError);
          else
            qWarning() << bufferError;
          return false;
        }
      C.fill(QVector3D(meshColor.red(),
		       meshColor.green(),
		       meshColor.blue()));			   
      meshSaved = MeshTools::saveToPLY(iso_meshflnm, V, VN, C, T, false);
    }
  else if (iso_meshflnm.right(3).toLower() == "stl")
    meshSaved = MeshTools::saveToSTL(iso_meshflnm, V, VN, T, false);
  //------------------------------------

  if (!meshSaved)
    {
      const QString message =
        QString("Mesh output could not be written completely: %1")
          .arg(iso_meshflnm);
      if (showProgress)
        QMessageBox::critical(0, "Export Mesh", message);
      else
        qWarning() << message;
      return false;
    }


  if (showProgress)
    {
      progress.setValue(100);
      qApp->processEvents();
    }

  return true;
}

void
Raw2Pvl::mapIsoGen(QList<QVariant> plist)
{
  VolumeData* volData = static_cast<VolumeData*>(plist[0].value<void*>());
  uchar voxelType = plist[1].toInt();
  int rvheight = plist[2].toInt();
  int rvwidth = plist[3].toInt();
  int rvdepth = plist[4].toInt();
  int dmin = plist[5].toInt();
  int dmax = plist[6].toInt();
  int wmin = plist[7].toInt();
  int wmax = plist[8].toInt();
  int hmin = plist[9].toInt();
  int hmax = plist[10].toInt();
  bool save0AtTop = plist[11].toBool();
  int iso = plist[12].toInt();
  QString meshflnm = plist[13].toString();
  float adaptivity = plist[14].toFloat();
  bool applyVoxelScaling = plist[15].toBool();
  int dataSmooth = plist[16].toInt();
  int meshSmooth = plist[17].toInt();
  int morphoType = plist[18].toInt();
  float morphoRadius = plist[19].toFloat();
  float resample = plist[20].toFloat();
  
  Raw2Pvl::parIsoGen(volData,
		     voxelType,
		     rvheight, rvwidth, rvdepth,
		     dmin, dmax,
		     wmin, wmax,
		     hmin, hmax,
		     save0AtTop,
		     iso,
		     meshflnm,
		     adaptivity,
		     applyVoxelScaling,
		     dataSmooth, meshSmooth,
		     morphoType, morphoRadius,
		     resample,
		     false);			     
}

void
Raw2Pvl::saveIsosurfaceRange(VolumeData* volData,
			     int dmin, int dmax,
			     int wmin, int wmax,
			     int hmin, int hmax,
			     QStringList timeseriesFiles,
			     QString meshFilename,
			     bool save0AtTop,
			     float bValue1, float bValue2,
			     float adaptivity, float resample,
			     int dataSmooth, int meshSmooth,
			     int morphoType, int morphoRadius,
			     QColor meshColor,
			     bool applyVoxelScaling)
{
  //--------------------
  QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
  QWidget *mainWidget = 0;
  for(QWidget *w : topLevelWidgets)
    {
      if (w->isWindow())
	{
	  mainWidget = w;
	  break;
	}
    }
  //--------------------

  
  int rvdepth, rvwidth, rvheight;
  volData->gridSize(rvdepth, rvwidth, rvheight);

  if (!validVolumeRange(rvdepth, rvwidth, rvheight,
                        dmin, dmax, wmin, wmax, hmin, hmax))
    {
      QMessageBox::warning(0, "Export Mesh Range",
                           "The selected volume range is invalid.");
      return;
    }

  int dsz=dmax-dmin+1;
  int wsz=wmax-wmin+1;
  int hsz=hmax-hmin+1;

  
  uchar voxelType = volData->voxelType();  
  if (voxelType > _Float)
    {
      QMessageBox::warning(0, "Export Mesh Range",
                           "Isosurface generation supports scalar volumes only.");
      return;
    }

  int bpv = 1;
  if (voxelType == _UChar) bpv = 1;
  else if (voxelType == _Char) bpv = 1;
  else if (voxelType == _UShort) bpv = 2;
  else if (voxelType == _Short) bpv = 2;
  else if (voxelType == _Int) bpv = 4;
  else if (voxelType == _Float) bpv = 4;

  bool trim = (dmin != 0 ||
	       wmin != 0 ||
	       hmin != 0 ||
	       dsz != rvdepth ||
	       wsz != rvwidth ||
	       hsz != rvheight);


  int tsfcount = qMax(1, timeseriesFiles.count());
  QChar fillChar = '0';
  int fieldWidth = 2;
  fieldWidth = tsfcount/10+2;
  for (int tsf=0; tsf<tsfcount; tsf++)
    {
      QString meshflnm;
      if (tsfcount == 1)
	meshflnm = meshFilename;
      else
	meshflnm = meshFilename.chopped(4) +
	           QString("_%1").arg((int)tsf, fieldWidth, 10, fillChar) +
	           meshFilename.right(4);
      
      if (tsfcount > 1)
	{
	  if (!volData->replaceFile(timeseriesFiles[tsf]))
	    {
	      QMessageBox::warning(0, "Export Mesh Range", volData->lastError());
	      return;
	    }
	  if (!currentVolumeLayoutMatches(volData,
	                                  rvdepth, rvwidth, rvheight,
	                                  voxelType))
	    {
	      QMessageBox::warning(0, "Export Mesh Range",
	        "A time-series volume has a different grid or voxel type. "
	        "Range generation was stopped before decoding it.");
	      return;
	    }
	}


      if (checkParIsoGen())
	{
	  //-----------------------------------------------
	  // parallel isosurface generation
	  //-----------------------------------------------

	  // create parameter list to be sent to the parallel iso surface generation routine
	  QList<QList<QVariant>> param;
	  for(int iso=(int)bValue1; iso<=(int)bValue2; iso++)
	    {
	      QList<QVariant> plist;
	      plist << QVariant::fromValue(static_cast<void*>(volData));
	      plist << QVariant((int)voxelType);
	      plist << QVariant(rvheight);
	      plist << QVariant(rvwidth);
	      plist << QVariant(rvdepth);	  
	      plist << QVariant(dmin);
	      plist << QVariant(dmax);
	      plist << QVariant(wmin);
	      plist << QVariant(wmax);
	      plist << QVariant(hmin);
	      plist << QVariant(hmax);
	      plist << QVariant(save0AtTop);
	      plist << QVariant(iso);
	      plist << QVariant(meshflnm);
	      plist << QVariant(adaptivity);
	      plist << QVariant(applyVoxelScaling);
	      plist << QVariant(dataSmooth);
	      plist << QVariant(meshSmooth);
	      plist << QVariant(morphoType);
	      plist << QVariant(morphoRadius);
	      plist << QVariant(resample);
	      
	      param << plist;
	    }
	  
	  
	  // Create a progress dialog.
	  QProgressDialog dialog;
	  dialog.setLabelText(QString("Exporting mesh using %1 thread(s)...").arg(QThread::idealThreadCount()));
	  
	  // Create a QFutureWatcher and connect signals and slots.
	  QFutureWatcher<void> futureWatcher;
	  QObject::connect(&futureWatcher, &QFutureWatcher<void>::finished, &dialog, &QProgressDialog::reset);
	  QObject::connect(&dialog, &QProgressDialog::canceled, &futureWatcher, &QFutureWatcher<void>::cancel);
	  QObject::connect(&futureWatcher,  &QFutureWatcher<void>::progressRangeChanged, &dialog, &QProgressDialog::setRange);
	  QObject::connect(&futureWatcher, &QFutureWatcher<void>::progressValueChanged,  &dialog, &QProgressDialog::setValue);
	  
	  // Start generation of isosurface for all values within the range
	  futureWatcher.setFuture(QtConcurrent::map(param, Raw2Pvl::mapIsoGen));
	  
	  // Display the dialog and start the event loop.
	  dialog.exec();
	  
	  futureWatcher.waitForFinished();
	  //-----------------------------------------------
	}
      else
	{
	  //-----------------------------------------------
	  // sequential isosurface generation	  
	  //-----------------------------------------------

	  QProgressDialog progress("Exporting Mesh",
				   "Cancel",
				   0, 100,
				   mainWidget,
				   Qt::Dialog|Qt::WindowStaysOnTopHint);
	  progress.setMinimumDuration(0);
	  progress.resize(500, 100);
	  progress.move(QCursor::pos());
	  for(int iso=(int)bValue1; iso<=(int)bValue2; iso++)
	    {	     	  	  
	      progress.setValue((int)(100*(float)(iso-bValue1)/(float)(bValue2+1-bValue1)));
	      qApp->processEvents();

	      if (!Raw2Pvl::parIsoGen(volData,
				 voxelType,
				 rvheight, rvwidth, rvdepth,
				 dmin, dmax,
				 wmin, wmax,
				 hmin, hmax,
				 save0AtTop,
				 iso,
				 meshflnm,
				 adaptivity,
				 applyVoxelScaling,
				 dataSmooth, meshSmooth,
				 morphoType, morphoRadius,
				 resample,
				 true))
		return;
	      if (progress.wasCanceled())
		{
		  progress.setValue(100);  
		  QMessageBox::information(0, "Save", "-----Aborted-----");
		  return;
		} 
	    }
	}
      
    } // loop timeseries
      
  QMessageBox::information(mainWidget, "Export Mesh", "Save Done");
}



bool
Raw2Pvl::getValues(int& ivType, float& isoValue,
		   int& bType, float& bValue1, float& bValue2,
		   float& adaptivity, float& resample,
		   int& morphoType, int& morphoRadius,
		   int& dataSmooth, int& meshSmooth,
		   QColor &color, bool& applyVoxelScaling,
		   bool tetMesh)
{
  isoValue = 0;
  adaptivity = 0.1;
  dataSmooth = 0;
  meshSmooth = 0;
  resample = 1.0;
  morphoType = 0;
  morphoRadius = 0;
  applyVoxelScaling = true;
  color = QColor(Qt::white);
  
  QString text("0");
  QString btext("<0");
  
  PropertyEditor propertyEditor;
  QMap<QString, QVariantList> plist;
  
  QVariantList vlist;

  vlist.clear();
  vlist << QVariant("string");
  vlist << btext;
  plist["background value"] = vlist;
  
  vlist.clear();
  vlist << QVariant("string");
  vlist << text;
  plist["isosurface value"] = vlist;
  
  if (!tetMesh)
    {
      vlist.clear();
      vlist << QVariant("float");
      vlist << QVariant(adaptivity);
      vlist << QVariant(0.0);
      vlist << QVariant(1.0);
      vlist << QVariant(0.01); // singlestep
      vlist << QVariant(3); // decimals
      plist["adaptivity"] = vlist;
    }
  
  vlist.clear();
  vlist << QVariant("float");
  vlist << QVariant(resample);
  vlist << QVariant(1.0);
  vlist << QVariant(10.0);
  vlist << QVariant(1); // singlestep
  vlist << QVariant(1); // decimals
  plist["downsample"] = vlist;
  
  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(dataSmooth);
  vlist << QVariant(0);
  vlist << QVariant(10);
  plist["smooth data"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(meshSmooth);
  vlist << QVariant(0);
  vlist << QVariant(10);
  plist["mesh smoothing"] = vlist;
  
  
  vlist.clear();
  vlist << QVariant("combobox");
  vlist << "0";
  vlist << "";
  vlist << "Dilate";
  vlist << "Erode";
  vlist << "Close";
  vlist << "Open";
  plist["morpho operator"] = vlist;

  vlist.clear();
  vlist << QVariant("int");
  vlist << QVariant(morphoRadius);
  vlist << QVariant(0);
  vlist << QVariant(100);
  plist["morpho radius"] = vlist;

  if (!tetMesh)
    {
      vlist.clear();
      vlist << QVariant("color");
      vlist << color;
      plist["color"] = vlist;
    }

  vlist.clear();
  vlist << QVariant("checkbox");
  vlist << QVariant(applyVoxelScaling);
  plist["apply voxel size"] = vlist;



  vlist.clear();
  QFile helpFile(":/mesh.help");
  if (helpFile.open(QFile::ReadOnly))
    {
      QTextStream in(&helpFile);
      QString line = in.readLine();
      while (!line.isNull())
	{
	  if (line == "#begin")
	    {
	      QString keyword = in.readLine();
	      QString helptext;
	      line = in.readLine();
	      while (!line.isNull())
		{
		  helptext += line;
		  helptext += "\n";
		  line = in.readLine();
		  if (line == "#end") break;
		}
	      vlist << keyword << helptext;
	    }
	  line = in.readLine();
	}
    }	      
  plist["commandhelp"] = vlist;
  

  QStringList keys;
  keys << "background value";
  keys << "isosurface value";
  if (!tetMesh)
    keys << "adaptivity";
  keys << "downsample";
  keys << "smooth data";
  keys << "mesh smoothing";
  keys << "morpho operator";
  keys << "morpho radius";
  if (!tetMesh)
    keys << "color";
  keys << "apply voxel size";
  keys << "commandhelp";
  //keys << "message";

  
  propertyEditor.set("Mesh Generation Parameters", plist, keys);
  propertyEditor.resize(700, 400);

  QMap<QString, QPair<QVariant, bool> > vmap;
  
  if (propertyEditor.exec() == QDialog::Accepted)
    vmap = propertyEditor.get();
  else
    return false;
  
  for(int ik=0; ik<keys.count(); ik++)
    {
      QPair<QVariant, bool> pair = vmap.value(keys[ik]);

      if (pair.second)
	{
	  if (keys[ik] == "background value")
	    btext = pair.first.toString();
	  else if (keys[ik] == "isosurface value")
	    text = pair.first.toString();
	  else if (keys[ik] == "adaptivity")
	    adaptivity = pair.first.toFloat();
	  else if (keys[ik] == "downsample")
	    resample = pair.first.toFloat();
	  else if (keys[ik] == "smooth data")
	    dataSmooth = pair.first.toInt();
	  else if (keys[ik] == "mesh smoothing")
	    meshSmooth = pair.first.toInt();
	  else if (keys[ik] == "morpho operator")
	    morphoType = pair.first.toInt();
	  else if (keys[ik] == "morpho radius")
	    morphoRadius = pair.first.toInt();
	  else if (keys[ik] == "color")
	    color = pair.first.value<QColor>();
	  else if (keys[ik] == "apply voxel size")
	    applyVoxelScaling = pair.first.toBool();
	}
    }


  
  //=========================
  bType = -2;  
  bValue1 = 0;
  bValue2 = 0;

  if (!btext.isEmpty())
    {
      QStringList list = btext.split(" ", QString::SkipEmptyParts);
      if (list.count() == 2)
	{
	  if (list[0].left(1) == ">" && list[1].left(1) == "<")
	    {
	      bType = 2;
	      bValue1 = list[0].mid(1).toFloat();
	      bValue2 = list[1].mid(1).toFloat();
	    }
	  else if (list[0].left(1) == "<" && list[1].left(1) == ">")
	    {
	      bType = 3;
	      bValue1 = list[0].mid(1).toFloat();
	      bValue2 = list[1].mid(1).toFloat();
	    }
	}
      else if (list.count() == 1)
	{
	  if (list[0].left(1) == "<")
	    {
	      bType = -1;
	      bValue1 = list[0].mid(1).toInt();
	    }
	  if (list[0].left(2) == "!=")
	    {
	      bType = 4;
	      bValue1 = list[0].mid(2).toInt();
	      //QMessageBox::information(0, "", QString("%1").arg(bValue1));
	    }
	  if (list[0].left(1) == ">")
	    {
	      bType = 1;
	      bValue1 = list[0].mid(1).toInt();
	    }
	}
    }

  if (bType == -2)
    {
      QMessageBox::information(0, "Background Value", QString("<Val,   !=Val,   >Val,   >Val1 <Val2,   <Val1 >Val2  expected.\nGot %1").arg(btext));
      return false;
    }
  //=========================

  
  //=========================
  ivType = -2;
  if (!text.isEmpty())
    {
      QStringList list = text.split("-", QString::SkipEmptyParts);
      if (list.count() == 2)
	{
	  ivType = 2; // isvalue range
	  bType = 10;
	  isoValue = list[0].toFloat();
	  bValue1 = list[0].toFloat();
	  bValue2 = list[1].toFloat();
	  QMessageBox::information(0, "", QString("Isosurface Value Range : %1 to %2").arg(bValue1).arg(bValue2));
	}
      else
	{
	  QStringList list = text.split(" ", QString::SkipEmptyParts);
	  if (list.count() == 1)
	    {
	      if (list[0].left(1) == "<")
		{
		  ivType = 1;
		  isoValue = list[0].mid(1).toFloat();
		}
	      else
		{
		  ivType = -1;
		  isoValue = list[0].toFloat();	      
		}
	    }
	  else if (list.count() == 2)
	    {
	      if (list[0] == "<")
		{
		  ivType = 1;
		  isoValue = list[1].toFloat();
		}
	    }
	}
    }

  if (ivType == -2)
    {
      QMessageBox::information(0, "Isosurface Value", QString("isoValue or <isoValue expected.\nGot %1").arg(text));
      return false;
    }


  return true;
}
