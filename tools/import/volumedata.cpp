#include "common.h"
#include "importmemoryadmission.h"
#include "volumedata.h"
#include "pluginoperationstatus.h"
#include "volumepluginvalidation.h"
#include "volumevaluemapping.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <limits>
#include <memory>
#include <new>

using namespace std;

namespace
{
const std::uint64_t kPreviewSafetyBytes = 64ULL*1024ULL*1024ULL;

QString previewMemoryAmount(std::uint64_t bytes)
{
  const double mib = static_cast<double>(bytes)/(1024.0*1024.0);
  if (mib < 1024.0)
    return QStringLiteral("%1 MiB").arg(mib, 0, 'f', 1);
  return QStringLiteral("%1 GiB").arg(mib/1024.0, 0, 'f', 2);
}

QString previewAdmissionError(const QString& operation,
                              const ImportMemoryAdmission& admission)
{
  QString reason;
  switch (admission.reason)
    {
    case ImportMemoryAdmissionReason::MemoryStatusUnavailable:
      reason = QStringLiteral(
        "Current physical-memory or Windows Commit headroom is unavailable.");
      break;
    case ImportMemoryAdmissionReason::InsufficientPhysicalMemory:
      reason = QStringLiteral(
        "The preview would consume reserved physical-memory headroom.");
      break;
    case ImportMemoryAdmissionReason::InsufficientCommit:
      reason = QStringLiteral(
        "The preview would consume reserved Windows Commit headroom.");
      break;
    case ImportMemoryAdmissionReason::AddressSpaceLimit:
      reason = QStringLiteral("The preview exceeds this process address space.");
      break;
    case ImportMemoryAdmissionReason::ArithmeticOverflow:
    case ImportMemoryAdmissionReason::InvalidRequest:
      reason = QStringLiteral("The preview buffer request is invalid.");
      break;
    case ImportMemoryAdmissionReason::Approved:
      reason = QStringLiteral("The preview allocation was approved.");
      break;
    }
  return QStringLiteral(
    "%1 was stopped before decoding. Required peak increment: %2; "
    "usable physical budget: %3; usable Commit budget: %4. %5")
    .arg(operation,
         previewMemoryAmount(admission.requiredBytes),
         admission.physicalMemoryChecked ?
           previewMemoryAmount(admission.availablePhysicalBudgetBytes) :
           QStringLiteral("unavailable"),
         admission.commitMemoryChecked ?
           previewMemoryAmount(admission.availableCommitBudgetBytes) :
           QStringLiteral("unavailable"),
         reason);
}

bool alignedPreviewStride(std::uint64_t rowBytes, std::uint64_t& stride)
{
  if (!checkedImportAdd(rowBytes, 3, stride))
    return false;
  stride &= ~std::uint64_t(3);
  return true;
}

bool preparePreviewBuffers(const QString& operation,
                           int width, int height,
                           int sourceBytesPerPixel,
                           int displayBytesPerPixel,
                           std::unique_ptr<uchar[]>& source,
                           std::unique_ptr<uchar[]>& display,
                           std::uint64_t& pixels,
                           std::uint64_t& sourceBytes,
                           std::uint64_t& displayBytes,
                           std::uint64_t& displayStride,
                           QString& error)
{
  pixels = sourceBytes = displayBytes = displayStride = 0;
  std::uint64_t requiredBytes = 0;
  std::uint64_t displayRowBytes = 0;
  if (width <= 0 || height <= 0 || sourceBytesPerPixel <= 0 ||
      displayBytesPerPixel <= 0 ||
      !checkedImportMultiply(static_cast<std::uint64_t>(width),
                             static_cast<std::uint64_t>(height), pixels) ||
      pixels > static_cast<std::uint64_t>(
                 std::numeric_limits<int>::max()) ||
      !checkedImportMultiply(pixels,
                             static_cast<std::uint64_t>(sourceBytesPerPixel),
                             sourceBytes) ||
      !checkedImportMultiply(static_cast<std::uint64_t>(width),
                             static_cast<std::uint64_t>(displayBytesPerPixel),
                             displayRowBytes) ||
      !alignedPreviewStride(displayRowBytes, displayStride) ||
      !checkedImportMultiply(displayStride,
                             static_cast<std::uint64_t>(height),
                             displayBytes) ||
      displayRowBytes > static_cast<std::uint64_t>(
                          std::numeric_limits<int>::max()) ||
      displayStride > static_cast<std::uint64_t>(
                        std::numeric_limits<int>::max()) ||
      !checkedImportAdd(sourceBytes, displayBytes, requiredBytes) ||
      !checkedImportAdd(requiredBytes, kPreviewSafetyBytes, requiredBytes))
    {
      error = operation + QStringLiteral(
        " was stopped because its dimensions or buffer size are invalid.");
      return false;
    }

  const ImportMemoryAdmission admission =
    evaluateImportMemoryAdmission(requiredBytes);
  if (!admission.approved)
    {
      error = previewAdmissionError(operation, admission);
      return false;
    }

  if (sourceBytes > static_cast<std::uint64_t>(
                      std::numeric_limits<std::size_t>::max()) ||
      displayBytes > static_cast<std::uint64_t>(
                       std::numeric_limits<std::size_t>::max()))
    {
      error = operation + QStringLiteral(
        " exceeds this process address space.");
      return false;
    }

  source.reset(new (std::nothrow) uchar[
    static_cast<std::size_t>(sourceBytes)]);
  display.reset(new (std::nothrow) uchar[
    static_cast<std::size_t>(displayBytes)]);
  if (!source || !display)
    {
      error = operation + QStringLiteral(
        " could not allocate its admitted buffers. The system memory state "
        "changed; decoding was not started.");
      return false;
    }
  std::memset(source.get(), 0, static_cast<std::size_t>(sourceBytes));
  std::memset(display.get(), 0, static_cast<std::size_t>(displayBytes));
  return true;
}

std::uint64_t previewDisplayOffset(std::uint64_t pixel,
                                   int width,
                                   std::uint64_t stride,
                                   int bytesPerPixel)
{
  const std::uint64_t row = pixel/static_cast<std::uint64_t>(width);
  const std::uint64_t column = pixel%static_cast<std::uint64_t>(width);
  return row*stride+column*static_cast<std::uint64_t>(bytesPerPixel);
}

bool expandPackedColorSlice(const uchar *source,
                            int voxelType,
                            std::uint64_t pixelCount,
                            uchar *destination)
{
  if (!source || !destination ||
      (voxelType != _Rgb && voxelType != _Rgba))
    return false;

  const std::uint64_t bytesPerPixel = voxelType == _Rgb ? 3 : 4;
  QRgb *pixels = reinterpret_cast<QRgb*>(destination);
  for (std::uint64_t i=0; i<pixelCount; ++i)
    {
      const uchar *input = source + i*bytesPerPixel;
      pixels[i] = voxelType == _Rgb ?
        qRgb(input[0], input[1], input[2]) :
        qRgba(input[0], input[1], input[2], input[3]);
    }
  return true;
}

bool mapPreviewValue(double value,
                     const QList<float>& rawMap,
                     const QList<int>& pvlMap,
                     int pvlMapMax,
                     uchar& destination)
{
  int mapped = 0;
  if (!mapImportValueToPvl(value, rawMap, pvlMap, mapped))
    return false;
  if (pvlMapMax > 255)
    mapped /= 256;
  destination = static_cast<uchar>(qBound(0, mapped, 255));
  return true;
}
}

VolumeData::VolumeData()
{
  m_image = 0;
  m_volInterface = 0;
  m_pluginObject = 0;
  clear();
}

VolumeData::~VolumeData()
{
  clear();
}

QStringList
VolumeData::sourceFiles() const
{
  if (m_pluginObject)
    {
      SourceFilesProvider *provider =
        qobject_cast<SourceFilesProvider*>(m_pluginObject);
      if (provider)
        {
          const QStringList resolved = provider->sourceFiles();
          if (!resolved.isEmpty())
            return resolved;
        }
    }
  return m_fileName;
}

void
VolumeData::clear()
{
  m_scriptsPlugin.clear();

  if (m_volInterface)
    {
      delete m_volInterface;
      m_volInterface = 0;
    }
  m_pluginObject = 0;

  m_scriptsPluginActive = false;

  m_fileName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_headerBytes = 0;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_lastError.clear();
  m_lastOperationCanceled = false;

  m_pvlMapMax = 255;
  m_rawMap.clear();
  m_pvlMap.clear();

  if (m_image)
    delete [] m_image;
  m_image = 0;
}

void
VolumeData::setVoxelInfo(int vu,
			 float vx, float vy, float vz)
{
  m_voxelUnit = vu;  
  m_voxelSizeX = vx;
  m_voxelSizeY = vy;
  m_voxelSizeZ = vz;
}

void
VolumeData::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString VolumeData::description()
{ return m_description; }
int VolumeData::voxelType() { return m_voxelType; }
int VolumeData::voxelUnit() { return m_voxelUnit; }
int VolumeData::headerBytes() { return m_headerBytes; }
int VolumeData::bytesPerVoxel() { return m_bytesPerVoxel; }

bool
VolumeData::setMinMax(float rmin, float rmax)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (!std::isfinite(rmin) || !std::isfinite(rmax) || rmin > rmax)
    {
      m_lastError = "The requested histogram range is invalid.";
      return false;
    }

  QList<uint> candidateHistogram;
  if (m_scriptsPluginActive)
    {
      try
        {
          m_scriptsPlugin.setMinMax(rmin, rmax);
          candidateHistogram = m_scriptsPlugin.histogram();
          m_lastError = m_scriptsPlugin.lastError();
        }
      catch (const std::bad_alloc&)
        {
          m_lastError = "The Python volume decoder ran out of memory while "
                        "regenerating its histogram.";
        }
      catch (const std::exception& exception)
        {
          m_lastError = QString("The Python volume decoder raised an exception "
                                "while regenerating its histogram: %1")
            .arg(QString::fromLocal8Bit(exception.what()));
        }
      catch (...)
        {
          m_lastError = "The Python volume decoder raised an unknown exception "
                        "while regenerating its histogram.";
        }
    }
  else
    {
      VolumePluginOperationStatus status;
      if (!updateNativeVolumePluginRange(m_volInterface, rmin, rmax,
                                         &candidateHistogram, &status))
        {
          m_lastError = status.error;
          m_lastOperationCanceled = status.canceled;

          QList<uint> ignoredHistogram;
          VolumePluginOperationStatus rollbackStatus;
          if (!updateNativeVolumePluginRange(
                m_volInterface, m_rawMin, m_rawMax,
                &ignoredHistogram, &rollbackStatus))
            {
              const QString rollbackError = rollbackStatus.error.isEmpty() ?
                QStringLiteral("unknown rollback failure") :
                rollbackStatus.error;
              m_lastError += QString(" The previous decoder range could not "
                                     "be restored: %1").arg(rollbackError);
            }
        }
    }

  if (!m_lastError.isEmpty() || m_lastOperationCanceled)
    return false;

  VolumePluginMetadata candidate;
  candidate.description = m_description;
  candidate.depth = m_depth;
  candidate.width = m_width;
  candidate.height = m_height;
  candidate.voxelType = m_voxelType;
  candidate.voxelUnit = m_voxelUnit;
  candidate.headerBytes = m_headerBytes;
  candidate.voxelSizeX = m_voxelSizeX;
  candidate.voxelSizeY = m_voxelSizeY;
  candidate.voxelSizeZ = m_voxelSizeZ;
  candidate.rawMinimum = rmin;
  candidate.rawMaximum = rmax;
  candidate.histogram = candidateHistogram;
  if (!validateVolumePluginMetadata(candidate, &m_lastError))
    return false;

  m_rawMin = rmin;
  m_rawMax = rmax;
  m_histogram = candidateHistogram;
  return true;
}
float VolumeData::rawMin() { return m_rawMin; }
float VolumeData::rawMax() { return m_rawMax; }
QList<uint> VolumeData::histogram() { return m_histogram; }
QList<float> VolumeData::rawMap() { return m_rawMap; }
QList<int> VolumeData::pvlMap() { return m_pvlMap; }

void
VolumeData::setMap(QList<float> rm,
		   QList<int> pm)
{
  int ignored = 0;
  if (!mapImportValueToPvl(0.0, rm, pm, ignored))
    {
      m_lastError = "The raw-to-preview value map is invalid.";
      return;
    }

  m_rawMap = rm;
  m_pvlMap = pm;

  m_pvlMapMax = m_pvlMap[m_pvlMap.count()-1];
}

void
VolumeData::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

bool
VolumeData::replaceFile(QString flnm)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (flnm.isEmpty())
    {
      m_lastError = "The replacement volume filename is empty.";
      return false;
    }

  try
    {
      if (m_scriptsPluginActive)
	{
	  if (!m_scriptsPlugin.replaceFile(flnm))
	    {
	      m_lastError = m_scriptsPlugin.lastError();
	      if (m_lastError.isEmpty())
		m_lastError = "The Python volume decoder rejected the replacement file.";
	      return false;
	    }
	  m_scriptsPlugin.gridSize(m_depth, m_width, m_height);
	  m_voxelType = m_scriptsPlugin.voxelType();
	}
	else
	{
	  if (!m_volInterface)
	    {
	      m_lastError = "No volume decoder is loaded.";
	      return false;
	    }
	  m_volInterface->replaceFile(flnm);
	  QObject *pluginObject = dynamic_cast<QObject*>(m_volInterface);
	  m_lastError = importPluginLastError(pluginObject);
	  m_lastOperationCanceled = importPluginWasCanceled(pluginObject);
	  if (m_lastOperationCanceled && m_lastError.isEmpty())
	    m_lastError = "The volume decoder canceled replacement-file loading.";
	  if (!m_lastError.isEmpty() || m_lastOperationCanceled)
	    return false;
	  m_volInterface->gridSize(m_depth, m_width, m_height);
	  m_voxelType = m_volInterface->voxelType();
	}
    }
  catch (const std::exception& exception)
    {
      m_lastError = QString("The volume decoder raised an exception while "
			    "replacing the input: %1")
	.arg(QString::fromLocal8Bit(exception.what()));
      return false;
    }
  catch (...)
    {
      m_lastError = "The volume decoder raised an unknown exception while "
	            "replacing the input.";
      return false;
    }

  if (m_depth <= 0 || m_width <= 0 || m_height <= 0)
    {
      m_lastError = "The replacement volume has invalid dimensions.";
      return false;
    }

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UShort || m_voxelType == _Short)
    m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int || m_voxelType == _Float)
    m_bytesPerVoxel = 4;
  else if (m_voxelType == _Rgb)
    m_bytesPerVoxel = 3;
  else if (m_voxelType == _Rgba)
    m_bytesPerVoxel = 4;

  m_fileName = QStringList() << flnm;
  return true;
}

bool
VolumeData::loadPlugin(QString pluginflnm)
{
  QStringList s = pluginflnm.split(" : ");
  if (s[0] == "script")
    {
      if (s.size() < 3 || s[2].trimmed().isEmpty())
	{
	  m_lastError = "The script plugin descriptor is malformed.";
	  return false;
	}
      QString jsonflnm = s[2];
      
      if (m_scriptsPlugin.start(jsonflnm))
      { 
        m_scriptsPluginActive = true;
        //QMessageBox::information(0, "Script Plugin Loaded", "Successfully loaded script plugin");
        return true;
      }

      QMessageBox::information(0, "Error", "Cannot load script plugin");
      m_scriptsPluginActive = false;
      return false;
    }
  

  QPluginLoader pluginLoader(pluginflnm);
  QObject *plugin = pluginLoader.instance();

  if (plugin)
    {
      m_volInterface = qobject_cast<VolInterface *>(plugin);
      if (m_volInterface)
	{
          m_pluginObject = plugin;
          return true;
	}
    }

  QMessageBox::information(0, "Error", "Cannot load plugin");

  return false;
}

bool
VolumeData::setFile(QStringList files,
		                QString voltype)
{
  clear();

  m_fileName = files;

  if (!loadPlugin(voltype))
    return false;
    
  if (m_scriptsPluginActive)
    {
      m_scriptsPlugin.init();
      m_scriptsPlugin.set4DVolume(false);
      if (! m_scriptsPlugin.setFile(m_fileName))
	{
	  m_lastError = m_scriptsPlugin.lastError();
	  if (m_lastError.isEmpty())
	    m_lastError = "The Python volume decoder rejected the selected input.";
	  return false;
	}

      m_scriptsPlugin.gridSize(m_depth, m_width, m_height);
      m_voxelType = m_scriptsPlugin.voxelType();
      m_description = m_scriptsPlugin.description();
    }
  else
    {
      VolumePluginMetadata metadata;
      if (!loadNativeVolumePlugin(m_volInterface, m_fileName,
                                  false, false, &metadata))
	{
	  m_lastError = metadata.error;
	  m_lastOperationCanceled = metadata.canceled;
	  return false;
	}
      m_depth = metadata.depth;
      m_width = metadata.width;
      m_height = metadata.height;
      m_voxelType = metadata.voxelType;
      m_voxelUnit = metadata.voxelUnit;
      m_headerBytes = metadata.headerBytes;
      m_voxelSizeX = metadata.voxelSizeX;
      m_voxelSizeY = metadata.voxelSizeY;
      m_voxelSizeZ = metadata.voxelSizeZ;
      m_rawMin = metadata.rawMinimum;
      m_rawMax = metadata.rawMaximum;
      m_histogram = metadata.histogram;
      m_description = metadata.description;
    }

  if (m_depth <= 0 || m_width <= 0 || m_height <= 0 ||
      m_voxelType < _UChar || m_voxelType > _Rgba)
    {
      m_lastError = "The volume decoder returned invalid dimensions or voxel type.";
      return false;
    }

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Rgb) m_bytesPerVoxel = 3;
  else if (m_voxelType == _Rgba) m_bytesPerVoxel = 4;

  float vx, vy, vz;
  if (m_scriptsPluginActive)
    {
      m_scriptsPlugin.voxelSize(vx, vy, vz);
      m_voxelSizeX = vx;
      m_voxelSizeY = vy;
      m_voxelSizeZ = vz;
      m_voxelUnit = m_scriptsPlugin.voxelUnit();
      m_headerBytes = m_scriptsPlugin.headerBytes();
    }
  
  if (m_scriptsPluginActive)
    {
      m_rawMin = m_scriptsPlugin.rawMin();
      m_rawMax = m_scriptsPlugin.rawMax();
      m_histogram = m_scriptsPlugin.histogram();
      if (m_histogram.isEmpty())
	{
	  m_lastError = m_scriptsPlugin.lastError();
	  if (m_lastError.isEmpty())
	    m_lastError = "The Python volume decoder returned an empty histogram.";
	  return false;
	}
    }
  VolumePluginMetadata validatedMetadata;
  validatedMetadata.depth = m_depth;
  validatedMetadata.width = m_width;
  validatedMetadata.height = m_height;
  validatedMetadata.voxelType = m_voxelType;
  validatedMetadata.voxelUnit = m_voxelUnit;
  validatedMetadata.headerBytes = m_headerBytes;
  validatedMetadata.voxelSizeX = m_voxelSizeX;
  validatedMetadata.voxelSizeY = m_voxelSizeY;
  validatedMetadata.voxelSizeZ = m_voxelSizeZ;
  validatedMetadata.rawMinimum = m_rawMin;
  validatedMetadata.rawMaximum = m_rawMax;
  validatedMetadata.histogram = m_histogram;
  if (!validateVolumePluginMetadata(validatedMetadata, &m_lastError))
    return false;

  m_rawMap.append(m_rawMin);
  m_rawMap.append(m_rawMax);
  m_pvlMap.append(0);
  m_pvlMap.append(m_pvlMapMax);

  return true;
}

bool
VolumeData::setFile(QStringList files,
		                QString voltype,
		                bool vol4d,
		                bool skipRawDialog)
{
  clear();

  m_fileName = files;

  if (!loadPlugin(voltype))
    return false;

  std::cout << std::endl << std::endl << std::endl;
  std::cout << "--------------------------------------------------------------" << std::endl;
  std::cout << std::endl << "Loading files: " << files.join(", ").toStdString() << std::endl;

  if (m_scriptsPluginActive)
    {
      m_scriptsPlugin.init();
      m_scriptsPlugin.set4DVolume(vol4d);

      if (! m_scriptsPlugin.setFile(m_fileName))
	{
	  m_lastError = m_scriptsPlugin.lastError();
	  if (m_lastError.isEmpty())
	    m_lastError = "The Python volume decoder rejected the selected input.";
	  return false;
	}

      m_scriptsPlugin.gridSize(m_depth, m_width, m_height);
      m_voxelType = m_scriptsPlugin.voxelType();
      m_description = m_scriptsPlugin.description();
    }
  else
    {
      VolumePluginMetadata metadata;
      if (!loadNativeVolumePlugin(m_volInterface, m_fileName,
                                  vol4d, skipRawDialog, &metadata))
	{
	  m_lastError = metadata.error;
	  m_lastOperationCanceled = metadata.canceled;
	  return false;
	}
      m_depth = metadata.depth;
      m_width = metadata.width;
      m_height = metadata.height;
      m_voxelType = metadata.voxelType;
      m_voxelUnit = metadata.voxelUnit;
      m_headerBytes = metadata.headerBytes;
      m_voxelSizeX = metadata.voxelSizeX;
      m_voxelSizeY = metadata.voxelSizeY;
      m_voxelSizeZ = metadata.voxelSizeZ;
      m_rawMin = metadata.rawMinimum;
      m_rawMax = metadata.rawMaximum;
      m_histogram = metadata.histogram;
      m_description = metadata.description;
    }

  if (m_depth <= 0 || m_width <= 0 || m_height <= 0 ||
      m_voxelType < _UChar || m_voxelType > _Rgba)
    {
      m_lastError = "The volume decoder returned invalid dimensions or voxel type.";
      return false;
    }

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Rgb) m_bytesPerVoxel = 3;
  else if (m_voxelType == _Rgba) m_bytesPerVoxel = 4;

  float vx, vy, vz;
  if (m_scriptsPluginActive)
    {
      m_scriptsPlugin.voxelSize(vx, vy, vz);
      m_voxelSizeX = vx;
      m_voxelSizeY = vy;
      m_voxelSizeZ = vz;
      m_voxelUnit = m_scriptsPlugin.voxelUnit();
      m_headerBytes = m_scriptsPlugin.headerBytes();
    }
  

  if (m_scriptsPluginActive)
    {
      m_rawMin = m_scriptsPlugin.rawMin();
      m_rawMax = m_scriptsPlugin.rawMax();
      m_histogram = m_scriptsPlugin.histogram();
      if (m_histogram.isEmpty())
	{
	  m_lastError = m_scriptsPlugin.lastError();
	  if (m_lastError.isEmpty())
	    m_lastError = "The Python volume decoder returned an empty histogram.";
	  return false;
	}
    }
  VolumePluginMetadata validatedMetadata;
  validatedMetadata.depth = m_depth;
  validatedMetadata.width = m_width;
  validatedMetadata.height = m_height;
  validatedMetadata.voxelType = m_voxelType;
  validatedMetadata.voxelUnit = m_voxelUnit;
  validatedMetadata.headerBytes = m_headerBytes;
  validatedMetadata.voxelSizeX = m_voxelSizeX;
  validatedMetadata.voxelSizeY = m_voxelSizeY;
  validatedMetadata.voxelSizeZ = m_voxelSizeZ;
  validatedMetadata.rawMinimum = m_rawMin;
  validatedMetadata.rawMaximum = m_rawMax;
  validatedMetadata.histogram = m_histogram;
  if (!validateVolumePluginMetadata(validatedMetadata, &m_lastError))
    return false;

  m_rawMap.append(m_rawMin);
  m_rawMap.append(m_rawMax);
  m_pvlMap.append(0);
  m_pvlMap.append(m_pvlMapMax);

  printVolumeInfo();

  return true;
}

void
VolumeData::printVolumeInfo()
{
    QString vstr;
    if (m_voxelUnit == _Nanometer)
      vstr = "nanometer";
    else if (m_voxelUnit == _Micron)
      vstr = "micron";
    else if (m_voxelUnit == _Millimeter)
      vstr = "millimeter";
    else if (m_voxelUnit == _Centimeter)
      vstr = "centimeter";
    else if (m_voxelUnit == _Meter)
      vstr = "meter";
    else
      vstr = QString::number(m_voxelUnit);

    QString vtstr;
    if (m_voxelType == _UChar)
      vtstr = "Unsigned Char";
    else if (m_voxelType == _Char)
      vtstr = "Signed Char";
    else if (m_voxelType == _UShort)
      vtstr = "Unsigned Short";
    else if (m_voxelType == _Short)
      vtstr = "Signed Short";
    else if (m_voxelType == _Int)
      vtstr = "Integer";
    else if (m_voxelType == _Float)
      vtstr = "Float";
    else if (m_voxelType == _Rgb)
      vtstr = "RGB";
    else if (m_voxelType == _Rgba)
      vtstr = "RGBA";
    else
      vtstr = QString::number(m_voxelType);

    QString mesg;
    mesg = QString("description : %1\n").arg(description());
    mesg += QString("voxelunit : %1\n").arg(vstr);
    mesg += QString("voxeltype : %1\n").arg(vtstr);
    mesg += QString("voxel size : %1 %2 %3\n").arg(m_voxelSizeX).arg(m_voxelSizeY).arg(m_voxelSizeZ);
    mesg += QString("bytes per voxel : %1\n").arg(m_bytesPerVoxel);
    mesg += QString("header : %1\n").arg(headerBytes());
    mesg += QString("dim : %1 %2 %3\n").arg(m_depth).arg(m_width).arg(m_height);		    
    mesg += QString("raw min max : %1 %2\n").arg(m_rawMin).arg(m_rawMax);		      

    cout << "\nVolume Data Information:" << endl;
    cout << mesg.toStdString() << endl;
}

bool
VolumeData::getDepthSlice(int slc, uchar *slice)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (!slice || slc < 0 || slc >= m_depth)
    {
      m_lastError = QString("Slice index %1 is outside [0, %2), or the output "
			    "buffer is null.").arg(slc).arg(m_depth);
      return false;
    }

  try
    {
      if (m_scriptsPluginActive)
	{
	  if (!m_scriptsPlugin.getDepthSlice(slc, slice))
	    {
	      m_lastError = m_scriptsPlugin.lastError();
	      if (m_lastError.isEmpty())
		m_lastError = "The Python volume decoder rejected the slice.";
	      return false;
	    }
	  return true;
	}
	else
	{
	  VolumePluginOperationStatus status;
	  if (!readNativeVolumePluginSlice(
	        m_volInterface, VolumePluginSliceAxis::Depth,
	        slc, slice, &status))
	    {
	      m_lastError = status.error;
	      m_lastOperationCanceled = status.canceled;
	      return false;
	    }
	  return true;
	}
    }
  catch (const std::exception& exception)
    {
      m_lastError = QString("The volume decoder raised an exception while "
			    "reading slice %1: %2")
	.arg(slc).arg(QString::fromLocal8Bit(exception.what()));
      return false;
    }
  catch (...)
    {
      m_lastError = QString("The volume decoder raised an unknown exception "
			    "while reading slice %1.").arg(slc);
      return false;
    }
}

QString VolumeData::lastError() const { return m_lastError; }
bool VolumeData::lastOperationCanceled() const
{
  return m_lastOperationCanceled;
}

QImage
VolumeData::getDepthSliceImage(int slc)
{
  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  if (slc < 0 || slc >= nX)
    return QImage(100, 100, QImage::Format_Indexed8);

  const bool color = m_voxelType == _Rgb || m_voxelType == _Rgba;
  if (!color && m_rawMap.size() < 2)
    {
      QMessageBox::warning(0, "Depth Preview",
                           "The raw-to-preview value map is invalid.");
      return QImage();
    }

  std::unique_ptr<uchar[]> sourceStorage;
  std::unique_ptr<uchar[]> imageStorage;
  std::uint64_t pixelCount = 0;
  std::uint64_t sourceBytes = 0;
  std::uint64_t imageBytes = 0;
  std::uint64_t imageStride = 0;
  QString bufferError;
  if (!preparePreviewBuffers(
        QStringLiteral("Depth preview"), nZ, nY,
        m_bytesPerVoxel, color ? 4 : 1,
        sourceStorage, imageStorage,
        pixelCount, sourceBytes, imageBytes, imageStride, bufferError))
    {
      QMessageBox::warning(0, "Depth Preview", bufferError);
      return QImage();
    }
  uchar *tmp = sourceStorage.get();
  uchar *image = imageStorage.get();

  if (!getDepthSlice(slc, tmp))
    {
      QMessageBox::warning(0, "Depth Preview", m_lastError);
      return QImage();
    }

  if (m_voxelType == _Rgb || m_voxelType == _Rgba)
    {  
      if (!expandPackedColorSlice(tmp, m_voxelType, pixelCount, image))
        return QImage();
      if (m_image)
        delete [] m_image;
      m_image = imageStorage.release();
      QImage img = QImage(m_image,
			  m_height, m_width, static_cast<int>(imageStride),
			  QImage::Format_ARGB32);
      return img;
    }

  for(int i=0; i<static_cast<int>(pixelCount); i++)
    {
      double v = 0.0;

      if (m_voxelType == _UChar)
	v = ((uchar *)tmp)[i];
      else if (m_voxelType == _Char)
	v = ((char *)tmp)[i];
      else if (m_voxelType == _UShort)
	v = ((ushort *)tmp)[i];
      else if (m_voxelType == _Short)
	v = ((short *)tmp)[i];
      else if (m_voxelType == _Int)
	v = ((int *)tmp)[i];
      else if (m_voxelType == _Float)
	v = ((float *)tmp)[i];

      uchar& destination = image[previewDisplayOffset(
        static_cast<std::uint64_t>(i), nZ, imageStride, 1)];
      if (!mapPreviewValue(v, m_rawMap, m_pvlMap,
			   m_pvlMapMax, destination))
	{
	  QMessageBox::warning(0, "Depth Preview",
			       "The raw-to-preview value map is invalid.");
	  return QImage();
	}
    }
  if (m_image)
    delete [] m_image;
  m_image = imageStorage.release();
  QImage img = QImage(m_image, nZ, nY, static_cast<int>(imageStride),
                      QImage::Format_Indexed8);
  return img;
}

QImage
VolumeData::getWidthSliceImage(int slc)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  if (slc < 0 || slc >= nY)
    {
      QImage img = QImage(100, 100, QImage::Format_Indexed8);
      return img;
    }

  const bool color = m_voxelType == _Rgb || m_voxelType == _Rgba;
  if (!color && m_rawMap.size() < 2)
    {
      QMessageBox::warning(0, "Width Preview",
                           "The raw-to-preview value map is invalid.");
      return QImage();
    }

  std::unique_ptr<uchar[]> sourceStorage;
  std::unique_ptr<uchar[]> imageStorage;
  std::uint64_t pixelCount = 0;
  std::uint64_t sourceBytes = 0;
  std::uint64_t imageBytes = 0;
  std::uint64_t imageStride = 0;
  QString bufferError;
  if (!preparePreviewBuffers(
        QStringLiteral("Width preview"), nZ, nX,
        m_bytesPerVoxel, color ? 4 : 1,
        sourceStorage, imageStorage,
        pixelCount, sourceBytes, imageBytes, imageStride, bufferError))
    {
      QMessageBox::warning(0, "Width Preview", bufferError);
      return QImage();
    }
  uchar *tmp = sourceStorage.get();
  uchar *image = imageStorage.get();

  if (m_scriptsPluginActive || !m_volInterface)
    {
      m_lastError = m_scriptsPluginActive ?
	"Width-oriented previews are unavailable for Python volume scripts." :
	"No volume decoder is loaded.";
      QMessageBox::warning(0, "Width Preview", m_lastError);
      return QImage();
    }
  VolumePluginOperationStatus widthStatus;
  if (!readNativeVolumePluginSlice(
        m_volInterface, VolumePluginSliceAxis::Width,
        slc, tmp, &widthStatus))
    {
      m_lastError = widthStatus.error;
      m_lastOperationCanceled = widthStatus.canceled;
      QMessageBox::warning(0, "Width Preview", m_lastError);
      return QImage();
    }

  if (m_voxelType == _Rgb || m_voxelType == _Rgba)
    {  
      if (!expandPackedColorSlice(tmp, m_voxelType, pixelCount, image))
        return QImage();
      if (m_image)
        delete [] m_image;
      m_image = imageStorage.release();
      QImage img = QImage(m_image,
			  m_height, m_depth, static_cast<int>(imageStride),
			  QImage::Format_ARGB32);
      return img;
    }

  for(int i=0; i<static_cast<int>(pixelCount); i++)
    {
      double v = 0.0;

      if (m_voxelType == _UChar)
	      v = ((uchar *)tmp)[i];
      else if (m_voxelType == _Char)
	      v = ((char *)tmp)[i];
      else if (m_voxelType == _UShort)
	      v = ((ushort *)tmp)[i];
      else if (m_voxelType == _Short)
	      v = ((short *)tmp)[i];
      else if (m_voxelType == _Int)
      	v = ((int *)tmp)[i];
      else if (m_voxelType == _Float)
      	v = ((float *)tmp)[i];

      uchar& destination = image[previewDisplayOffset(
        static_cast<std::uint64_t>(i), nZ, imageStride, 1)];
      if (!mapPreviewValue(v, m_rawMap, m_pvlMap,
			   m_pvlMapMax, destination))
	{
	  QMessageBox::warning(0, "Width Preview",
			       "The raw-to-preview value map is invalid.");
	  return QImage();
	}
    }
  if (m_image)
    delete [] m_image;
  m_image = imageStorage.release();
  QImage img = QImage(m_image, nZ, nX, static_cast<int>(imageStride),
                      QImage::Format_Indexed8);
  return img;
}

QImage
VolumeData::getHeightSliceImage(int slc)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  if (slc < 0 || slc >= nZ)
    {
      QImage img = QImage(100, 100, QImage::Format_Indexed8);
      return img;
    }

  const bool color = m_voxelType == _Rgb || m_voxelType == _Rgba;
  if (!color && m_rawMap.size() < 2)
    {
      QMessageBox::warning(0, "Height Preview",
                           "The raw-to-preview value map is invalid.");
      return QImage();
    }

  std::unique_ptr<uchar[]> sourceStorage;
  std::unique_ptr<uchar[]> imageStorage;
  std::uint64_t pixelCount = 0;
  std::uint64_t sourceBytes = 0;
  std::uint64_t imageBytes = 0;
  std::uint64_t imageStride = 0;
  QString bufferError;
  if (!preparePreviewBuffers(
        QStringLiteral("Height preview"), nY, nX,
        m_bytesPerVoxel, color ? 4 : 1,
        sourceStorage, imageStorage,
        pixelCount, sourceBytes, imageBytes, imageStride, bufferError))
    {
      QMessageBox::warning(0, "Height Preview", bufferError);
      return QImage();
    }
  uchar *tmp = sourceStorage.get();
  uchar *image = imageStorage.get();

  if (m_scriptsPluginActive || !m_volInterface)
    {
      m_lastError = m_scriptsPluginActive ?
	"Height-oriented previews are unavailable for Python volume scripts." :
	"No volume decoder is loaded.";
      QMessageBox::warning(0, "Height Preview", m_lastError);
      return QImage();
    }
  VolumePluginOperationStatus heightStatus;
  if (!readNativeVolumePluginSlice(
        m_volInterface, VolumePluginSliceAxis::Height,
        slc, tmp, &heightStatus))
    {
      m_lastError = heightStatus.error;
      m_lastOperationCanceled = heightStatus.canceled;
      QMessageBox::warning(0, "Height Preview", m_lastError);
      return QImage();
    }

  if (m_voxelType == _Rgb || m_voxelType == _Rgba)
    {  
      if (!expandPackedColorSlice(tmp, m_voxelType, pixelCount, image))
        return QImage();
      if (m_image)
        delete [] m_image;
      m_image = imageStorage.release();
      QImage img = QImage(m_image,
			  nY, nX, static_cast<int>(imageStride),
			  QImage::Format_ARGB32);
      return img;
    }

  for(int i=0; i<static_cast<int>(pixelCount); i++)
    {
      double v = 0.0;

      if (m_voxelType == _UChar)
	      v = ((uchar *)tmp)[i];
      else if (m_voxelType == _Char)
      	v = ((char *)tmp)[i];
      else if (m_voxelType == _UShort)
      	v = ((ushort *)tmp)[i];
      else if (m_voxelType == _Short)
      	v = ((short *)tmp)[i];
      else if (m_voxelType == _Int)
      	v = ((int *)tmp)[i];
      else if (m_voxelType == _Float)
      	v = ((float *)tmp)[i];

      uchar& destination = image[previewDisplayOffset(
        static_cast<std::uint64_t>(i), nY, imageStride, 1)];
      if (!mapPreviewValue(v, m_rawMap, m_pvlMap,
			   m_pvlMapMax, destination))
	{
	  QMessageBox::warning(0, "Height Preview",
			       "The raw-to-preview value map is invalid.");
	  return QImage();
	}
    }
  if (m_image)
    delete [] m_image;
  m_image = imageStorage.release();
  QImage img = QImage(m_image, nY, nX, static_cast<int>(imageStride),
                      QImage::Format_Indexed8);
  return img;
}

QPair<QVariant, QVariant>
VolumeData::rawValue(int d, int w, int h)
{
  QPair<QVariant, QVariant> pair;
  m_lastError.clear();
  m_lastOperationCanceled = false;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      pair.first = QVariant("OutOfBounds");
      pair.second = QVariant("OutOfBounds");
      return pair;
    }

  QVariant v;
  if (m_scriptsPluginActive)
    {
      v = m_scriptsPlugin.rawValue(d, w, h);
      m_lastError = m_scriptsPlugin.lastError();
      if (!m_lastError.isEmpty())
        {
          pair.first = QVariant(m_lastError);
          pair.second = QVariant("DecoderError");
          return pair;
        }
    }
  else
    {
      VolumePluginOperationStatus status;
      if (!readNativeVolumePluginRawValue(
            m_volInterface, d, w, h, &v, &status))
        {
          m_lastError = status.error;
          m_lastOperationCanceled = status.canceled;
          pair.first = QVariant(m_lastError);
          pair.second = QVariant(m_lastOperationCanceled ?
                                 "Canceled" : "DecoderError");
          return pair;
        }
    }


  if (v.type() == QVariant::String)
    {
      pair.first = v;

      QString str = v.toString();
      if (str == "OutOfBounds")
	      pair.second = QVariant("OutOfBounds");
      else
	      pair.second = QVariant("rgba");
      return pair;
    }

  double val = std::numeric_limits<double>::quiet_NaN();

  if (v.type() == QVariant::UInt)
    val = v.toUInt();
  else if (v.type() == QVariant::Int)
    val = v.toInt();
  else if (v.type() == QVariant::Double ||
          v.type() == QMetaType::Float)
    val = v.toDouble();

  int pv = 0;
  if (!mapImportValueToPvl(val, m_rawMap, m_pvlMap, pv))
    {
      pair.first = v;
      pair.second = QVariant("InvalidMap");
      return pair;
    }

  pair.first = v;
  pair.second = QVariant((uint)pv);
  return pair;
}

bool
VolumeData::saveTrimmed(QString trimFile,
		       int dmin, int dmax,
		       int wmin, int wmax,
		       int hmin, int hmax)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;

  if (m_scriptsPluginActive)
    {
      m_lastError =
	"Direct trimmed-volume export is unavailable for Python volume scripts.";
      QMessageBox::warning(0, "Save Trimmed Volume", m_lastError);
      return false;
    }
  if (!m_volInterface)
    {
      m_lastError = "No volume decoder is loaded.";
      QMessageBox::warning(0, "Save Trimmed Volume", m_lastError);
      return false;
    }

  QObject *pluginObject = dynamic_cast<QObject*>(m_volInterface);
  if (!importPluginReportsOperationStatus(pluginObject))
    {
      m_lastError =
	"The loaded volume decoder cannot report trimmed-export success or "
	"cancellation. Export was not started.";
      QMessageBox::warning(0, "Save Trimmed Volume", m_lastError);
      return false;
    }

  try
    {
      m_volInterface->saveTrimmed(trimFile,
				  dmin, dmax,
				  wmin, wmax,
				  hmin, hmax);

      m_lastError = importPluginLastError(pluginObject);
      m_lastOperationCanceled = importPluginWasCanceled(pluginObject);
    }
  catch (const std::exception& exception)
    {
      m_lastError = QString("The volume decoder raised an exception during "
			    "trimmed export: %1")
	.arg(QString::fromLocal8Bit(exception.what()));
      return false;
    }
  catch (...)
    {
      m_lastError = "The volume decoder raised an unknown exception during "
	            "trimmed export.";
      return false;
    }

  if (m_lastOperationCanceled && m_lastError.isEmpty())
    m_lastError = "The volume decoder canceled trimmed export.";
  return m_lastError.isEmpty() && !m_lastOperationCanceled;
}
