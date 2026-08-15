#include "global.h"
#include "volumebase.h"
#include "staticfunctions.h"
#include "volumefilemanager.h"
#include "mainwindowui.h"
#include "xmlheaderfunctions.h"
#include "../common/src/pvlmanifest.h"
#include "volumeinformation.h"

#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace
{
const char *c_defaultWindowTitle =
  "Drishti - Volume Exploration and Presentation Tool";

bool
checkedMultiply(size_t lhs, size_t rhs, size_t& result)
{
  if (lhs != 0 && rhs > std::numeric_limits<size_t>::max()/lhs)
    return false;

  result = lhs*rhs;
  return true;
}

bool
checkedVolumeSize(int height, int width, int depth, int bytesPerVoxel,
		  size_t& planeVoxels, size_t& volumeBytes)
{
  if (height <= 0 || width <= 0 || depth <= 0 || bytesPerVoxel <= 0)
    return false;

  size_t volumeVoxels = 0;
  return checkedMultiply(static_cast<size_t>(height),
			 static_cast<size_t>(width), planeVoxels) &&
    checkedMultiply(planeVoxels, static_cast<size_t>(depth), volumeVoxels) &&
    checkedMultiply(volumeVoxels, static_cast<size_t>(bytesPerVoxel),
		    volumeBytes);
}

bool
powerOfTwoExponent(int value, int& exponent)
{
  if (value <= 0)
    return false;

  qint64 power = 1;
  exponent = 0;
  while (power < static_cast<qint64>(value))
    {
      power *= 2;
      ++exponent;
    }

  return true;
}

bool
powerOfTwoValue(int exponent, int& value)
{
  if (exponent < 0 || exponent >= 31)
    return false;

  const quint32 power = static_cast<quint32>(1) << exponent;
  if (power > static_cast<quint32>(std::numeric_limits<int>::max()))
    return false;

  value = static_cast<int>(power);
  return true;
}

bool
supportedPvlVoxelType(int voxelType, int& bytesPerVoxel)
{
  if (voxelType == VolumeFileManager::_UChar)
    {
      bytesPerVoxel = 1;
      return true;
    }

  if (voxelType == VolumeFileManager::_UShort)
    {
      bytesPerVoxel = 2;
      return true;
    }

  return false;
}

void
restoreWindowTitle()
{
  Ui::MainWindow *ui = MainWindowUI::mainWindowUI();
  if (ui && ui->menubar && ui->menubar->parentWidget())
    ui->menubar->parentWidget()->setWindowTitle(c_defaultWindowTitle);
}

void
showVolumeError(const QString& error)
{
  QMessageBox::information(0, "Error", error);
}

class LowresProgress
{
 public:
  LowresProgress()
    : m_started(false),
      m_progressBar(0),
      m_window(0),
      m_statusBar(0)
  {}

  ~LowresProgress()
  {
    finish(false);
  }

  void start(const QString& title, const QString& status)
  {
    Ui::MainWindow *ui = MainWindowUI::mainWindowUI();
    if (ui && ui->menubar && ui->menubar->parentWidget())
      {
	m_window = ui->menubar->parentWidget();
	m_previousWindowTitle = m_window->windowTitle();
	m_window->setWindowTitle(title);
      }
    if (ui && ui->statusBar)
      {
	m_statusBar = ui->statusBar;
	m_previousStatusMessage = m_statusBar->currentMessage();
	m_statusBar->showMessage(status);
      }

    m_progressBar = Global::progressBar();
    m_progressBar->setValue(0);
    m_progressBar->show();
    m_started = true;
  }

  void setValue(int value)
  {
    if (m_progressBar)
      m_progressBar->setValue(qBound(0, value, 100));
  }

  void finish(bool success)
  {
    if (!m_started)
      return;

    if (m_statusBar)
      m_statusBar->showMessage(m_previousStatusMessage);
    if (m_progressBar)
      m_progressBar->setValue(success ? 100 : 0);
    Global::hideProgressBar();
    if (m_window)
      m_window->setWindowTitle(m_previousWindowTitle);
    if (qApp)
      qApp->processEvents();

    m_started = false;
  }

 private:
  bool m_started;
  QProgressBar *m_progressBar;
  QWidget *m_window;
  QStatusBar *m_statusBar;
  QString m_previousWindowTitle;
  QString m_previousStatusMessage;
};
}

int VolumeBase::pvlVoxelType() { return m_pvlVoxelType; }
Vec VolumeBase::getFullVolumeSize() { return m_fullVolumeSize; }
Vec VolumeBase::getLowresVolumeSize() { return m_lowresVolumeSize; }
Vec VolumeBase::getLowresTextureVolumeSize() { return m_lowresTextureVolumeSize; }
int VolumeBase::getLowresSubsamplingLevel() { return m_subSamplingLevel; }

int* VolumeBase::getLowres1dHistogram() { return m_1dHistogram; }
int* VolumeBase::getLowres2dHistogram() { return m_2dHistogram; }

unsigned char* VolumeBase::getLowresVolume() { return m_lowresVolume; }
unsigned char* VolumeBase::getLowresTextureVolume() { return m_lowresTextureVolume; }

VolumeBase::VolumeBase()
{
  m_pvlVoxelType = 0;
  m_depth = m_width = m_height = 0;
  m_fullVolumeSize = Vec(0, 0, 0);
  m_lowresVolumeSize = Vec(0, 0, 0);
  m_lowresTextureVolumeSize = Vec(0, 0, 0);
  m_subSamplingLevel = 1;
  m_1dHistogram = m_2dHistogram = 0;
  m_lowresVolume = m_lowresTextureVolume = 0;
}

VolumeBase::~VolumeBase()
{
  if (m_1dHistogram) delete [] m_1dHistogram;
  if (m_2dHistogram) delete [] m_2dHistogram;
  if (m_lowresVolume) delete [] m_lowresVolume;
  if (m_lowresTextureVolume) delete [] m_lowresTextureVolume;

  m_1dHistogram = m_2dHistogram = 0;
  m_lowresVolume = m_lowresTextureVolume = 0;
}

bool
VolumeBase::loadDummyVolume(int nx, int ny, int nz)
{
  if (nx <= 0 || ny <= 0 || nz <= 0)
    {
      showVolumeError(QString("Invalid dummy volume dimensions %1 x %2 x %3")
		      .arg(nx).arg(ny).arg(nz));
      return false;
    }

  int px2, py2, pz2;
  if (!powerOfTwoExponent(nz, px2) ||
      !powerOfTwoExponent(ny, py2) ||
      !powerOfTwoExponent(nx, pz2))
    {
      showVolumeError("Cannot calculate dummy volume dimensions");
      return false;
    }

  int subSamplingLevel = 1;
  while (px2+py2+pz2 > 22) // limit lowres volume size
    {
      if (subSamplingLevel > std::numeric_limits<int>::max()/2)
	{
	  showVolumeError("Dummy volume subsampling level is too large");
	  return false;
	}
      subSamplingLevel *= 2;

      const int lowHeight = qMax(1, nz/subSamplingLevel);
      const int lowWidth = qMax(1, ny/subSamplingLevel);
      const int lowDepth = qMax(1, nx/subSamplingLevel);
      if (!powerOfTwoExponent(lowHeight, px2) ||
	  !powerOfTwoExponent(lowWidth, py2) ||
	  !powerOfTwoExponent(lowDepth, pz2))
	{
	  showVolumeError("Cannot calculate dummy volume subsampling dimensions");
	  return false;
	}
    }

  int nsubX, nsubY, nsubZ;
  if (!powerOfTwoValue(px2, nsubX) ||
      !powerOfTwoValue(py2, nsubY) ||
      !powerOfTwoValue(pz2, nsubZ))
    {
      showVolumeError("Dummy volume texture dimensions are too large");
      return false;
    }

  std::unique_ptr<int[]> histogram1D(new(std::nothrow) int[256]);
  std::unique_ptr<int[]> histogram2D(new(std::nothrow) int[256*256]);
  if (!histogram1D || !histogram2D)
    {
      showVolumeError("Not enough memory for dummy volume histograms");
      return false;
    }
  std::memset(histogram1D.get(), 0, 256*sizeof(int));
  std::memset(histogram2D.get(), 0, 256*256*sizeof(int));

  delete [] m_1dHistogram;
  delete [] m_2dHistogram;
  delete [] m_lowresVolume;
  delete [] m_lowresTextureVolume;

  m_volumeFile.clear();
  m_pvlVoxelType = 0;
  m_depth = nx;
  m_width = ny;
  m_height = nz;
  m_fullVolumeSize = Vec(nz, ny, nx);
  m_subSamplingLevel = subSamplingLevel;
  m_lowresVolumeSize = Vec(qMax(1, nz/subSamplingLevel),
			   qMax(1, ny/subSamplingLevel),
			   qMax(1, nx/subSamplingLevel));
  m_lowresTextureVolumeSize = Vec(nsubX, nsubY, nsubZ);
  m_1dHistogram = histogram1D.release();
  m_2dHistogram = histogram2D.release();
  m_lowresVolume = 0;
  m_lowresTextureVolume = 0;

  return true;
}

bool
VolumeBase::loadVolume(const char* volfile, bool redo)
{
  if (!volfile || !volfile[0])
    {
      showVolumeError("No preprocessed volume file was specified");
      return false;
    }

  const QString volumeFile = QString::fromUtf8(volfile);

  PvlManifest manifest;
  if (!PvlManifestParser::parse(volumeFile, manifest, true))
    {
      showVolumeError(manifest.error);
      return false;
    }

  if (!VolumeInformation::xmlHeaderFile(volumeFile))
    {
      showVolumeError(
	QString("%1 is not a valid preprocessed volume file").arg(volumeFile));
      return false;
    }

  const int depth = manifest.depth;
  const int width = manifest.width;
  const int height = manifest.height;
  const int pvlVoxelType = manifest.voxelType;
  int bytesPerVoxel = 0;
  if (!supportedPvlVoxelType(pvlVoxelType, bytesPerVoxel))
    {
      showVolumeError(QString("%1 uses unsupported PVL voxel type %2; "
			      "only unsigned 8-bit and unsigned 16-bit data are supported")
		      .arg(volumeFile).arg(pvlVoxelType));
      return false;
    }

  size_t planeVoxels = 0;
  size_t volumeBytes = 0;
  if (!checkedVolumeSize(height, width, depth, bytesPerVoxel,
			 planeVoxels, volumeBytes))
    {
      showVolumeError(QString("%1 volume dimensions overflow addressable memory")
		      .arg(volumeFile));
      return false;
    }
  Q_UNUSED(planeVoxels);
  Q_UNUSED(volumeBytes);

  const QString oldVolumeFile = m_volumeFile;
  const int oldPvlVoxelType = m_pvlVoxelType;
  const int oldDepth = m_depth;
  const int oldWidth = m_width;
  const int oldHeight = m_height;
  const Vec oldFullVolumeSize = m_fullVolumeSize;
  const Vec oldLowresVolumeSize = m_lowresVolumeSize;
  const Vec oldLowresTextureVolumeSize = m_lowresTextureVolumeSize;
  const int oldSubSamplingLevel = m_subSamplingLevel;
  int *oldHistogram1D = m_1dHistogram;
  int *oldHistogram2D = m_2dHistogram;
  unsigned char *oldLowresVolume = m_lowresVolume;
  unsigned char *oldLowresTextureVolume = m_lowresTextureVolume;

  m_1dHistogram = 0;
  m_2dHistogram = 0;
  m_lowresVolume = 0;
  m_lowresTextureVolume = 0;
  m_volumeFile = volumeFile;
  m_pvlVoxelType = pvlVoxelType;
  m_depth = depth;
  m_width = width;
  m_height = height;
  m_fullVolumeSize = Vec(height, width, depth);

  if (!createLowresVolume(redo) || !createLowresTextureVolume())
    {
      delete [] m_1dHistogram;
      delete [] m_2dHistogram;
      delete [] m_lowresVolume;
      delete [] m_lowresTextureVolume;

      m_volumeFile = oldVolumeFile;
      m_pvlVoxelType = oldPvlVoxelType;
      m_depth = oldDepth;
      m_width = oldWidth;
      m_height = oldHeight;
      m_fullVolumeSize = oldFullVolumeSize;
      m_lowresVolumeSize = oldLowresVolumeSize;
      m_lowresTextureVolumeSize = oldLowresTextureVolumeSize;
      m_subSamplingLevel = oldSubSamplingLevel;
      m_1dHistogram = oldHistogram1D;
      m_2dHistogram = oldHistogram2D;
      m_lowresVolume = oldLowresVolume;
      m_lowresTextureVolume = oldLowresTextureVolume;
      return false;
    }

  delete [] oldHistogram1D;
  delete [] oldHistogram2D;
  delete [] oldLowresVolume;
  delete [] oldLowresTextureVolume;

  Global::setPvlVoxelType(m_pvlVoxelType);

  restoreWindowTitle();

  return true;
}

bool
VolumeBase::createLowresVolume(bool redo)
{
  Q_UNUSED(redo);

  if (Global::volumeType() == Global::DummyVolume)
    {      
      m_subSamplingLevel = 1;
      m_lowresVolumeSize = m_fullVolumeSize;
      return true;
    }

  int px2, py2, pz2;
  if (m_depth <= 0 || m_width <= 0 || m_height <= 0 ||
      !powerOfTwoExponent(m_height, px2) ||
      !powerOfTwoExponent(m_width, py2) ||
      !powerOfTwoExponent(m_depth, pz2))
    {
      showVolumeError("Cannot create a low-resolution volume from invalid dimensions");
      return false;
    }

  int lod = 1;

  int gtsz = 0;
  if (!powerOfTwoExponent(Global::maxArrayTextureLayers(), gtsz))
    {
      showVolumeError("Invalid OpenGL array texture layer limit");
      return false;
    }
  gtsz --; // be on the lower side
  if (px2 > gtsz || py2 > gtsz || pz2 > gtsz)
    {
      int maxp2, df;
      maxp2 = qMax(px2, qMax(py2, pz2));
      df = maxp2 - gtsz;  // 2^gtsz = Global::maxArrayTextureLayers()
      lod += df;
    }
  
  gtsz = qMin(26, Global::textureSize()-1);
  if ((px2+py2+pz2) - 3*(lod-1) > gtsz)
    {
      int df0, df;
      df0 = (px2+py2+pz2) - 3*(lod-1) - gtsz; 
      df = df0/3 + (df0%3 > 0);
      lod += df; 
    }

  int subSamplingLevel = 0;
  if (!powerOfTwoValue(lod-1, subSamplingLevel))
    {
      showVolumeError("Required low-resolution subsampling level is too large");
      return false;
    }

  // max hardware texture size is 512x512x512
  if ((px2 > 9 || py2 > 9 || pz2 > 9) &&
      subSamplingLevel == 1)
    subSamplingLevel = 2;

  const int height = qMax(1, m_height/subSamplingLevel);
  const int width = qMax(1, m_width/subSamplingLevel);
  const int depth = qMax(1, m_depth/subSamplingLevel);
  const Vec lowresVolumeSize(height, width, depth);

  int bpv = 0;
  if (!supportedPvlVoxelType(m_pvlVoxelType, bpv))
    {
      showVolumeError(QString("Unsupported PVL voxel type %1")
		      .arg(m_pvlVoxelType));
      return false;
    }

  VolumeFileManager pvlFileManager;
  PvlManifest manifest;
  if (!PvlManifestParser::parse(m_volumeFile, manifest, true))
    {
      showVolumeError(manifest.error);
      return false;
    }
  int slabSize = manifest.slabSize;
  int headerSize = manifest.headerSize;
  QStringList pvlnames = manifest.pvlNames;
  pvlFileManager.setFilenameList(pvlnames);
  pvlFileManager.setBaseFilename(m_volumeFile);
  pvlFileManager.setVoxelType(m_pvlVoxelType);
  pvlFileManager.setDepth(m_depth);
  pvlFileManager.setWidth(m_width);
  pvlFileManager.setHeight(m_height);
  pvlFileManager.setHeaderSize(headerSize); // default is 13 bytes
  pvlFileManager.setSlabSize(slabSize);
  if (!pvlFileManager.exists())
    {
      const QString detail = pvlFileManager.lastError();
      showVolumeError(detail.isEmpty() ?
		      QString("Cannot access volume data for %1").arg(m_volumeFile) :
		      detail);
      return false;
    }

  size_t lowresPlaneVoxels = 0;
  size_t lowresVolumeBytes = 0;
  if (!checkedVolumeSize(height, width, depth, bpv,
			 lowresPlaneVoxels, lowresVolumeBytes))
    {
      showVolumeError("Low-resolution volume dimensions overflow addressable memory");
      return false;
    }

  size_t sourcePlaneVoxels = 0;
  size_t sourceVolumeBytes = 0;
  if (!checkedVolumeSize(m_height, m_width, m_depth, bpv,
			 sourcePlaneVoxels, sourceVolumeBytes))
    {
      showVolumeError("Source volume dimensions overflow addressable memory");
      return false;
    }
  Q_UNUSED(sourceVolumeBytes);

  std::unique_ptr<unsigned char[]> lowresVolume(
    new(std::nothrow) unsigned char[lowresVolumeBytes]);
  if (!lowresVolume)
    {
      showVolumeError(QString("Not enough memory for the %1-byte low-resolution volume")
		      .arg(static_cast<qulonglong>(lowresVolumeBytes)));
      return false;
    }

  LowresProgress progress;
  progress.start("Generating Lowres Version", "Loading data for Lowres mode");

  size_t lowresPlaneBytes = 0;
  if (!checkedMultiply(lowresPlaneVoxels, static_cast<size_t>(bpv),
		       lowresPlaneBytes))
    {
      progress.finish(false);
      showVolumeError("Low-resolution slice dimensions overflow addressable memory");
      return false;
    }

  for (int kslc=0; kslc<depth; ++kslc)
    {
      const qint64 sourceSlice =
	static_cast<qint64>(kslc)*static_cast<qint64>(subSamplingLevel);
      if (sourceSlice < 0 || sourceSlice >= m_depth)
	{
	  progress.finish(false);
	  showVolumeError("Calculated source slice is outside the volume");
	  return false;
	}

      progress.setValue(static_cast<int>(100LL*kslc/depth));
      if (kslc%10 == 0 && qApp)
	qApp->processEvents();

      const uchar *volumeSlice =
	pvlFileManager.getSlice(static_cast<int>(sourceSlice));
      if (!volumeSlice)
	{
	  const QString detail = pvlFileManager.lastError();
	  progress.finish(false);
	  showVolumeError(detail.isEmpty() ?
			  QString("Cannot read source slice %1").arg(sourceSlice) :
			  detail);
	  return false;
	}

      unsigned char *destination =
	lowresVolume.get()+static_cast<size_t>(kslc)*lowresPlaneBytes;
      if (subSamplingLevel > 1)
	{
	  size_t destinationVoxel = 0;
	  if (bpv == 1)
	    {
	      for (int j=0; j<width; ++j)
		{ 
		  const size_t y = static_cast<size_t>(j)*subSamplingLevel;
		  for (int i=0; i<height; ++i)
		    { 
		      const size_t x = static_cast<size_t>(i)*subSamplingLevel;
		      const size_t sourceVoxel = y*static_cast<size_t>(m_height)+x;
		      if (sourceVoxel >= sourcePlaneVoxels)
			{
			  progress.finish(false);
			  showVolumeError("Calculated source voxel is outside the slice");
			  return false;
			}
		      destination[destinationVoxel++] = volumeSlice[sourceVoxel];
		    } 
		}
	    }
	  else
	    {
	      const ushort *source16 = reinterpret_cast<const ushort*>(volumeSlice);
	      ushort *destination16 = reinterpret_cast<ushort*>(destination);
	      for (int j=0; j<width; ++j)
		{ 
		  const size_t y = static_cast<size_t>(j)*subSamplingLevel;
		  for (int i=0; i<height; ++i)
		    { 
		      const size_t x = static_cast<size_t>(i)*subSamplingLevel;
		      const size_t sourceVoxel = y*static_cast<size_t>(m_height)+x;
		      if (sourceVoxel >= sourcePlaneVoxels)
			{
			  progress.finish(false);
			  showVolumeError("Calculated source voxel is outside the slice");
			  return false;
			}
		      destination16[destinationVoxel++] = source16[sourceVoxel];
		    } 
		}
	    }
	}
      else
	std::memcpy(destination, volumeSlice, lowresPlaneBytes);
    }

  unsigned char *oldLowresVolume = m_lowresVolume;
  const Vec oldLowresVolumeSize = m_lowresVolumeSize;
  const int oldSubSamplingLevel = m_subSamplingLevel;
  m_lowresVolume = lowresVolume.release();
  m_lowresVolumeSize = lowresVolumeSize;
  m_subSamplingLevel = subSamplingLevel;

  if (!generateHistograms())
    {
      delete [] m_lowresVolume;
      m_lowresVolume = oldLowresVolume;
      m_lowresVolumeSize = oldLowresVolumeSize;
      m_subSamplingLevel = oldSubSamplingLevel;
      progress.finish(false);
      return false;
    }

  delete [] oldLowresVolume;
  progress.finish(true);
  return true;
}

bool
VolumeBase::generateHistograms()
{
  int bpv = 0;
  if (!m_lowresVolume || !supportedPvlVoxelType(m_pvlVoxelType, bpv))
    {
      showVolumeError("Cannot generate histograms without a valid low-resolution volume");
      return false;
    }

  const int height = static_cast<int>(m_lowresVolumeSize.x);
  const int width = static_cast<int>(m_lowresVolumeSize.y);
  const int depth = static_cast<int>(m_lowresVolumeSize.z);

  size_t planeVoxels = 0;
  size_t volumeBytes = 0;
  if (!checkedVolumeSize(height, width, depth, bpv,
			 planeVoxels, volumeBytes))
    {
      showVolumeError("Histogram volume dimensions overflow addressable memory");
      return false;
    }
  Q_UNUSED(volumeBytes);

  std::unique_ptr<int[]> histogram1D(new(std::nothrow) int[256]);
  std::unique_ptr<int[]> histogram2D(new(std::nothrow) int[256*256]);
  std::unique_ptr<float[]> floatHistogram1D(new(std::nothrow) float[256]);
  std::unique_ptr<float[]> floatHistogram2D(
    new(std::nothrow) float[256*256]);
  if (!histogram1D || !histogram2D ||
      !floatHistogram1D || !floatHistogram2D)
    {
      showVolumeError("Not enough memory to generate low-resolution histograms");
      return false;
    }

  std::memset(histogram1D.get(), 0, 256*sizeof(int));
  std::memset(histogram2D.get(), 0, 256*256*sizeof(int));
  std::memset(floatHistogram1D.get(), 0, 256*sizeof(float));
  std::memset(floatHistogram2D.get(), 0, 256*256*sizeof(float));

  size_t volumeVoxels = 0;
  if (!checkedMultiply(planeVoxels, static_cast<size_t>(depth),
		       volumeVoxels))
    {
      showVolumeError("Histogram voxel count overflow");
      return false;
    }

  if (bpv == 1) // uchar
    {
      float *flhist1D = floatHistogram1D.get();
      float *flhist2D = floatHistogram2D.get();
      for (size_t voxel=0; voxel<volumeVoxels; ++voxel)
	flhist1D[m_lowresVolume[voxel]]++;

      if (depth < 3 || width < 3 || height < 3)
	{
	  for (size_t voxel=0; voxel<volumeVoxels; ++voxel)
	    flhist2D[m_lowresVolume[voxel]]++;
	}
      else
	{
	  for (int k=1; k<depth-1; ++k)
	    {
	      const uchar *g0 =
		m_lowresVolume+static_cast<size_t>(k-1)*planeVoxels;
	      const uchar *g1 =
		m_lowresVolume+static_cast<size_t>(k)*planeVoxels;
	      const uchar *g2 =
		m_lowresVolume+static_cast<size_t>(k+1)*planeVoxels;

	      for (int j=1; j<width-1; ++j)
		for (int i=1; i<height-1; ++i)
		  {
		    const size_t index = static_cast<size_t>(j)*height+i;
		    int gx = g1[index+1] - g1[index-1];
		    int gy = g1[index+height] - g1[index-height];
		    int gz = g2[index] - g0[index];
		    int gsum = sqrtf(gx*gx+gy*gy+gz*gz);
		    gsum = qBound(0, gsum, 255);
		    int v = g1[index];
		    flhist2D[gsum*256 + v]++;
		  }
	    }
	}
    }
  else // ushort
    {
      float *flhist1D = floatHistogram1D.get();
      float *flhist2D = floatHistogram2D.get();
      const ushort *volume16 = reinterpret_cast<const ushort*>(m_lowresVolume);
      for (size_t k=0; k<volumeVoxels; ++k)
	flhist1D[volume16[k]/256]++;

      for (size_t k=0; k<volumeVoxels; ++k)
	flhist2D[volume16[k]]++;
    }

  if (m_pvlVoxelType == 0)
    StaticFunctions::generateHistograms(floatHistogram1D.get(),
					floatHistogram2D.get(),
					histogram1D.get(), histogram2D.get());
  else // just copy
    {
      for (int i=0; i<256; ++i)
	histogram1D[i] = static_cast<int>(floatHistogram1D[i]);
      for (int i=0; i<256*256; ++i)
	histogram2D[i] = static_cast<int>(floatHistogram2D[i]);
    }

  delete [] m_1dHistogram;
  delete [] m_2dHistogram;
  m_1dHistogram = histogram1D.release();
  m_2dHistogram = histogram2D.release();
  return true;
}

bool
VolumeBase::createLowresTextureVolume()
{
  if (Global::volumeType() == Global::DummyVolume)
    return true;

  if (!m_lowresVolume)
    {
      showVolumeError("Cannot create a low-resolution texture without volume data");
      return false;
    }

  int px2, py2, pz2;
  const int vx = static_cast<int>(m_lowresVolumeSize.x);
  const int vy = static_cast<int>(m_lowresVolumeSize.y);
  const int vz = static_cast<int>(m_lowresVolumeSize.z);
  if (vx <= 0 || vy <= 0 || vz <= 0 ||
      !powerOfTwoExponent(vx, px2) ||
      !powerOfTwoExponent(vy, py2) ||
      !powerOfTwoExponent(vz, pz2))
    {
      showVolumeError("Invalid low-resolution texture dimensions");
      return false;
    }

  int nsubX, nsubY, nsubZ;
  if (!powerOfTwoValue(px2, nsubX) ||
      !powerOfTwoValue(py2, nsubY) ||
      !powerOfTwoValue(pz2, nsubZ))
    {
      showVolumeError("Low-resolution texture dimensions are too large");
      return false;
    }

  int bpv = 0;
  if (!supportedPvlVoxelType(m_pvlVoxelType, bpv))
    {
      showVolumeError(QString("Unsupported PVL voxel type %1")
		      .arg(m_pvlVoxelType));
      return false;
    }

  size_t texturePlaneVoxels = 0;
  size_t textureBytes = 0;
  if (!checkedVolumeSize(nsubX, nsubY, nsubZ, bpv,
			 texturePlaneVoxels, textureBytes))
    {
      showVolumeError("Low-resolution texture size overflows addressable memory");
      return false;
    }

  size_t sourcePlaneVoxels = 0;
  size_t sourceBytes = 0;
  if (!checkedVolumeSize(vx, vy, vz, bpv, sourcePlaneVoxels, sourceBytes))
    {
      showVolumeError("Low-resolution source size overflows addressable memory");
      return false;
    }
  Q_UNUSED(sourceBytes);

  size_t rowBytes = 0;
  if (!checkedMultiply(static_cast<size_t>(vx), static_cast<size_t>(bpv),
		       rowBytes))
    {
      showVolumeError("Low-resolution texture row size overflow");
      return false;
    }

  std::unique_ptr<unsigned char[]> textureVolume(
    new(std::nothrow) unsigned char[textureBytes]);
  if (!textureVolume)
    {
      showVolumeError(QString("Not enough memory for the %1-byte low-resolution texture")
		      .arg(static_cast<qulonglong>(textureBytes)));
      return false;
    }
  std::memset(textureVolume.get(), 0, textureBytes);

  LowresProgress progress;
  progress.start("Generating Lowres Texture Volume",
		 "Preparing data for Lowres mode");

  for (int k=0; k<vz; ++k)
    {
      progress.setValue(static_cast<int>(100LL*k/vz));
      if (k%10 == 0 && qApp)
	qApp->processEvents();
      for (int j=0; j<vy; ++j)
	{
	  const size_t textureVoxelOffset =
	    static_cast<size_t>(k)*texturePlaneVoxels+
	    static_cast<size_t>(j)*nsubX;
	  const size_t sourceVoxelOffset =
	    static_cast<size_t>(k)*sourcePlaneVoxels+
	    static_cast<size_t>(j)*vx;
	  std::memcpy(textureVolume.get()+textureVoxelOffset*bpv,
		      m_lowresVolume+sourceVoxelOffset*bpv,
		      rowBytes);
	}
    }

  delete [] m_lowresTextureVolume;
  m_lowresTextureVolume = textureVolume.release();
  m_lowresTextureVolumeSize = Vec(nsubX, nsubY, nsubZ);
  progress.finish(true);
  return true;
}
