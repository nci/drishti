#include <QtGui>
#include "common.h"
#include "dicomplugin.h"
#include "dicomhistogramutils.h"
#include "importmemoryadmission.h"

#include <QApplication>
#include <QEventLoop>
#include <QFutureWatcher>
#include <QProgressDialog>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>

#include <QtConcurrent>

#include "itkCommand.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkImage.h"
#include "itkImageSeriesReader.h"
#include "itkProcessObject.h"

#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <vector>

namespace
{
typedef itk::GDCMImageIO DicomImageIOType;
typedef std::vector<std::string> DicomFileNames;

const quint64 kDicomDecodeSafetyBytes = 256ULL*1024ULL*1024ULL;

struct DicomProgressContext
{
  std::atomic_bool *cancelRequested;
  std::atomic_int *progressValue;
};

void updateDicomProgress(itk::Object *caller,
                         const itk::EventObject &,
                         void *clientData)
{
  DicomProgressContext *context =
    static_cast<DicomProgressContext*>(clientData);
  itk::ProcessObject *process = dynamic_cast<itk::ProcessObject*>(caller);
  if (!context || !process)
    return;

  context->progressValue->store(
    qBound(0, static_cast<int>(70.0*process->GetProgress()), 70));
  if (context->cancelRequested->load())
    process->AbortGenerateDataOn();
}

QString dicomMemoryAmount(quint64 bytes)
{
  const double mib = static_cast<double>(bytes)/(1024.0*1024.0);
  if (mib < 1024.0)
    return QStringLiteral("%1 MiB").arg(mib, 0, 'f', 1);
  return QStringLiteral("%1 GiB").arg(mib/1024.0, 0, 'f', 2);
}

struct DicomLoadResult
{
  bool success;
  bool canceled;
  QString error;
  itk::DataObject::Pointer image;
  int voxelType;
  int bytesPerVoxel;
  int depth;
  int width;
  int height;
  float voxelSizeX;
  float voxelSizeY;
  float voxelSizeZ;
  float rawMin;
  float rawMax;
  QList<uint> histogram;

  DicomLoadResult()
    : success(false), canceled(false),
      voxelType(_UChar), bytesPerVoxel(1),
      depth(0), width(0), height(0),
      voxelSizeX(1), voxelSizeY(1), voxelSizeZ(1),
      rawMin(0), rawMax(0)
  {
  }
};

template <typename PixelType>
bool loadDicomSeries(const DicomFileNames &dcmFiles,
                     DicomImageIOType::Pointer imageIO,
                     int voxelType,
                     std::atomic_bool &cancelRequested,
                     std::atomic_int &progressValue,
                     DicomLoadResult &result)
{
  typedef itk::Image<PixelType, 3> ImageType;
  typedef itk::ImageSeriesReader<ImageType> ReaderType;

  typename ReaderType::Pointer reader = ReaderType::New();
  reader->SetImageIO(imageIO);
  reader->SetFileNames(dcmFiles);

  DicomProgressContext progressContext =
    { &cancelRequested, &progressValue };
  itk::CStyleCommand::Pointer progressCommand = itk::CStyleCommand::New();
  progressCommand->SetClientData(&progressContext);
  progressCommand->SetCallback(updateDicomProgress);
  reader->AddObserver(itk::ProgressEvent(), progressCommand);

  reader->UpdateOutputInformation();
  const typename ImageType::SizeType imageSize =
    reader->GetOutput()->GetLargestPossibleRegion().GetSize();
  if (imageSize[0] == 0 || imageSize[1] == 0 || imageSize[2] == 0 ||
      imageSize[0] > static_cast<typename ImageType::SizeType::SizeValueType>(
                       std::numeric_limits<int>::max()) ||
      imageSize[1] > static_cast<typename ImageType::SizeType::SizeValueType>(
                       std::numeric_limits<int>::max()) ||
      imageSize[2] > static_cast<typename ImageType::SizeType::SizeValueType>(
                       std::numeric_limits<int>::max()))
    {
      result.error = "The DICOM grid dimensions are invalid or too large.";
      return false;
    }

  quint64 voxelCount = 0;
  quint64 volumeBytes = 0;
  quint64 requiredBytes = 0;
  if (!checkedImportMultiply(static_cast<quint64>(imageSize[0]),
                             static_cast<quint64>(imageSize[1]),
                             voxelCount) ||
      !checkedImportMultiply(voxelCount,
                             static_cast<quint64>(imageSize[2]),
                             voxelCount) ||
      !checkedImportMultiply(voxelCount,
                             static_cast<quint64>(sizeof(PixelType)),
                             volumeBytes) ||
      !checkedImportMultiply(volumeBytes, 2, requiredBytes) ||
      !checkedImportAdd(requiredBytes, kDicomDecodeSafetyBytes,
                        requiredBytes) ||
      voxelCount > static_cast<quint64>(
                     std::numeric_limits<std::size_t>::max()))
    {
      result.error =
        "The DICOM volume size overflows the supported address space.";
      return false;
    }

  const ImportMemoryAdmission admission =
    evaluateImportMemoryAdmission(requiredBytes);
  if (!admission.approved)
    {
      result.error = QString(
        "DICOM decoding was stopped before allocating the full volume. "
        "Required peak increment: %1; usable physical budget: %2; "
        "usable Commit budget: %3.")
        .arg(dicomMemoryAmount(admission.requiredBytes),
             admission.physicalMemoryChecked ?
               dicomMemoryAmount(admission.availablePhysicalBudgetBytes) :
               QStringLiteral("unavailable"),
             admission.commitMemoryChecked ?
               dicomMemoryAmount(admission.availableCommitBudgetBytes) :
               QStringLiteral("unavailable"));
      return false;
    }

  if (cancelRequested.load())
    {
      result.canceled = true;
      result.error = "DICOM import canceled";
      return false;
    }

  reader->Update();
  if (cancelRequested.load() || reader->GetAbortGenerateData())
    {
      result.canceled = true;
      result.error = "DICOM import canceled";
      return false;
    }

  typename ImageType::Pointer image = reader->GetOutput();
  const PixelType *pixels = image ? image->GetBufferPointer() : 0;
  if (!pixels)
    {
      result.error = "The DICOM decoder returned no voxel buffer.";
      return false;
    }

  bool haveFiniteValue = false;
  long double minimum = 0;
  long double maximum = 0;
  for (quint64 index=0; index<voxelCount; ++index)
    {
      if ((index & ((1ULL << 20)-1)) == 0)
        {
          if (cancelRequested.load())
            {
              result.canceled = true;
              result.error = "DICOM import canceled";
              return false;
            }
          progressValue.store(70 + static_cast<int>(
            15.0L*static_cast<long double>(index)/
            static_cast<long double>(voxelCount)));
        }

      const long double value = static_cast<long double>(
        pixels[static_cast<std::size_t>(index)]);
      if (!std::isfinite(static_cast<double>(value)))
        continue;
      if (!haveFiniteValue)
        {
          minimum = maximum = value;
          haveFiniteValue = true;
        }
      else
        {
          minimum = qMin(minimum, value);
          maximum = qMax(maximum, value);
        }
    }
  if (!haveFiniteValue)
    {
      result.error = "The DICOM volume contains no finite scalar values.";
      return false;
    }

  const int histogramSize =
    (voxelType == _UChar || voxelType == _Char) ? 256 : 65536;
  result.histogram.clear();
  result.histogram.reserve(histogramSize);
  for (int bin=0; bin<histogramSize; ++bin)
    result.histogram.append(0);
  const uint maxHistogramCount = std::numeric_limits<uint>::max();
  for (quint64 index=0; index<voxelCount; ++index)
    {
      if ((index & ((1ULL << 20)-1)) == 0)
        {
          if (cancelRequested.load())
            {
              result.canceled = true;
              result.error = "DICOM import canceled";
              return false;
            }
          progressValue.store(85 + static_cast<int>(
            15.0L*static_cast<long double>(index)/
            static_cast<long double>(voxelCount)));
        }

      const PixelType pixel = pixels[static_cast<std::size_t>(index)];
      const long double value = static_cast<long double>(pixel);
      if (!std::isfinite(static_cast<double>(value)))
        continue;

      int histogramIndex = 0;
      if (voxelType == _UChar)
        histogramIndex = static_cast<unsigned char>(pixel);
      else if (voxelType == _Char)
        histogramIndex = static_cast<int>(static_cast<signed char>(pixel)) + 128;
      else if (voxelType == _UShort)
        histogramIndex = static_cast<unsigned short>(pixel);
      else if (voxelType == _Short)
        histogramIndex = DicomHistogramUtils::signedShortIndex(
          static_cast<short>(pixel));
      else if (maximum > minimum)
        {
          const long double fraction = (value-minimum)/(maximum-minimum);
          histogramIndex = static_cast<int>(fraction*65535.0L);
        }
      histogramIndex = qBound(0, histogramIndex, histogramSize-1);
      uint &count = result.histogram[histogramIndex];
      if (count < maxHistogramCount)
        ++count;
    }

  const typename ImageType::SpacingType spacing = image->GetSpacing();
  image->DisconnectPipeline();
  result.image = image;
  result.voxelType = voxelType;
  result.bytesPerVoxel = static_cast<int>(sizeof(PixelType));
  result.depth = static_cast<int>(imageSize[2]);
  result.width = static_cast<int>(imageSize[1]);
  result.height = static_cast<int>(imageSize[0]);
  result.voxelSizeX = spacing[0] > 0 ? spacing[0] : 1;
  result.voxelSizeY = spacing[1] > 0 ? spacing[1] : 1;
  result.voxelSizeZ = spacing[2] > 0 ? spacing[2] : 1;
  result.rawMin = static_cast<float>(minimum);
  result.rawMax = static_cast<float>(maximum);
  result.success = true;
  progressValue.store(100);
  return true;
}

template <typename PixelType>
const PixelType *dicomPixels(itk::DataObject *data)
{
  typedef itk::Image<PixelType, 3> ImageType;
  ImageType *image = dynamic_cast<ImageType*>(data);
  return image ? image->GetBufferPointer() : 0;
}

const void *dicomPixelBuffer(itk::DataObject *data, int voxelType)
{
  if (voxelType == _UChar) return dicomPixels<unsigned char>(data);
  if (voxelType == _Char) return dicomPixels<signed char>(data);
  if (voxelType == _UShort) return dicomPixels<unsigned short>(data);
  if (voxelType == _Short) return dicomPixels<short>(data);
  if (voxelType == _Int) return dicomPixels<int>(data);
  if (voxelType == _Float) return dicomPixels<float>(data);
  return 0;
}

struct DicomDiscoveryResult
{
  bool success;
  bool canceled;
  QString error;
  QString directory;
  QStringList seriesUids;

  DicomDiscoveryResult() : success(false), canceled(false) {}
};
}

void DicomPlugin::generateHistogram() {} // to satisfy the interface

QStringList
DicomPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "DICOM Image Directory";
  //  regString << "files";
  //  regString << "DICOM Image Files";
  
  return regString;
}

void
DicomPlugin::init()
{
  m_dimg = 0;

  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_lastError.clear();
  m_lastOperationCanceled = false;
  m_4dvol = false;
}

void
DicomPlugin::clear()
{
  m_dimg = 0;

  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_lastError.clear();
  m_lastOperationCanceled = false;
  m_4dvol = false;
}

void
DicomPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
DicomPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString DicomPlugin::description() { return m_description; }
int DicomPlugin::voxelType() { return m_voxelType; }
int DicomPlugin::voxelUnit() { return m_voxelUnit; }
int DicomPlugin::headerBytes() { return m_headerBytes; }

void
DicomPlugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
}
float DicomPlugin::rawMin() { return m_rawMin; }
float DicomPlugin::rawMax() { return m_rawMax; }
QList<uint> DicomPlugin::histogram() { return m_histogram; }
QString DicomPlugin::lastError() const { return m_lastError; }
bool DicomPlugin::wasCanceled() const { return m_lastOperationCanceled; }

void
DicomPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
DicomPlugin::replaceFile(QString flnm)
{
  Q_UNUSED(flnm);
  m_lastOperationCanceled = false;
  m_lastError = "DICOM time-series replacement is unsupported. "
                "Convert each series to RAW before merging volumes.";
}

bool
DicomPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (files.isEmpty())
    {
      m_lastError = "No DICOM directory was selected.";
      return false;
    }

  const QString selectedDirectory = QFileInfo(files[0]).absoluteFilePath();
  if (!QFileInfo(selectedDirectory).isDir())
    {
      m_lastError = "The selected DICOM input is not a readable directory.";
      return false;
    }

  std::atomic_bool discoveryCancelRequested(false);
  QProgressDialog discoveryProgress("Scanning DICOM directory", "Cancel",
                                    0, 0, QApplication::activeWindow());
  discoveryProgress.setWindowModality(Qt::ApplicationModal);
  discoveryProgress.setMinimumDuration(0);
  discoveryProgress.setAutoClose(false);
  discoveryProgress.setAutoReset(false);

  QEventLoop discoveryWaitLoop;
  QFutureWatcher<DicomDiscoveryResult> discoveryWatcher;
  QObject::connect(&discoveryProgress, &QProgressDialog::canceled,
                   [&]()
                   {
                     discoveryCancelRequested.store(true);
                     QTimer::singleShot(0, &discoveryProgress, [&]()
                     {
                       discoveryProgress.setLabelText(
                         "Cancel requested; finishing the current DICOM scan");
                       discoveryProgress.setCancelButton(0);
                       discoveryProgress.show();
                     });
                   });
  QObject::connect(&discoveryWatcher,
                   &QFutureWatcher<DicomDiscoveryResult>::finished,
                   &discoveryWaitLoop, &QEventLoop::quit);

  discoveryProgress.show();
  QApplication::processEvents();

  QFuture<DicomDiscoveryResult> discoveryFuture = QtConcurrent::run(
    [selectedDirectory, &discoveryCancelRequested]()
    {
      DicomDiscoveryResult result;
      result.directory = selectedDirectory;
      try
        {
          typedef itk::GDCMSeriesFileNames NamesGeneratorType;
          NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
          nameGenerator->SetUseSeriesDetails(true);
          nameGenerator->SetRecursive(true);
          const QByteArray encodedDirectory = QFile::encodeName(result.directory);
          nameGenerator->SetDirectory(encodedDirectory.constData());

          typedef std::vector<std::string> SeriesIdContainer;
          const SeriesIdContainer &seriesUids = nameGenerator->GetSeriesUIDs();
          if (discoveryCancelRequested.load())
            {
              result.canceled = true;
              result.error = "DICOM import canceled";
              return result;
            }
          for (SeriesIdContainer::const_iterator uid=seriesUids.begin();
               uid != seriesUids.end(); ++uid)
            result.seriesUids.append(QString::fromUtf8(uid->c_str()));

          if (result.seriesUids.isEmpty())
            {
              result.error =
                "No DICOM series was found in the selected directory.";
              return result;
            }
          result.success = true;
        }
      catch (const itk::ExceptionObject &error)
        {
          result.error = QString("Cannot inspect the DICOM series: %1")
                           .arg(error.what());
        }
      catch (const std::exception &error)
        {
          result.error = QString("Cannot inspect the DICOM series: %1")
                           .arg(error.what());
        }
      catch (...)
        {
          result.error =
            "Cannot inspect the DICOM series because of an unknown error.";
        }
      return result;
    });

  discoveryWatcher.setFuture(discoveryFuture);
  discoveryWaitLoop.exec();
  // QProgressDialog::close() emits canceled(), which would turn a completed
  // background scan into a user cancellation through the connection above.
  discoveryProgress.hide();

  const DicomDiscoveryResult discovery = discoveryWatcher.result();
  if (discovery.success && discoveryCancelRequested.load())
    {
      m_lastOperationCanceled = true;
      m_lastError = "DICOM import canceled";
      return false;
    }
  if (!discovery.success)
    {
      m_lastOperationCanceled = discovery.canceled;
      m_lastError = discovery.error;
      return false;
    }

  const QString flnm0 = discovery.directory;
  QString selectedSeriesUid;
  if (discovery.seriesUids.size() == 1)
    selectedSeriesUid = discovery.seriesUids.first();
  else
    {
      bool accepted = false;
      selectedSeriesUid = QInputDialog::getItem(
        0, "Choose a series for extraction", "Series",
        discovery.seriesUids, 0, false, &accepted);
      if (!accepted)
        {
          m_lastOperationCanceled = true;
          m_lastError = "DICOM import canceled";
          return false;
        }
      if (!discovery.seriesUids.contains(selectedSeriesUid))
        {
          m_lastError = "The selected DICOM series is unavailable.";
          return false;
        }
    }

  const std::string seriesIdentifier = selectedSeriesUid.toStdString();

      std::atomic_bool cancelRequested(false);
      std::atomic_int progressValue(0);
      QProgressDialog progress("Loading DICOM series", "Cancel", 0, 100,
                               QApplication::activeWindow());
      progress.setWindowModality(Qt::ApplicationModal);
      progress.setMinimumDuration(0);
      progress.setAutoClose(false);
      progress.setAutoReset(false);

      QEventLoop waitLoop;
      QTimer progressTimer;
      QFutureWatcher<DicomLoadResult> watcher;
      QObject::connect(&progress, &QProgressDialog::canceled,
                       [&cancelRequested]() { cancelRequested.store(true); });
      QObject::connect(&watcher, &QFutureWatcher<DicomLoadResult>::finished,
                       &waitLoop, &QEventLoop::quit);
      QObject::connect(&progressTimer, &QTimer::timeout,
                       [&]() { progress.setValue(progressValue.load()); });

      QFuture<DicomLoadResult> future = QtConcurrent::run(
        [flnm0, seriesIdentifier, &cancelRequested, &progressValue]()
        {
          DicomLoadResult result;
          try
            {
              if (cancelRequested.load())
                {
                  result.canceled = true;
                  result.error = "DICOM import canceled";
                  return result;
                }

              typedef itk::GDCMSeriesFileNames NamesGeneratorType;
              NamesGeneratorType::Pointer nameGenerator =
                NamesGeneratorType::New();
              nameGenerator->SetUseSeriesDetails(true);
              nameGenerator->SetRecursive(true);
              const QByteArray encodedDirectory = QFile::encodeName(flnm0);
              nameGenerator->SetDirectory(encodedDirectory.constData());
              const DicomFileNames dcmFiles =
                nameGenerator->GetFileNames(seriesIdentifier);
              if (cancelRequested.load())
                {
                  result.canceled = true;
                  result.error = "DICOM import canceled";
                  return result;
                }
              if (dcmFiles.empty())
                {
                  result.error =
                    "The selected DICOM series does not contain readable files.";
                  return result;
                }

              DicomImageIOType::Pointer imageIO = DicomImageIOType::New();
              imageIO->SetFileName(dcmFiles.front());
              imageIO->ReadImageInformation();
              if (imageIO->GetPixelType() != itk::ImageIOBase::SCALAR ||
                  imageIO->GetNumberOfComponents() != 1)
                {
                  result.error =
                    "Only scalar DICOM image series are supported; color or "
                    "multi-component data was rejected without conversion.";
                  return result;
                }

              const itk::ImageIOBase::IOComponentType componentType =
                imageIO->GetComponentType();
              if (componentType == itk::ImageIOBase::UCHAR)
                loadDicomSeries<unsigned char>(dcmFiles, imageIO, _UChar,
                                               cancelRequested, progressValue,
                                               result);
              else if (componentType == itk::ImageIOBase::CHAR)
                loadDicomSeries<signed char>(dcmFiles, imageIO, _Char,
                                             cancelRequested, progressValue,
                                             result);
              else if (componentType == itk::ImageIOBase::USHORT)
                loadDicomSeries<unsigned short>(dcmFiles, imageIO, _UShort,
                                                cancelRequested, progressValue,
                                                result);
              else if (componentType == itk::ImageIOBase::SHORT)
                loadDicomSeries<short>(dcmFiles, imageIO, _Short,
                                       cancelRequested, progressValue, result);
              else if (componentType == itk::ImageIOBase::INT)
                loadDicomSeries<int>(dcmFiles, imageIO, _Int,
                                     cancelRequested, progressValue, result);
              else if (componentType == itk::ImageIOBase::FLOAT)
                loadDicomSeries<float>(dcmFiles, imageIO, _Float,
                                       cancelRequested, progressValue, result);
              else
                {
                  result.error = QString(
                    "DICOM scalar component type '%1' has no lossless Drishti "
                    "voxel representation and was rejected before decoding.")
                    .arg(QString::fromStdString(
                      itk::ImageIOBase::GetComponentTypeAsString(componentType)));
                }
            }
          catch (const itk::ExceptionObject &error)
            {
              result.canceled = cancelRequested.load();
              result.error = result.canceled ?
                QStringLiteral("DICOM import canceled") :
                QString("DICOM decoding failed: %1").arg(error.what());
            }
          catch (const std::exception &error)
            {
              result.error = QString("DICOM decoding failed: %1").arg(error.what());
            }
          catch (...)
            {
              result.error = "DICOM decoding failed with an unknown error.";
            }
          return result;
        });

      watcher.setFuture(future);
      progressTimer.start(50);
      waitLoop.exec();
      progressTimer.stop();
      // Hiding avoids QProgressDialog::close() emitting canceled() after a
      // successful load.  A real Cancel-button click still sets the flag.
      progress.hide();

      const DicomLoadResult loadResult = watcher.result();
      if (loadResult.success && cancelRequested.load())
        {
          m_lastOperationCanceled = true;
          m_lastError = "DICOM import canceled";
          return false;
        }
      if (!loadResult.success)
        {
          m_lastOperationCanceled = loadResult.canceled;
          m_lastError = loadResult.error;
          return false;
        }



//      //-----------------
//      {
//	QTextEdit *tedit = new QTextEdit();
//	typedef itk::MetaDataDictionary   DictionaryType;
//	const  DictionaryType & dictionary = dicomIO->GetMetaDataDictionary();
//	typedef itk::MetaDataObject< std::string > MetaDataStringType;
//	DictionaryType::ConstIterator itr = dictionary.Begin();
//	DictionaryType::ConstIterator end = dictionary.End();	
//	while( itr != end )
//	  {
//	    itk::MetaDataObjectBase::Pointer  entry = itr->second;
//	    MetaDataStringType::Pointer entryvalue =
//	      dynamic_cast<MetaDataStringType *>( entry.GetPointer() );
//	    if( entryvalue )
//	      {
//		std::string tagkey   = itr->first;
//		std::string tagvalue = entryvalue->GetMetaDataObjectValue();
//		tedit->insertPlainText(QString(tagkey.c_str()) + " = " + QString(tagvalue.c_str()) + "\n");
//	      }
//	    ++itr;
//	  }
//
//	
//	std::string tagkey;
//	std::string labelId;
//	std::string tagvalue;
//	{
//	  tagkey = "0028|0030"; // pixel spacing
//	  if (itk::GDCMImageIO::GetLabelFromTag(tagkey, labelId))
//	    {
//	      if (dicomIO->GetValueFromTag(tagkey, tagvalue))
//		{
//		  tedit->insertPlainText(QString(labelId.c_str()) + " = " + QString(tagvalue.c_str()) + "\n");
//		}
//	      else
//		{
//		  tedit->insertPlainText("\n\Pixel Spacing not available\n\n");
//		}
//	    }
//	  //}
//	  //{
//	  tagkey = "0018|0050"; // slice thickness
//	  if (itk::GDCMImageIO::GetLabelFromTag(tagkey, labelId))
//	    {
//	      if (dicomIO->GetValueFromTag(tagkey, tagvalue))
//		{
//		  tedit->insertPlainText(QString(labelId.c_str()) + " = " + QString(tagvalue.c_str()) + "\n");
//		}
//	      else
//		{
//		  tedit->insertPlainText("\n\nSlice Thickness not available\n\n");
//		}
//	    }
//	  //}	
//	  //{
//	  tagkey = "0020|0032"; // upper left corner
//	  if (itk::GDCMImageIO::GetLabelFromTag(tagkey, labelId))
//	    {
//	      if (dicomIO->GetValueFromTag(tagkey, tagvalue))
//		{
//		  tedit->insertPlainText(QString(labelId.c_str()) + " = " + QString(tagvalue.c_str()) + "\n");
//		}
//	      else
//		{
//		  tedit->insertPlainText("\n\Upper Left Corner not available\n\n");
//		}
//	    }	
//	}
//	
//	  
//	QVBoxLayout *layout = new QVBoxLayout();
//	layout->addWidget(tedit);
//	
//	QDialog *showMD = new QDialog();
//	showMD->setWindowTitle("Series MetaData");
//	showMD->setSizeGripEnabled(true);
//	showMD->setModal(true);
//	showMD->setLayout(layout);
//	showMD->exec();
//      }
//      //-----------------



      
      m_dimg = loadResult.image;
      m_voxelType = loadResult.voxelType;
      m_depth = loadResult.depth;
      m_width = loadResult.width;
      m_height = loadResult.height;
      m_voxelSizeX = loadResult.voxelSizeX;
      m_voxelSizeY = loadResult.voxelSizeY;
      m_voxelSizeZ = loadResult.voxelSizeZ;
      m_voxelUnit = _Millimeter;
      m_headerBytes = 0;
      m_bytesPerVoxel = loadResult.bytesPerVoxel;
      m_rawMin = loadResult.rawMin;
      m_rawMax = loadResult.rawMax;
      m_histogram = loadResult.histogram;
      m_fileName = QStringList() << flnm0;
      m_imageList.clear();

  return true;
}

void
DicomPlugin::getDepthSlice(int slc,
			   uchar *slice)
{
  m_lastError.clear();
  if (!slice || !m_dimg || slc < 0 || slc >= m_depth)
    {
      m_lastError = QString("DICOM slice %1 is invalid.").arg(slc);
      return;
    }

  quint64 slicePixels = 0;
  quint64 sliceBytes = 0;
  quint64 sliceOffset = 0;
  quint64 byteOffset = 0;
  if (!checkedImportMultiply(static_cast<quint64>(m_width),
                             static_cast<quint64>(m_height), slicePixels) ||
      !checkedImportMultiply(slicePixels,
                             static_cast<quint64>(m_bytesPerVoxel),
                             sliceBytes) ||
      !checkedImportMultiply(static_cast<quint64>(slc),
                             slicePixels, sliceOffset) ||
      !checkedImportMultiply(sliceOffset,
                             static_cast<quint64>(m_bytesPerVoxel),
                             byteOffset) ||
      sliceBytes > static_cast<quint64>(
                     std::numeric_limits<std::size_t>::max()) ||
      byteOffset > static_cast<quint64>(
                      std::numeric_limits<std::size_t>::max()))
    {
      m_lastError = "The DICOM slice size overflows the supported address space.";
      return;
    }

  const void *buffer = dicomPixelBuffer(m_dimg.GetPointer(), m_voxelType);
  if (!buffer)
    {
      m_lastError = "The DICOM voxel buffer is unavailable.";
      return;
    }
  std::memcpy(slice,
              static_cast<const uchar*>(buffer)+
                static_cast<std::size_t>(byteOffset),
              static_cast<std::size_t>(sliceBytes));
}

//void
//DicomPlugin::getWidthSlice(int slc,
//			   uchar *slice)
//{
//  itk::Size<3> size;
//  size[0] = m_height; size[1] = 1; size[2] = m_depth;
//  itk::Index<3> index;
//  index[0] = 0; index[1] = slc;
//  index[2] = 0; 
//  typedef itk::ImageRegion<3> RegionType;
//  RegionType region(index,size);
//
//  typedef itk::ExtractImageFilter< ImageType, ImageType > FilterType;
//  FilterType::Pointer filter = FilterType::New();
//  filter->SetExtractionRegion(region);
//  filter->SetInput(m_dimg);
//  filter->SetDirectionCollapseToIdentity();
//  filter->Update(); 
//  ImageType *output = filter->GetOutput();
//  char *tmp = (char*)(output->GetBufferPointer());
//
//  memcpy(slice, tmp, m_depth*m_height*m_bytesPerVoxel);
//}
//
//void
//DicomPlugin::getHeightSlice(int slc,
//			     uchar *slice)
//{
//  itk::Size<3> size;
//  size[0] = 1; size[1] = m_width; size[2] = m_depth;
//  itk::Index<3> index;
//  index[0] = slc; index[1] = 0;
//  index[2] = 0; 
//  typedef itk::ImageRegion<3> RegionType;
//  RegionType region(index,size);
//
//  typedef itk::ExtractImageFilter< ImageType, ImageType > FilterType;
//  FilterType::Pointer filter = FilterType::New();
//  filter->SetExtractionRegion(region);
//  filter->SetInput(m_dimg);
//  filter->SetDirectionCollapseToIdentity();
//  filter->Update(); 
//  ImageType *output = filter->GetOutput();
//  char *tmp = (char*)(output->GetBufferPointer());
//
//  memcpy(slice, tmp, m_width*m_depth*m_bytesPerVoxel);
//}

QVariant
DicomPlugin::rawValue(int d, int w, int h)
{
  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height || !m_dimg)
    return QVariant("OutOfBounds");

  const quint64 index =
    (static_cast<quint64>(d)*static_cast<quint64>(m_width) +
     static_cast<quint64>(w))*static_cast<quint64>(m_height) +
    static_cast<quint64>(h);
  if (index > static_cast<quint64>(
                std::numeric_limits<std::size_t>::max()))
    return QVariant("OutOfBounds");

  const std::size_t offset = static_cast<std::size_t>(index);
  if (m_voxelType == _UChar)
    {
      const unsigned char *pixels =
        dicomPixels<unsigned char>(m_dimg.GetPointer());
      return pixels ? QVariant(static_cast<uint>(pixels[offset])) :
                      QVariant("Unavailable");
    }
  if (m_voxelType == _Char)
    {
      const signed char *pixels = dicomPixels<signed char>(m_dimg.GetPointer());
      return pixels ? QVariant(static_cast<int>(pixels[offset])) :
                      QVariant("Unavailable");
    }
  if (m_voxelType == _UShort)
    {
      const unsigned short *pixels =
        dicomPixels<unsigned short>(m_dimg.GetPointer());
      return pixels ? QVariant(static_cast<uint>(pixels[offset])) :
                      QVariant("Unavailable");
    }
  if (m_voxelType == _Short)
    {
      const short *pixels = dicomPixels<short>(m_dimg.GetPointer());
      return pixels ? QVariant(static_cast<int>(pixels[offset])) :
                      QVariant("Unavailable");
    }
  if (m_voxelType == _Int)
    {
      const int *pixels = dicomPixels<int>(m_dimg.GetPointer());
      return pixels ? QVariant(pixels[offset]) : QVariant("Unavailable");
    }
  if (m_voxelType == _Float)
    {
      const float *pixels = dicomPixels<float>(m_dimg.GetPointer());
      return pixels ? QVariant(static_cast<double>(pixels[offset])) :
                      QVariant("Unavailable");
    }
  return QVariant("Unavailable");
}

//void
//DicomPlugin::saveTrimmed(QString trimFile,
//			    int dmin, int dmax,
//			    int wmin, int wmax,
//			    int hmin, int hmax)
//{
//  QProgressDialog progress("Saving trimmed volume",
//			   0,
//			   0, 100,
//			   0);
//  progress.setMinimumDuration(0);
//
//  int nX, nY, nZ;
//  nX = m_depth;
//  nY = m_width;
//  nZ = m_height;
//
//  int mX, mY, mZ;
//  mX = dmax-dmin+1;
//  mY = wmax-wmin+1;
//  mZ = hmax-hmin+1;
//
//  int nbytes = nY*nZ*m_bytesPerVoxel;
//  uchar *tmp = new uchar[nbytes];
//
//  uchar vt;
//  if (m_voxelType == _UChar) vt = 0; // unsigned byte
//  if (m_voxelType == _Char) vt = 1; // signed byte
//  if (m_voxelType == _UShort) vt = 2; // unsigned short
//  if (m_voxelType == _Short) vt = 3; // signed short
//  if (m_voxelType == _Int) vt = 4; // int
//  if (m_voxelType == _Float) vt = 8; // float
//  
//  QFile fout(trimFile);
//  fout.open(QFile::WriteOnly);
//
//  fout.write((char*)&vt, 1);
//  fout.write((char*)&mX, 4);
//  fout.write((char*)&mY, 4);
//  fout.write((char*)&mZ, 4);
//
//  itk::Size<3> size;
//  size[0] = m_height; size[1] = m_width; size[2] = 1;
//  itk::Index<3> index;
//  index[0] = 0; index[1] = 0;
//
//  for(int i=dmax; i>=dmin; i--)
//    {
//      index[2] = i; 
//      typedef itk::ImageRegion<3> RegionType;
//      RegionType region(index,size);
//
//      typedef itk::ExtractImageFilter< ImageType, ImageType > FilterType;
//      FilterType::Pointer filter = FilterType::New();
//      filter->SetExtractionRegion(region);
//      filter->SetInput(m_dimg);
//      filter->SetDirectionCollapseToIdentity();
//      filter->Update(); 
//      ImageType *output = filter->GetOutput();
//      char *tmp1 = (char*)(output->GetBufferPointer());
//
//      memcpy(tmp, tmp1, nbytes);
//      
//      for(uint j=wmin; j<=wmax; j++)
//	{
//	  memcpy(tmp+(j-wmin)*mZ*m_bytesPerVoxel,
//		 tmp+(j*nZ + hmin)*m_bytesPerVoxel,
//		 mZ*m_bytesPerVoxel);
//	}
//	  
//      fout.write((char*)tmp, mY*mZ*m_bytesPerVoxel);
//
//      progress.setValue((int)(100*(float)(dmax-i)/(float)mX));
//      qApp->processEvents();
//    }
//
//  fout.close();
//
//  delete [] tmp;
//
//  progress.setValue(100);
//
//  m_headerBytes = 13; // to be used for applyMapping function
//}
