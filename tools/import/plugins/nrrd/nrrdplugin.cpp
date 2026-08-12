#include <QtGui>

#include "common.h"
#include "importmemoryadmission.h"
#include "nrrdplugin.h"

#include <QApplication>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QTimer>
#include <QVector>

#include <QtConcurrent>

#include <itkImageIOBase.h>
#include <itkMacro.h>
#include <itkNrrdImageIO.h>

#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace
{
const quint64 kNrrdDecodeSafetyBytes = 256ULL*1024ULL*1024ULL;
const quint64 kStatisticsChunkVoxels = 1024ULL*1024ULL;

struct NrrdVolumeInfo
{
  int depth;
  int width;
  int height;
  int voxelType;
  int bytesPerVoxel;
  float voxelSizeX;
  float voxelSizeY;
  float voxelSizeZ;
  quint64 voxelCount;
  quint64 volumeBytes;

  NrrdVolumeInfo()
    : depth(0), width(0), height(0), voxelType(_UChar),
      bytesPerVoxel(1), voxelSizeX(1), voxelSizeY(1), voxelSizeZ(1),
      voxelCount(0), volumeBytes(0)
  {
  }
};

struct NrrdLoadResult
{
  bool success;
  bool canceled;
  QString error;
  NrrdVolumeInfo info;
  std::shared_ptr<uchar> volume;
  float rawMin;
  float rawMax;
  QList<uint> histogram;

  NrrdLoadResult()
    : success(false), canceled(false), rawMin(0), rawMax(0)
  {
  }
};

struct NrrdHistogramResult
{
  bool success;
  bool canceled;
  QString error;
  QList<uint> histogram;

  NrrdHistogramResult() : success(false), canceled(false) {}
};

QString memoryAmount(quint64 bytes)
{
  const double mib = static_cast<double>(bytes)/(1024.0*1024.0);
  if (mib < 1024.0)
    return QStringLiteral("%1 MiB").arg(mib, 0, 'f', 1);
  return QStringLiteral("%1 GiB").arg(mib/1024.0, 0, 'f', 2);
}

QString memoryAdmissionError(const ImportMemoryAdmission &admission)
{
  switch (admission.reason)
    {
    case ImportMemoryAdmissionReason::MemoryStatusUnavailable:
      return QStringLiteral(
        "NRRD import stopped because available physical and commit memory "
        "could not be measured safely.");
    case ImportMemoryAdmissionReason::InsufficientPhysicalMemory:
      return QStringLiteral(
        "NRRD import needs about %1, but only %2 remains after reserving "
        "memory for Windows and integrated graphics.")
        .arg(memoryAmount(admission.requiredBytes),
             memoryAmount(admission.availablePhysicalBudgetBytes));
    case ImportMemoryAdmissionReason::InsufficientCommit:
      return QStringLiteral(
        "NRRD import needs about %1 of committed memory, but only %2 is "
        "available after the safety reserve.")
        .arg(memoryAmount(admission.requiredBytes),
             memoryAmount(admission.availableCommitBudgetBytes));
    case ImportMemoryAdmissionReason::AddressSpaceLimit:
      return QStringLiteral(
        "The NRRD volume is larger than this process can address.");
    case ImportMemoryAdmissionReason::ArithmeticOverflow:
    case ImportMemoryAdmissionReason::InvalidRequest:
      return QStringLiteral(
        "The NRRD volume size overflows the supported address space.");
    case ImportMemoryAdmissionReason::Approved:
      break;
    }
  return QStringLiteral("NRRD import was rejected by the memory safety check.");
}

bool componentLayout(itk::ImageIOBase::IOComponentType componentType,
                     int &voxelType,
                     int &bytesPerVoxel)
{
  switch (componentType)
    {
    case itk::ImageIOBase::UCHAR:
      voxelType = _UChar;
      bytesPerVoxel = 1;
      return true;
    case itk::ImageIOBase::CHAR:
      voxelType = _Char;
      bytesPerVoxel = 1;
      return true;
    case itk::ImageIOBase::USHORT:
      voxelType = _UShort;
      bytesPerVoxel = 2;
      return true;
    case itk::ImageIOBase::SHORT:
      voxelType = _Short;
      bytesPerVoxel = 2;
      return true;
    case itk::ImageIOBase::INT:
      voxelType = _Int;
      bytesPerVoxel = 4;
      return true;
    case itk::ImageIOBase::FLOAT:
      voxelType = _Float;
      bytesPerVoxel = 4;
      return true;
    default:
      return false;
    }
}

float validSpacing(double spacing)
{
  return std::isfinite(spacing) && spacing > 0.0 ?
    static_cast<float>(spacing) : 1.0f;
}

bool readVolumeInfo(const QString &fileName,
                    NrrdVolumeInfo &info,
                    QString &error)
{
  try
    {
      const QByteArray encodedName = QFile::encodeName(fileName);
      itk::NrrdImageIO::Pointer imageIO = itk::NrrdImageIO::New();
      if (!imageIO->CanReadFile(encodedName.constData()))
        {
          error = QStringLiteral("The selected file is not a readable NRRD volume.");
          return false;
        }

      imageIO->SetFileName(encodedName.constData());
      imageIO->ReadImageInformation();
      if (imageIO->GetNumberOfDimensions() != 3)
        {
          error = QStringLiteral("Drishti Import requires a three-dimensional NRRD volume.");
          return false;
        }
      if (imageIO->GetPixelType() != itk::ImageIOBase::SCALAR ||
          imageIO->GetNumberOfComponents() != 1)
        {
          error = QStringLiteral("Only scalar, single-component NRRD volumes are supported.");
          return false;
        }

      const itk::ImageIOBase::SizeValueType dimX = imageIO->GetDimensions(0);
      const itk::ImageIOBase::SizeValueType dimY = imageIO->GetDimensions(1);
      const itk::ImageIOBase::SizeValueType dimZ = imageIO->GetDimensions(2);
      const quint64 maxInt = static_cast<quint64>(
        std::numeric_limits<int>::max());
      if (dimX == 0 || dimY == 0 || dimZ == 0 ||
          static_cast<quint64>(dimX) > maxInt ||
          static_cast<quint64>(dimY) > maxInt ||
          static_cast<quint64>(dimZ) > maxInt)
        {
          error = QStringLiteral("The NRRD dimensions are empty or exceed Drishti's limits.");
          return false;
        }

      if (!componentLayout(imageIO->GetComponentType(),
                           info.voxelType, info.bytesPerVoxel))
        {
          error = QStringLiteral(
            "Supported NRRD component types are 8-bit and 16-bit signed or "
            "unsigned integers, signed 32-bit integers, and 32-bit floats.");
          return false;
        }

      info.height = static_cast<int>(dimX);
      info.width = static_cast<int>(dimY);
      info.depth = static_cast<int>(dimZ);
      info.voxelSizeX = validSpacing(imageIO->GetSpacing(0));
      info.voxelSizeY = validSpacing(imageIO->GetSpacing(1));
      info.voxelSizeZ = validSpacing(imageIO->GetSpacing(2));

      quint64 planeVoxels = 0;
      if (!checkedImportMultiply(static_cast<quint64>(info.width),
                                 static_cast<quint64>(info.height),
                                 planeVoxels) ||
          !checkedImportMultiply(static_cast<quint64>(info.depth),
                                 planeVoxels, info.voxelCount) ||
          !checkedImportMultiply(info.voxelCount,
                                 static_cast<quint64>(info.bytesPerVoxel),
                                 info.volumeBytes) ||
          info.volumeBytes > static_cast<quint64>(
            std::numeric_limits<std::size_t>::max()))
        {
          error = QStringLiteral("The NRRD volume size overflows the supported address space.");
          return false;
        }
      return true;
    }
  catch (const itk::ExceptionObject &exception)
    {
      error = QStringLiteral("Cannot read NRRD metadata: %1").arg(exception.what());
    }
  catch (const std::exception &exception)
    {
      error = QStringLiteral("Cannot read NRRD metadata: %1").arg(exception.what());
    }
  catch (...)
    {
      error = QStringLiteral("Cannot read NRRD metadata because of an unknown error.");
    }
  return false;
}

bool sameLayout(const NrrdVolumeInfo &first, const NrrdVolumeInfo &second)
{
  return first.depth == second.depth &&
         first.width == second.width &&
         first.height == second.height &&
         first.voxelType == second.voxelType &&
         first.bytesPerVoxel == second.bytesPerVoxel &&
         first.volumeBytes == second.volumeBytes;
}

void setProgress(std::atomic_int &progress,
                 quint64 completed,
                 quint64 total,
                 int start,
                 int end)
{
  if (total == 0)
    {
      progress.store(end);
      return;
    }
  const double fraction = static_cast<double>(completed)/
                          static_cast<double>(total);
  progress.store(qBound(start,
    start+static_cast<int>((end-start)*fraction), end));
}

void copyHistogram(const QVector<quint64> &source, QList<uint> &destination)
{
  destination.clear();
  destination.reserve(source.size());
  const quint64 maxCount = std::numeric_limits<uint>::max();
  for (quint64 count : source)
    destination.append(static_cast<uint>(qMin(count, maxCount)));
}

template <typename T>
bool exactStatistics(const uchar *buffer,
                     quint64 voxelCount,
                     qint64 minimumValue,
                     int binCount,
                     std::atomic_bool &cancelRequested,
                     std::atomic_int &progress,
                     NrrdLoadResult &result)
{
  const T *values = reinterpret_cast<const T*>(buffer);
  QVector<quint64> counts(binCount, 0);
  T rawMinimum = std::numeric_limits<T>::max();
  T rawMaximum = std::numeric_limits<T>::lowest();

  for (quint64 start = 0; start < voxelCount;
       start += kStatisticsChunkVoxels)
    {
      if (cancelRequested.load())
        {
          result.canceled = true;
          result.error = QStringLiteral("NRRD import canceled");
          return false;
        }
      const quint64 end = qMin(voxelCount, start+kStatisticsChunkVoxels);
      for (quint64 index = start; index < end; ++index)
        {
          const T value = values[static_cast<std::size_t>(index)];
          rawMinimum = qMin(rawMinimum, value);
          rawMaximum = qMax(rawMaximum, value);
          const qint64 histogramIndex = static_cast<qint64>(value)-minimumValue;
          if (histogramIndex < 0 || histogramIndex >= binCount)
            {
              result.error = QStringLiteral("A NRRD histogram index is outside its valid range.");
              return false;
            }
          ++counts[static_cast<int>(histogramIndex)];
        }
      setProgress(progress, end, voxelCount, 70, 100);
    }

  result.rawMin = static_cast<float>(rawMinimum);
  result.rawMax = static_cast<float>(rawMaximum);
  copyHistogram(counts, result.histogram);
  return true;
}

template <typename T>
bool mappedStatistics(const uchar *buffer,
                      quint64 voxelCount,
                      bool finiteOnly,
                      std::atomic_bool &cancelRequested,
                      std::atomic_int &progress,
                      NrrdLoadResult &result)
{
  const T *values = reinterpret_cast<const T*>(buffer);
  double rawMinimum = std::numeric_limits<double>::infinity();
  double rawMaximum = -std::numeric_limits<double>::infinity();

  for (quint64 start = 0; start < voxelCount;
       start += kStatisticsChunkVoxels)
    {
      if (cancelRequested.load())
        {
          result.canceled = true;
          result.error = QStringLiteral("NRRD import canceled");
          return false;
        }
      const quint64 end = qMin(voxelCount, start+kStatisticsChunkVoxels);
      for (quint64 index = start; index < end; ++index)
        {
          const double value = static_cast<double>(
            values[static_cast<std::size_t>(index)]);
          if (finiteOnly && !std::isfinite(value))
            continue;
          rawMinimum = qMin(rawMinimum, value);
          rawMaximum = qMax(rawMaximum, value);
        }
      setProgress(progress, end, voxelCount, 70, 85);
    }

  if (!std::isfinite(rawMinimum) || !std::isfinite(rawMaximum))
    {
      result.error = QStringLiteral("The NRRD volume contains no finite voxel values.");
      return false;
    }

  QVector<quint64> counts(65536, 0);
  const double range = rawMaximum-rawMinimum;
  for (quint64 start = 0; start < voxelCount;
       start += kStatisticsChunkVoxels)
    {
      if (cancelRequested.load())
        {
          result.canceled = true;
          result.error = QStringLiteral("NRRD import canceled");
          return false;
        }
      const quint64 end = qMin(voxelCount, start+kStatisticsChunkVoxels);
      for (quint64 index = start; index < end; ++index)
        {
          const double value = static_cast<double>(
            values[static_cast<std::size_t>(index)]);
          if (finiteOnly && !std::isfinite(value))
            continue;
          int histogramIndex = 0;
          if (range > 0.0)
            histogramIndex = qBound(0,
              static_cast<int>(65535.0*(value-rawMinimum)/range), 65535);
          ++counts[histogramIndex];
        }
      setProgress(progress, end, voxelCount, 85, 100);
    }

  result.rawMin = static_cast<float>(rawMinimum);
  result.rawMax = static_cast<float>(rawMaximum);
  copyHistogram(counts, result.histogram);
  return true;
}

bool calculateStatistics(const NrrdVolumeInfo &info,
                         const uchar *buffer,
                         std::atomic_bool &cancelRequested,
                         std::atomic_int &progress,
                         NrrdLoadResult &result)
{
  switch (info.voxelType)
    {
    case _UChar:
      return exactStatistics<uchar>(buffer, info.voxelCount, 0, 256,
                                    cancelRequested, progress, result);
    case _Char:
      return exactStatistics<signed char>(buffer, info.voxelCount, -128, 256,
                                          cancelRequested, progress, result);
    case _UShort:
      return exactStatistics<ushort>(buffer, info.voxelCount, 0, 65536,
                                     cancelRequested, progress, result);
    case _Short:
      return exactStatistics<short>(buffer, info.voxelCount, -32768, 65536,
                                    cancelRequested, progress, result);
    case _Int:
      return mappedStatistics<int>(buffer, info.voxelCount, false,
                                   cancelRequested, progress, result);
    case _Float:
      return mappedStatistics<float>(buffer, info.voxelCount, true,
                                     cancelRequested, progress, result);
    default:
      result.error = QStringLiteral("The NRRD voxel type is unsupported.");
      return false;
    }
}

template <typename T>
bool histogramForRange(const uchar *buffer,
                       quint64 voxelCount,
                       double rawMinimum,
                       double rawMaximum,
                       bool finiteOnly,
                       std::atomic_bool &cancelRequested,
                       std::atomic_int &progress,
                       NrrdHistogramResult &result)
{
  const T *values = reinterpret_cast<const T*>(buffer);
  QVector<quint64> counts(65536, 0);
  const double range = rawMaximum-rawMinimum;
  for (quint64 start = 0; start < voxelCount;
       start += kStatisticsChunkVoxels)
    {
      if (cancelRequested.load())
        {
          result.canceled = true;
          result.error = QStringLiteral("NRRD histogram generation canceled");
          return false;
        }
      const quint64 end = qMin(voxelCount, start+kStatisticsChunkVoxels);
      for (quint64 index = start; index < end; ++index)
        {
          const double value = static_cast<double>(
            values[static_cast<std::size_t>(index)]);
          if (finiteOnly && !std::isfinite(value))
            continue;
          int histogramIndex = 0;
          if (range > 0.0)
            histogramIndex = qBound(0,
              static_cast<int>(65535.0*(value-rawMinimum)/range), 65535);
          ++counts[histogramIndex];
        }
      setProgress(progress, end, voxelCount, 0, 100);
    }
  copyHistogram(counts, result.histogram);
  result.success = true;
  return true;
}
}

NrrdPlugin::NrrdPlugin()
{
  init();
}

NrrdPlugin::~NrrdPlugin() = default;

QStringList
NrrdPlugin::registerPlugin()
{
  return QStringList() << "files" << "NRRD Files";
}

void
NrrdPlugin::init()
{
  clear();
}

void
NrrdPlugin::clear()
{
  m_fileName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Millimeter;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
  m_entireVolume.reset();
  m_lastError.clear();
  m_lastOperationCanceled = false;
}

void NrrdPlugin::set4DVolume(bool flag) { m_4dvol = flag; }

void
NrrdPlugin::voxelSize(float &vx, float &vy, float &vz)
{
  vx = m_voxelSizeX;
  vy = m_voxelSizeY;
  vz = m_voxelSizeZ;
}

QString NrrdPlugin::description() { return m_description; }
int NrrdPlugin::voxelType() { return m_voxelType; }
int NrrdPlugin::voxelUnit() { return m_voxelUnit; }
int NrrdPlugin::headerBytes() { return m_headerBytes; }
float NrrdPlugin::rawMin() { return m_rawMin; }
float NrrdPlugin::rawMax() { return m_rawMax; }
QList<uint> NrrdPlugin::histogram() { return m_histogram; }
QString NrrdPlugin::lastError() const { return m_lastError; }
bool NrrdPlugin::wasCanceled() const { return m_lastOperationCanceled; }

void
NrrdPlugin::gridSize(int &depth, int &width, int &height)
{
  depth = m_depth;
  width = m_width;
  height = m_height;
}

bool
NrrdPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (files.size() != 1 || files.first().trimmed().isEmpty())
    {
      m_lastError = QStringLiteral("Select exactly one NRRD volume file.");
      return false;
    }

  const QString selectedFile = QFileInfo(files.first()).absoluteFilePath();
  const QFileInfo fileInfo(selectedFile);
  if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable())
    {
      m_lastError = QStringLiteral("The selected NRRD file is missing or unreadable: %1")
        .arg(selectedFile);
      return false;
    }

  NrrdVolumeInfo expectedInfo;
  if (!readVolumeInfo(selectedFile, expectedInfo, m_lastError))
    return false;

  quint64 histogramBytes = 0;
  quint64 requiredBytes = 0;
  if (!checkedImportMultiply(65536, sizeof(quint64), histogramBytes) ||
      !checkedImportAdd(expectedInfo.volumeBytes, histogramBytes,
                        requiredBytes) ||
      !checkedImportAdd(requiredBytes, kNrrdDecodeSafetyBytes,
                        requiredBytes))
    {
      m_lastError = QStringLiteral("The NRRD working-set calculation overflowed.");
      return false;
    }

  const ImportMemoryAdmission admission =
    evaluateImportMemoryAdmission(requiredBytes);
  if (!admission.approved)
    {
      m_lastError = memoryAdmissionError(admission);
      return false;
    }

  std::atomic_bool cancelRequested(false);
  std::atomic_int progressValue(0);
  QProgressDialog progress(QStringLiteral("Loading NRRD volume"),
                           QStringLiteral("Cancel"), 0, 100,
                           QApplication::activeWindow());
  progress.setWindowModality(Qt::ApplicationModal);
  progress.setMinimumDuration(0);
  progress.setAutoClose(false);
  progress.setAutoReset(false);
  progress.show();

  QEventLoop waitLoop;
  QTimer progressTimer;
  QFutureWatcher<NrrdLoadResult> watcher;
  QObject::connect(&progress, &QProgressDialog::canceled,
                   [&cancelRequested]() { cancelRequested.store(true); });
  QObject::connect(&progressTimer, &QTimer::timeout,
                   [&progress, &progressValue]()
                   { progress.setValue(progressValue.load()); });
  QObject::connect(&watcher, &QFutureWatcher<NrrdLoadResult>::finished,
                   &waitLoop, &QEventLoop::quit);

  const QFuture<NrrdLoadResult> future = QtConcurrent::run(
    [selectedFile, expectedInfo, &cancelRequested, &progressValue]()
    {
      NrrdLoadResult result;
      result.info = expectedInfo;
      try
        {
          if (cancelRequested.load())
            {
              result.canceled = true;
              result.error = QStringLiteral("NRRD import canceled");
              return result;
            }

          NrrdVolumeInfo currentInfo;
          if (!readVolumeInfo(selectedFile, currentInfo, result.error))
            return result;
          if (!sameLayout(expectedInfo, currentInfo))
            {
              result.error = QStringLiteral(
                "The NRRD file changed after its metadata was inspected.");
              return result;
            }
          progressValue.store(5);

          uchar *allocated = new (std::nothrow)
            uchar[static_cast<std::size_t>(expectedInfo.volumeBytes)];
          if (!allocated)
            {
              result.error = QStringLiteral(
                "NRRD import could not allocate the admitted volume buffer.");
              return result;
            }
          result.volume = std::shared_ptr<uchar>(
            allocated, std::default_delete<uchar[]>());
          progressValue.store(10);

          itk::NrrdImageIO::Pointer imageIO = itk::NrrdImageIO::New();
          const QByteArray encodedName = QFile::encodeName(selectedFile);
          imageIO->SetFileName(encodedName.constData());
          imageIO->ReadImageInformation();
          if (cancelRequested.load())
            {
              result.canceled = true;
              result.error = QStringLiteral("NRRD import canceled");
              return result;
            }

          imageIO->Read(result.volume.get());
          progressValue.store(70);
          if (cancelRequested.load())
            {
              result.canceled = true;
              result.error = QStringLiteral("NRRD import canceled");
              return result;
            }

          if (!calculateStatistics(expectedInfo, result.volume.get(),
                                   cancelRequested, progressValue, result))
            return result;

          result.success = true;
          progressValue.store(100);
        }
      catch (const itk::ExceptionObject &exception)
        {
          result.canceled = cancelRequested.load();
          result.error = result.canceled ?
            QStringLiteral("NRRD import canceled") :
            QStringLiteral("NRRD decoding failed: %1").arg(exception.what());
        }
      catch (const std::bad_alloc &)
        {
          result.error = QStringLiteral(
            "NRRD decoding exhausted memory despite the admission check.");
        }
      catch (const std::exception &exception)
        {
          result.error = QStringLiteral("NRRD decoding failed: %1")
            .arg(exception.what());
        }
      catch (...)
        {
          result.error = QStringLiteral(
            "NRRD decoding failed because of an unknown error.");
        }
      return result;
    });

  watcher.setFuture(future);
  progressTimer.start(50);
  waitLoop.exec();
  progressTimer.stop();
  // Closing QProgressDialog emits canceled(); hide it so a completed worker
  // cannot leave the cancellation flag set after a successful import.
  progress.hide();

  const NrrdLoadResult result = watcher.result();
  if (!result.success)
    {
      m_lastOperationCanceled = result.canceled;
      m_lastError = result.error.isEmpty() ?
        QStringLiteral("The NRRD decoder rejected the selected volume.") :
        result.error;
      return false;
    }

  m_fileName = QStringList() << selectedFile;
  m_description = QStringLiteral("NRRD volume");
  m_depth = result.info.depth;
  m_width = result.info.width;
  m_height = result.info.height;
  m_voxelType = result.info.voxelType;
  m_bytesPerVoxel = result.info.bytesPerVoxel;
  m_voxelUnit = _Millimeter;
  m_voxelSizeX = result.info.voxelSizeX;
  m_voxelSizeY = result.info.voxelSizeY;
  m_voxelSizeZ = result.info.voxelSizeZ;
  m_skipBytes = 0;
  m_headerBytes = 0;
  m_rawMin = result.rawMin;
  m_rawMax = result.rawMax;
  m_histogram = result.histogram;
  m_entireVolume = result.volume;
  return true;
}

void
NrrdPlugin::replaceFile(QString fileName)
{
  setFile(QStringList() << fileName);
}

void
NrrdPlugin::setMinMax(float rawMinimum, float rawMaximum)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (!std::isfinite(rawMinimum) || !std::isfinite(rawMaximum) ||
      rawMinimum > rawMaximum)
    {
      m_lastError = QStringLiteral("The requested NRRD histogram range is invalid.");
      return;
    }

  const float previousRawMinimum = m_rawMin;
  const float previousRawMaximum = m_rawMax;
  const QList<uint> previousHistogram = m_histogram;
  m_rawMin = rawMinimum;
  m_rawMax = rawMaximum;
  if (m_voxelType == _Int || m_voxelType == _Float)
    {
      generateHistogram();
      if (!m_lastError.isEmpty() || m_lastOperationCanceled)
        {
          m_rawMin = previousRawMinimum;
          m_rawMax = previousRawMaximum;
          m_histogram = previousHistogram;
        }
    }
}

void
NrrdPlugin::generateHistogram()
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (!m_entireVolume || m_depth <= 0 || m_width <= 0 || m_height <= 0)
    {
      m_lastError = QStringLiteral("No NRRD volume is loaded.");
      return;
    }
  if (m_voxelType != _Int && m_voxelType != _Float)
    return;
  if (!std::isfinite(m_rawMin) || !std::isfinite(m_rawMax) ||
      m_rawMax < m_rawMin)
    {
      m_lastError = QStringLiteral("The requested NRRD histogram range is invalid.");
      return;
    }

  quint64 planeVoxels = 0;
  quint64 voxelCount = 0;
  if (!checkedImportMultiply(static_cast<quint64>(m_width),
                             static_cast<quint64>(m_height), planeVoxels) ||
      !checkedImportMultiply(static_cast<quint64>(m_depth),
                             planeVoxels, voxelCount))
    {
      m_lastError = QStringLiteral("The NRRD histogram size overflowed.");
      return;
    }

  std::atomic_bool cancelRequested(false);
  std::atomic_int progressValue(0);
  QProgressDialog progress(QStringLiteral("Generating NRRD histogram"),
                           QStringLiteral("Cancel"), 0, 100,
                           QApplication::activeWindow());
  progress.setWindowModality(Qt::ApplicationModal);
  progress.setMinimumDuration(0);
  progress.setAutoClose(false);
  progress.setAutoReset(false);

  QEventLoop waitLoop;
  QTimer progressTimer;
  QFutureWatcher<NrrdHistogramResult> watcher;
  QObject::connect(&progress, &QProgressDialog::canceled,
                   [&cancelRequested]() { cancelRequested.store(true); });
  QObject::connect(&progressTimer, &QTimer::timeout,
                   [&progress, &progressValue]()
                   { progress.setValue(progressValue.load()); });
  QObject::connect(&watcher, &QFutureWatcher<NrrdHistogramResult>::finished,
                   &waitLoop, &QEventLoop::quit);

  const std::shared_ptr<uchar> volume = m_entireVolume;
  const int voxelType = m_voxelType;
  const double rawMinimum = m_rawMin;
  const double rawMaximum = m_rawMax;
  const QFuture<NrrdHistogramResult> future = QtConcurrent::run(
    [volume, voxelType, voxelCount, rawMinimum, rawMaximum,
     &cancelRequested, &progressValue]()
    {
      NrrdHistogramResult result;
      if (voxelType == _Int)
        histogramForRange<int>(volume.get(), voxelCount,
                               rawMinimum, rawMaximum, false,
                               cancelRequested, progressValue, result);
      else
        histogramForRange<float>(volume.get(), voxelCount,
                                 rawMinimum, rawMaximum, true,
                                 cancelRequested, progressValue, result);
      return result;
    });

  watcher.setFuture(future);
  progressTimer.start(50);
  waitLoop.exec();
  progressTimer.stop();
  // Keep programmatic completion distinct from a user pressing Cancel.
  progress.hide();

  const NrrdHistogramResult result = watcher.result();
  if (!result.success)
    {
      m_lastOperationCanceled = result.canceled;
      m_lastError = result.error;
      return;
    }
  m_histogram = result.histogram;
}

void
NrrdPlugin::getDepthSlice(int sliceIndex, uchar *slice)
{
  m_lastError.clear();
  quint64 sliceVoxels = 0;
  quint64 sliceBytes = 0;
  quint64 offset = 0;
  if (!slice || !m_entireVolume || sliceIndex < 0 || sliceIndex >= m_depth ||
      !checkedImportMultiply(static_cast<quint64>(m_width),
                             static_cast<quint64>(m_height), sliceVoxels) ||
      !checkedImportMultiply(sliceVoxels,
                             static_cast<quint64>(m_bytesPerVoxel),
                             sliceBytes) ||
      !checkedImportMultiply(static_cast<quint64>(sliceIndex),
                             sliceBytes, offset) ||
      sliceBytes > static_cast<quint64>(
        std::numeric_limits<std::size_t>::max()) ||
      offset > static_cast<quint64>(
        std::numeric_limits<std::size_t>::max()))
    {
      m_lastError = QStringLiteral("NRRD slice %1 is invalid.").arg(sliceIndex);
      return;
    }

  std::memcpy(slice,
              m_entireVolume.get()+static_cast<std::size_t>(offset),
              static_cast<std::size_t>(sliceBytes));
}

QVariant
NrrdPlugin::rawValue(int depth, int width, int height)
{
  if (!m_entireVolume || depth < 0 || depth >= m_depth ||
      width < 0 || width >= m_width ||
      height < 0 || height >= m_height)
    return QVariant(QStringLiteral("OutOfBounds"));

  quint64 planeVoxels = 0;
  quint64 voxelIndex = 0;
  if (!checkedImportMultiply(static_cast<quint64>(m_width),
                             static_cast<quint64>(m_height), planeVoxels) ||
      !checkedImportMultiply(static_cast<quint64>(depth),
                             planeVoxels, voxelIndex) ||
      !checkedImportAdd(voxelIndex,
                        static_cast<quint64>(width)*m_height+height,
                        voxelIndex))
    return QVariant(QStringLiteral("ReadError"));

  const uchar *address = m_entireVolume.get()+
    static_cast<std::size_t>(voxelIndex)*m_bytesPerVoxel;
  if (m_voxelType == _UChar)
    return QVariant(static_cast<uint>(*address));
  if (m_voxelType == _Char)
    {
      signed char value = 0;
      std::memcpy(&value, address, sizeof(value));
      return QVariant(static_cast<int>(value));
    }
  if (m_voxelType == _UShort)
    {
      ushort value = 0;
      std::memcpy(&value, address, sizeof(value));
      return QVariant(static_cast<uint>(value));
    }
  if (m_voxelType == _Short)
    {
      short value = 0;
      std::memcpy(&value, address, sizeof(value));
      return QVariant(static_cast<int>(value));
    }
  if (m_voxelType == _Int)
    {
      int value = 0;
      std::memcpy(&value, address, sizeof(value));
      return QVariant(value);
    }
  if (m_voxelType == _Float)
    {
      float value = 0;
      std::memcpy(&value, address, sizeof(value));
      return QVariant(static_cast<double>(value));
    }
  return QVariant(QStringLiteral("ReadError"));
}
