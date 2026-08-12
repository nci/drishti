#include "global.h"
#include "volumergbbase.h"
#include "staticfunctions.h"
#include "volumefilemanager.h"
#include "mainwindowui.h"
#include "xmlheaderfunctions.h"
#include "volumeinformation.h"

#include <QDomDocument>
#include <QFile>

#include <cstring>
#include <initializer_list>
#include <limits>
#include <memory>
#include <new>

namespace
{
const int Histogram1DSize = 256;
const int Histogram2DSize = 256*256;

bool
checkedMultiply(qint64 lhs, qint64 rhs, qint64& result)
{
  if (lhs < 0 || rhs < 0 ||
      (lhs != 0 && rhs > std::numeric_limits<qint64>::max()/lhs))
    return false;

  result = lhs*rhs;
  return true;
}

bool
checkedProduct(std::initializer_list<size_t> factors, size_t& result)
{
  result = 1;
  for (std::initializer_list<size_t>::const_iterator it = factors.begin();
       it != factors.end(); ++it)
    {
      if (*it != 0 && result > std::numeric_limits<size_t>::max()/(*it))
	return false;
      result *= *it;
    }
  return true;
}

qulonglong
roundedMiB(size_t bytes)
{
  const size_t bytesPerMiB = 1024U*1024U;
  return static_cast<qulonglong>(bytes/bytesPerMiB+
	                         (bytes%bytesPerMiB != 0));
}

bool
powerOfTwoExponent(int value, int& exponent)
{
  if (value < 0)
    return false;

  quint64 power = 1;
  exponent = 0;
  while (power < static_cast<quint64>(value))
    {
      if (exponent >= 30)
	return false;
      power <<= 1;
      ++exponent;
    }
  return true;
}

bool
powerOfTwoValue(int exponent, int& value)
{
  if (exponent < 0 || exponent >= std::numeric_limits<int>::digits)
    return false;

  value = static_cast<int>(1U << exponent);
  return true;
}

int
channelCount()
{
  return Global::volumeType() == Global::RGBAVolume ? 4 : 3;
}

QString
channelName(int channel)
{
  static const char* names[] = {"red", "green", "blue", "alpha"};
  return QString::fromLatin1(names[channel]);
}

bool
readDimensions(const QString& volumeFile,
	       int& depth, int& width, int& height,
	       QString& error)
{
  QFile file(volumeFile);
  if (!file.open(QIODevice::ReadOnly))
    {
      error = QString("Cannot open colour volume header %1: %2")
	      .arg(volumeFile, file.errorString());
      return false;
    }

  QDomDocument document;
  QString parseError;
  int errorLine = 0;
  int errorColumn = 0;
  if (!document.setContent(&file, &parseError, &errorLine, &errorColumn))
    {
      error = QString("Invalid colour volume XML at line %1, column %2: %3")
	      .arg(errorLine).arg(errorColumn).arg(parseError);
      return false;
    }

  const QDomNodeList gridNodes = document.elementsByTagName("gridsize");
  if (gridNodes.count() != 1)
    {
      error = "Colour volume header must contain exactly one gridsize element";
      return false;
    }

  const QStringList dimensions =
    gridNodes.at(0).toElement().text().simplified()
             .split(' ', QString::SkipEmptyParts);
  if (dimensions.count() != 3)
    {
      error = "Colour volume gridsize must contain exactly three integers";
      return false;
    }

  int* outputs[3] = {&depth, &width, &height};
  for (int axis=0; axis<3; ++axis)
    {
      bool ok = false;
      const qlonglong value = dimensions.at(axis).toLongLong(&ok);
      if (!ok || value <= 0 || value > std::numeric_limits<int>::max())
	{
	  error = QString("Invalid colour volume gridsize value '%1'")
	          .arg(dimensions.at(axis));
	  return false;
	}
      *outputs[axis] = static_cast<int>(value);
    }

  return true;
}

QWidget*
mainWindowWidget()
{
  Ui::MainWindow* ui = MainWindowUI::mainWindowUI();
  if (!ui || !ui->menubar)
    return 0;
  return ui->menubar->parentWidget();
}

void
setMainWindowTitle(const QString& title)
{
  QWidget* window = mainWindowWidget();
  if (window)
    window->setWindowTitle(title);
}

class ProgressScope
{
 public:
  ProgressScope(const QString& title, bool enabled)
    : m_window(enabled ? mainWindowWidget() : 0),
      m_progress(m_window ? Global::progressBar() : 0),
      m_completed(false)
  {
    if (m_window)
      {
	m_previousTitle = m_window->windowTitle();
	m_window->setWindowTitle(title);
      }
    if (m_progress)
      {
	m_progress->setValue(0);
	m_progress->show();
      }
  }

  ~ProgressScope()
  {
    if (m_progress)
      {
	m_progress->setValue(m_completed ? 100 : 0);
	Global::hideProgressBar();
      }
    if (m_window)
      m_window->setWindowTitle(m_previousTitle);
  }

  void setValue(int value)
  {
    if (m_progress)
      m_progress->setValue(qBound(0, value, 100));
  }

  void complete() { m_completed = true; }

 private:
  QWidget* m_window;
  QProgressBar* m_progress;
  QString m_previousTitle;
  bool m_completed;
};

class BoolFlagScope
{
 public:
  explicit BoolFlagScope(bool& flag)
    : m_flag(flag), m_previousValue(flag)
  {
    m_flag = true;
  }

  ~BoolFlagScope()
  {
    m_flag = m_previousValue;
  }

 private:
  bool& m_flag;
  bool m_previousValue;
};

bool
configureFileManagers(VolumeFileManager (&managers)[4],
		      const QString& volumeFile,
		      int depth, int width, int height,
		      int slabSize, int channels,
		      QString& error)
{
  if (volumeFile.length() < 6)
    {
      error = "Colour volume filename is too short to identify channel files";
      return false;
    }

  if (slabSize <= 0)
    {
      error = "Colour volume has an invalid slab size";
      return false;
    }

  QString baseFilename = volumeFile;
  baseFilename.chop(6);
  for (int channel=0; channel<channels; ++channel)
    {
      managers[channel].setBaseFilename(baseFilename+channelName(channel));
      managers[channel].setDepth(depth);
      managers[channel].setWidth(width);
      managers[channel].setHeight(height);
      managers[channel].setHeaderSize(13);
      managers[channel].setSlabSize(slabSize);

      if (!managers[channel].exists())
	{
	  error = QString("Cannot read the %1 channel volume: %2")
	          .arg(channelName(channel), managers[channel].lastError());
	  return false;
	}
    }

  return true;
}
}


Vec VolumeRGBBase::getFullVolumeSize() { return m_fullVolumeSize; }
Vec VolumeRGBBase::getLowresVolumeSize() { return m_lowresVolumeSize; }
Vec VolumeRGBBase::getLowresTextureVolumeSize() { return m_lowresTextureVolumeSize; }
int VolumeRGBBase::getLowresSubsamplingLevel() { return m_subSamplingLevel; }

unsigned char* VolumeRGBBase::getLowresVolume() { return m_lowresVolume; }
unsigned char* VolumeRGBBase::getLowresTextureVolume() { return m_lowresTextureVolume; }

int*
VolumeRGBBase::getLowres1dHistogram(int vn)
{
  if (vn == 0)
    return m_1dHistogramR;
  if (vn == 1)
    return m_1dHistogramG;
  if (vn == 2)
    return m_1dHistogramB;
  if (vn == 3)
    return m_1dHistogramA;

  return 0;
}

int*
VolumeRGBBase::getLowres2dHistogram(int vn)
{
  if (vn == 0)
    return m_2dHistogramR;
  if (vn == 1)
    return m_2dHistogramG;
  if (vn == 2)
    return m_2dHistogramB;
  if (vn == 3)
    return m_2dHistogramA;

  return 0;
}


VolumeRGBBase::VolumeRGBBase()
  : m_depth(0),
    m_width(0),
    m_height(0),
    m_1dHistogramR(0),
    m_2dHistogramR(0),
    m_1dHistogramG(0),
    m_2dHistogramG(0),
    m_1dHistogramB(0),
    m_2dHistogramB(0),
    m_1dHistogramA(0),
    m_2dHistogramA(0),
    m_subSamplingLevel(1),
    m_lowresVolume(0),
    m_lowresTextureVolume(0),
    m_loadingVolume(false)
{
  m_fullVolumeSize = Vec(0, 0, 0);
  m_lowresVolumeSize = Vec(0, 0, 0);
  m_lowresTextureVolumeSize = Vec(0, 0, 0);

  // A failed eager allocation leaves a valid object with null histograms.
  // loadVolume() retries and reports the failure through its bool contract.
  ensureHistogramStorage();
}

VolumeRGBBase::~VolumeRGBBase()
{
  clearHistogramStorage();

  delete [] m_lowresVolume;
  delete [] m_lowresTextureVolume;
  m_lowresVolume = m_lowresTextureVolume = 0;
}

void
VolumeRGBBase::clearHistogramStorage()
{
  delete [] m_1dHistogramR;
  delete [] m_2dHistogramR;
  delete [] m_1dHistogramG;
  delete [] m_2dHistogramG;
  delete [] m_1dHistogramB;
  delete [] m_2dHistogramB;
  delete [] m_1dHistogramA;
  delete [] m_2dHistogramA;

  m_1dHistogramR = m_2dHistogramR = 0;
  m_1dHistogramG = m_2dHistogramG = 0;
  m_1dHistogramB = m_2dHistogramB = 0;
  m_1dHistogramA = m_2dHistogramA = 0;
}

bool
VolumeRGBBase::ensureHistogramStorage()
{
  if (m_1dHistogramR && m_2dHistogramR &&
      m_1dHistogramG && m_2dHistogramG &&
      m_1dHistogramB && m_2dHistogramB &&
      m_1dHistogramA && m_2dHistogramA)
    return true;

  clearHistogramStorage();

  std::unique_ptr<int[]> hist1DR(new (std::nothrow) int[Histogram1DSize]);
  std::unique_ptr<int[]> hist2DR(new (std::nothrow) int[Histogram2DSize]);
  std::unique_ptr<int[]> hist1DG(new (std::nothrow) int[Histogram1DSize]);
  std::unique_ptr<int[]> hist2DG(new (std::nothrow) int[Histogram2DSize]);
  std::unique_ptr<int[]> hist1DB(new (std::nothrow) int[Histogram1DSize]);
  std::unique_ptr<int[]> hist2DB(new (std::nothrow) int[Histogram2DSize]);
  std::unique_ptr<int[]> hist1DA(new (std::nothrow) int[Histogram1DSize]);
  std::unique_ptr<int[]> hist2DA(new (std::nothrow) int[Histogram2DSize]);

  if (!hist1DR || !hist2DR || !hist1DG || !hist2DG ||
      !hist1DB || !hist2DB || !hist1DA || !hist2DA)
    return setError("Not enough memory for colour volume histograms");

  memset(hist1DR.get(), 0, Histogram1DSize*sizeof(int));
  memset(hist2DR.get(), 0, Histogram2DSize*sizeof(int));
  memset(hist1DG.get(), 0, Histogram1DSize*sizeof(int));
  memset(hist2DG.get(), 0, Histogram2DSize*sizeof(int));
  memset(hist1DB.get(), 0, Histogram1DSize*sizeof(int));
  memset(hist2DB.get(), 0, Histogram2DSize*sizeof(int));
  memset(hist1DA.get(), 0, Histogram1DSize*sizeof(int));
  memset(hist2DA.get(), 0, Histogram2DSize*sizeof(int));

  m_1dHistogramR = hist1DR.release();
  m_2dHistogramR = hist2DR.release();
  m_1dHistogramG = hist1DG.release();
  m_2dHistogramG = hist2DG.release();
  m_1dHistogramB = hist1DB.release();
  m_2dHistogramB = hist2DB.release();
  m_1dHistogramA = hist1DA.release();
  m_2dHistogramA = hist2DA.release();
  return true;
}

bool
VolumeRGBBase::setError(const QString& error)
{
  m_errorString = error;
  qWarning("%s", qPrintable(error));
  return false;
}

bool
VolumeRGBBase::loadVolume(const char* volfile, bool redo)
{
  m_errorString.clear();
  if (!volfile || !volfile[0])
    {
      setError("No colour volume file was specified");
      QMessageBox::information(0, "Error", m_errorString);
      return false;
    }

  const QString volumeFile = QString::fromUtf8(volfile);
  if (!VolumeInformation::xmlHeaderFile(volumeFile))
    {
      QMessageBox::information(0, "Error",
	QString("%1 is not a valid colour volume file").arg(volumeFile));
      return false;
    }

  int depth = 0;
  int width = 0;
  int height = 0;
  QString dimensionError;
  if (!readDimensions(volumeFile, depth, width, height, dimensionError))
    {
      setError(dimensionError);
      QMessageBox::information(0, "Error", m_errorString);
      return false;
    }

  qint64 sliceVoxels = 0;
  qint64 volumeVoxels = 0;
  if (!checkedMultiply(width, height, sliceVoxels) ||
      !checkedMultiply(sliceVoxels, depth, volumeVoxels))
    {
      setError("Colour volume dimensions overflow the supported size");
      QMessageBox::information(0, "Error", m_errorString);
      return false;
    }
  Q_UNUSED(volumeVoxels);

  const QString oldVolumeFile = m_volumeFile;
  const int oldDepth = m_depth;
  const int oldWidth = m_width;
  const int oldHeight = m_height;
  const Vec oldFullVolumeSize = m_fullVolumeSize;
  const Vec oldLowresVolumeSize = m_lowresVolumeSize;
  const Vec oldLowresTextureVolumeSize = m_lowresTextureVolumeSize;
  const int oldSubSamplingLevel = m_subSamplingLevel;
  int* old1DHistogramR = m_1dHistogramR;
  int* old2DHistogramR = m_2dHistogramR;
  int* old1DHistogramG = m_1dHistogramG;
  int* old2DHistogramG = m_2dHistogramG;
  int* old1DHistogramB = m_1dHistogramB;
  int* old2DHistogramB = m_2dHistogramB;
  int* old1DHistogramA = m_1dHistogramA;
  int* old2DHistogramA = m_2dHistogramA;
  uchar* oldLowresVolume = m_lowresVolume;
  uchar* oldLowresTextureVolume = m_lowresTextureVolume;

  m_1dHistogramR = m_2dHistogramR = 0;
  m_1dHistogramG = m_2dHistogramG = 0;
  m_1dHistogramB = m_2dHistogramB = 0;
  m_1dHistogramA = m_2dHistogramA = 0;
  m_lowresVolume = 0;
  m_lowresTextureVolume = 0;

  m_volumeFile = volumeFile;
  m_depth = depth;
  m_width = width;
  m_height = height;
  m_fullVolumeSize = Vec(m_height, m_width, m_depth);
  m_lowresVolumeSize = Vec(0, 0, 0);
  m_lowresTextureVolumeSize = Vec(0, 0, 0);
  m_subSamplingLevel = 1;

  bool ok = false;
  {
    BoolFlagScope loadingScope(m_loadingVolume);
    ok = generateHistograms(redo) &&
         createLowresVolume(redo) &&
         createLowresTextureVolume();
  }

  if (!ok)
    {
      clearHistogramStorage();
      delete [] m_lowresVolume;
      delete [] m_lowresTextureVolume;

      m_volumeFile = oldVolumeFile;
      m_depth = oldDepth;
      m_width = oldWidth;
      m_height = oldHeight;
      m_fullVolumeSize = oldFullVolumeSize;
      m_lowresVolumeSize = oldLowresVolumeSize;
      m_lowresTextureVolumeSize = oldLowresTextureVolumeSize;
      m_subSamplingLevel = oldSubSamplingLevel;
      m_1dHistogramR = old1DHistogramR;
      m_2dHistogramR = old2DHistogramR;
      m_1dHistogramG = old1DHistogramG;
      m_2dHistogramG = old2DHistogramG;
      m_1dHistogramB = old1DHistogramB;
      m_2dHistogramB = old2DHistogramB;
      m_1dHistogramA = old1DHistogramA;
      m_2dHistogramA = old2DHistogramA;
      m_lowresVolume = oldLowresVolume;
      m_lowresTextureVolume = oldLowresTextureVolume;

      setMainWindowTitle("Drishti - Volume Exploration and Presentation Tool");
      QMessageBox::information(0, "Error",
	m_errorString.isEmpty() ? "Cannot load colour volume" : m_errorString);
      return false;
    }

  delete [] old1DHistogramR;
  delete [] old2DHistogramR;
  delete [] old1DHistogramG;
  delete [] old2DHistogramG;
  delete [] old1DHistogramB;
  delete [] old2DHistogramB;
  delete [] old1DHistogramA;
  delete [] old2DHistogramA;
  delete [] oldLowresVolume;
  delete [] oldLowresTextureVolume;

  setMainWindowTitle("Drishti - Volume Exploration and Presentation Tool");
  return true;
}

bool
VolumeRGBBase::generateHistograms(bool redo)
{
  Q_UNUSED(redo);

  const int channels = channelCount();
  const int slabSize = XmlHeaderFunctions::getSlabsizeFromHeader(m_volumeFile);
  VolumeFileManager fileManagers[4];
  QString error;
  if (!configureFileManagers(fileManagers, m_volumeFile,
			     m_depth, m_width, m_height,
			     slabSize, channels, error))
    return setError(error);

  size_t oneDCount = 0;
  size_t twoDCount = 0;
  size_t oneDBytes = 0;
  size_t twoDBytes = 0;
  if (!checkedProduct({static_cast<size_t>(channels),
	               static_cast<size_t>(Histogram1DSize)}, oneDCount) ||
      !checkedProduct({static_cast<size_t>(channels),
	               static_cast<size_t>(Histogram2DSize)}, twoDCount) ||
      !checkedProduct({oneDCount, sizeof(float)}, oneDBytes) ||
      !checkedProduct({twoDCount, sizeof(float)}, twoDBytes))
    return setError("Colour histogram allocation size overflows");

  std::unique_ptr<float[]> hist1D(new (std::nothrow) float[oneDCount]);
  std::unique_ptr<float[]> hist2D(new (std::nothrow) float[twoDCount]);
  if (!hist1D || !hist2D)
    return setError("Not enough memory to calculate colour histograms");
  memset(hist1D.get(), 0, oneDBytes);
  memset(hist2D.get(), 0, twoDBytes);

  float* hist1DR = hist1D.get();
  float* hist1DG = hist1DR+Histogram1DSize;
  float* hist1DB = hist1DG+Histogram1DSize;
  float* hist1DA = channels == 4 ? hist1DB+Histogram1DSize : 0;
  float* hist2DR = hist2D.get();
  float* hist2DG = hist2DR+Histogram2DSize;
  float* hist2DB = hist2DG+Histogram2DSize;
  float* hist2DA = channels == 4 ? hist2DB+Histogram2DSize : 0;

  size_t slicePixels = 0;
  if (!checkedProduct({static_cast<size_t>(m_width),
	               static_cast<size_t>(m_height)}, slicePixels))
    return setError("Colour volume slice size overflows");

  ProgressScope progress("Reading Volume", !m_loadingVolume);
  for (int k=0; k<m_depth; ++k)
    {
      progress.setValue(static_cast<int>(100.0*static_cast<double>(k)/m_depth));
      if (qApp && !m_loadingVolume)
	qApp->processEvents();

      uchar* slices[4] = {0, 0, 0, 0};
      for (int channel=0; channel<channels; ++channel)
	{
	  slices[channel] = fileManagers[channel].getSlice(k);
	  if (!slices[channel])
	    return setError(QString("Cannot read %1 channel slice %2: %3")
	                    .arg(channelName(channel)).arg(k)
	                    .arg(fileManagers[channel].lastError()));
	}

      for (size_t index=0; index<slicePixels; ++index)
	{
	  const uchar r = slices[0][index];
	  const uchar g = slices[1][index];
	  const uchar b = slices[2][index];

	  hist1DR[r]++;
	  hist2DR[static_cast<int>(g)*256+r]++;
	  hist1DG[g]++;
	  hist2DG[static_cast<int>(b)*256+g]++;
	  hist1DB[b]++;
	  hist2DB[static_cast<int>(r)*256+b]++;

	  if (channels == 4)
	    {
	      const uchar alpha = slices[3][index];
	      const uchar rgb = qMax(r, qMax(g, b));
	      hist1DA[alpha]++;
	      hist2DA[static_cast<int>(rgb)*256+alpha]++;
	    }
	}
    }

  if (!ensureHistogramStorage())
    return false;

  memset(m_1dHistogramR, 0, Histogram1DSize*sizeof(int));
  memset(m_2dHistogramR, 0, Histogram2DSize*sizeof(int));
  memset(m_1dHistogramG, 0, Histogram1DSize*sizeof(int));
  memset(m_2dHistogramG, 0, Histogram2DSize*sizeof(int));
  memset(m_1dHistogramB, 0, Histogram1DSize*sizeof(int));
  memset(m_2dHistogramB, 0, Histogram2DSize*sizeof(int));
  memset(m_1dHistogramA, 0, Histogram1DSize*sizeof(int));
  memset(m_2dHistogramA, 0, Histogram2DSize*sizeof(int));

  StaticFunctions::generateHistograms(hist1DR, hist2DR,
			      m_1dHistogramR, m_2dHistogramR);
  StaticFunctions::generateHistograms(hist1DG, hist2DG,
			      m_1dHistogramG, m_2dHistogramG);
  StaticFunctions::generateHistograms(hist1DB, hist2DB,
			      m_1dHistogramB, m_2dHistogramB);
  if (channels == 4)
    StaticFunctions::generateHistograms(hist1DA, hist2DA,
				m_1dHistogramA, m_2dHistogramA);

  progress.complete();
  return true;
}

bool
VolumeRGBBase::createLowresVolume(bool redo)
{
  Q_UNUSED(redo);

  int px2 = 0;
  int py2 = 0;
  int pz2 = 0;
  if (!powerOfTwoExponent(m_height, px2) ||
      !powerOfTwoExponent(m_width, py2) ||
      !powerOfTwoExponent(m_depth, pz2))
    return setError("Colour volume dimensions exceed the supported range");

  const int texturePowerBudget = qMin(26, Global::textureSize()-1);
  if (texturePowerBudget < 0)
    return setError("The low-resolution texture budget is invalid");

  int samplingLevel = 1;
  while (px2+py2+pz2 > texturePowerBudget)
    {
      if (samplingLevel > std::numeric_limits<int>::max()/2)
	return setError("Low-resolution sampling level overflows");
      samplingLevel *= 2;
      if (!powerOfTwoExponent(m_height/samplingLevel, px2) ||
	  !powerOfTwoExponent(m_width/samplingLevel, py2) ||
	  !powerOfTwoExponent(m_depth/samplingLevel, pz2))
	return setError("Low-resolution dimensions exceed the supported range");
    }

  if ((px2 > 9 || py2 > 9 || pz2 > 9) && samplingLevel == 1)
    samplingLevel = 2;

  const int height = qMax(1, m_height/samplingLevel);
  const int width = qMax(1, m_width/samplingLevel);
  const int depth = qMax(1, m_depth/samplingLevel);
  const Vec lowresSize(height, width, depth);

  const int channels = channelCount();
  size_t sourcePixels = 0;
  size_t planeElements = 0;
  size_t outputBytes = 0;
  if (!checkedProduct({static_cast<size_t>(m_width),
	               static_cast<size_t>(m_height)}, sourcePixels) ||
      !checkedProduct({static_cast<size_t>(channels),
	               static_cast<size_t>(width),
	               static_cast<size_t>(height)}, planeElements) ||
      !checkedProduct({planeElements, static_cast<size_t>(depth)}, outputBytes))
    return setError("Low-resolution colour volume allocation size overflows");

  const int slabSize = XmlHeaderFunctions::getSlabsizeFromHeader(m_volumeFile);
  VolumeFileManager fileManagers[4];
  QString error;
  if (!configureFileManagers(fileManagers, m_volumeFile,
			     m_depth, m_width, m_height,
			     slabSize, channels, error))
    return setError(error);

  std::unique_ptr<uchar[]> lowresVolume(
	new (std::nothrow) uchar[outputBytes]);
  if (!lowresVolume)
    return setError(QString("Not enough memory for the %1 MiB low-resolution colour volume")
	            .arg(roundedMiB(outputBytes)));
  memset(lowresVolume.get(), 0, outputBytes);

  ProgressScope progress("Generating Lowres Version", !m_loadingVolume);
  if (samplingLevel == 1)
    {
      for (int k=0; k<m_depth; ++k)
	{
	  progress.setValue(static_cast<int>(100.0*static_cast<double>(k)/m_depth));
	  if (qApp && !m_loadingVolume && k%10 == 0)
	    qApp->processEvents();

	  for (int channel=0; channel<channels; ++channel)
	    {
	      uchar* slice = fileManagers[channel].getSlice(k);
	      if (!slice)
		return setError(QString("Cannot read %1 channel slice %2: %3")
		                .arg(channelName(channel)).arg(k)
		                .arg(fileManagers[channel].lastError()));

	      const size_t outputSlice = static_cast<size_t>(k)*planeElements;
	      for (size_t index=0; index<sourcePixels; ++index)
		lowresVolume[outputSlice+channels*index+channel] = slice[index];
	    }
	}
    }
  else
    {
      const qint64 ringCount64 = 2LL*samplingLevel-1;
      if (ringCount64 <= 0 ||
	  ringCount64 > std::numeric_limits<int>::max() ||
	  ringCount64 > std::numeric_limits<ushort>::max()/255)
	return setError("Low-resolution filter window exceeds the supported range");
      const int ringCount = static_cast<int>(ringCount64);

      size_t ringBytes = 0;
      size_t accumulatorBytes = 0;
      if (!checkedProduct({static_cast<size_t>(ringCount), planeElements},
		          ringBytes) ||
	  !checkedProduct({planeElements, sizeof(ushort)}, accumulatorBytes))
	return setError("Low-resolution filter allocation size overflows");

      std::unique_ptr<uchar[]> ringData(new (std::nothrow) uchar[ringBytes]);
      std::unique_ptr<ushort[]> accumulator(
	new (std::nothrow) ushort[planeElements]);
      if (!ringData || !accumulator)
	return setError("Not enough memory for the low-resolution colour filter");

      int count = 0;
      int nextSlot = 0;
      int outputSlice = 0;
      for (int k=0; k<m_depth; ++k)
	{
	  progress.setValue(static_cast<int>(100.0*static_cast<double>(k)/m_depth));
	  if (qApp && !m_loadingVolume && k%10 == 0)
	    qApp->processEvents();

	  uchar* filteredSlice = ringData.get()+
	                         static_cast<size_t>(nextSlot)*planeElements;
	  for (int channel=0; channel<channels; ++channel)
	    {
	      uchar* slice = fileManagers[channel].getSlice(k);
	      if (!slice)
		return setError(QString("Cannot read %1 channel slice %2: %3")
		                .arg(channelName(channel)).arg(k)
		                .arg(fileManagers[channel].lastError()));

	      size_t lowresIndex = 0;
	      for (int j=0; j<width; ++j)
		{
		  const int y = j*samplingLevel;
		  const int lowY = qMax(0, y-samplingLevel+1);
		  const int highY = qMin(m_width-1, y+samplingLevel-1);
		  for (int i=0; i<height; ++i)
		    {
		      const int x = i*samplingLevel;
		      const int lowX = qMax(0, x-samplingLevel+1);
		      const int highX = qMin(m_height-1, x+samplingLevel-1);

		      float sum = 0.0f;
		      for (int jy=lowY; jy<=highY; ++jy)
			for (int ix=lowX; ix<=highX; ++ix)
			  sum += slice[static_cast<size_t>(jy)*m_height+ix];

		      const qint64 sampleCount =
			static_cast<qint64>(highY-lowY+1)*(highX-lowX+1);
		      filteredSlice[channels*lowresIndex+channel] =
			static_cast<uchar>(sum/static_cast<float>(sampleCount));
		      ++lowresIndex;
		    }
		}
	    }

	  nextSlot = (nextSlot+1)%ringCount;
	  ++count;
	  if (count == ringCount)
	    {
	      if (outputSlice >= depth)
		return setError("Low-resolution filter produced too many slices");

	      memset(accumulator.get(), 0, accumulatorBytes);
	      for (int slot=0; slot<ringCount; ++slot)
		{
		  const uchar* source = ringData.get()+
		                        static_cast<size_t>(slot)*planeElements;
		  for (size_t index=0; index<planeElements; ++index)
		    accumulator[index] += source[index];
		}

	      uchar* destination = lowresVolume.get()+
	                           static_cast<size_t>(outputSlice)*planeElements;
	      for (size_t index=0; index<planeElements; ++index)
		destination[index] = static_cast<uchar>(accumulator[index]/ringCount);

	      count = ringCount/2;
	      ++outputSlice;
	    }
	}
      if (outputSlice == 0)
	{
	  const int partialCount = qMin(m_depth, ringCount);
	  if (partialCount <= 0)
	    return setError("Low-resolution filter has no source slices");

	  memset(accumulator.get(), 0, accumulatorBytes);
	  for (int slot=0; slot<partialCount; ++slot)
	    {
	      const uchar* source = ringData.get()+
	                            static_cast<size_t>(slot)*planeElements;
	      for (size_t index=0; index<planeElements; ++index)
		accumulator[index] += source[index];
	    }
	  for (size_t index=0; index<planeElements; ++index)
	    lowresVolume[index] =
	      static_cast<uchar>(accumulator[index]/partialCount);
	  outputSlice = 1;
	}

      for (int slice=outputSlice; slice<depth; ++slice)
	memcpy(lowresVolume.get()+static_cast<size_t>(slice)*planeElements,
	       lowresVolume.get()+static_cast<size_t>(slice-1)*planeElements,
	       planeElements);
    }

  delete [] m_lowresVolume;
  m_lowresVolume = lowresVolume.release();
  m_lowresVolumeSize = lowresSize;
  m_subSamplingLevel = samplingLevel;
  progress.complete();
  return true;
}

bool
VolumeRGBBase::createLowresTextureVolume()
{
  if (!m_lowresVolume)
    return setError("Low-resolution colour volume is unavailable");

  const int vx = static_cast<int>(m_lowresVolumeSize.x);
  const int vy = static_cast<int>(m_lowresVolumeSize.y);
  const int vz = static_cast<int>(m_lowresVolumeSize.z);
  if (vx <= 0 || vy <= 0 || vz <= 0)
    return setError("Low-resolution texture dimensions must all be positive");

  int px2 = 0;
  int py2 = 0;
  int pz2 = 0;
  int nsubX = 0;
  int nsubY = 0;
  int nsubZ = 0;
  if (!powerOfTwoExponent(vx, px2) ||
      !powerOfTwoExponent(vy, py2) ||
      !powerOfTwoExponent(vz, pz2) ||
      !powerOfTwoValue(px2, nsubX) ||
      !powerOfTwoValue(py2, nsubY) ||
      !powerOfTwoValue(pz2, nsubZ))
    return setError("Low-resolution texture dimensions exceed the supported range");

  const int channels = channelCount();
  size_t textureRowBytes = 0;
  size_t textureSliceBytes = 0;
  size_t textureBytes = 0;
  size_t sourceRowBytes = 0;
  size_t sourceSliceBytes = 0;
  if (!checkedProduct({static_cast<size_t>(channels),
	               static_cast<size_t>(nsubX)}, textureRowBytes) ||
      !checkedProduct({textureRowBytes,
	               static_cast<size_t>(nsubY)}, textureSliceBytes) ||
      !checkedProduct({textureSliceBytes,
	               static_cast<size_t>(nsubZ)}, textureBytes) ||
      !checkedProduct({static_cast<size_t>(channels),
	               static_cast<size_t>(vx)}, sourceRowBytes) ||
      !checkedProduct({sourceRowBytes,
	               static_cast<size_t>(vy)}, sourceSliceBytes))
    return setError("Low-resolution texture allocation size overflows");

  std::unique_ptr<uchar[]> texture(new (std::nothrow) uchar[textureBytes]);
  if (!texture)
    return setError(QString("Not enough memory for the %1 MiB low-resolution texture")
	            .arg(roundedMiB(textureBytes)));
  memset(texture.get(), 0, textureBytes);

  ProgressScope progress("Generating Lowres Texture Volume", !m_loadingVolume);
  for (int k=0; k<vz; ++k)
    {
      progress.setValue(static_cast<int>(100.0*static_cast<double>(k)/vz));
      if (qApp && !m_loadingVolume && k%10 == 0)
	qApp->processEvents();

      for (int j=0; j<vy; ++j)
	memcpy(texture.get()+static_cast<size_t>(k)*textureSliceBytes+
	                         static_cast<size_t>(j)*textureRowBytes,
	       m_lowresVolume+static_cast<size_t>(k)*sourceSliceBytes+
	                          static_cast<size_t>(j)*sourceRowBytes,
	       sourceRowBytes);
    }

  delete [] m_lowresTextureVolume;
  m_lowresTextureVolume = texture.release();
  m_lowresTextureVolumeSize = Vec(nsubX, nsubY, nsubZ);
  progress.complete();
  return true;
}
