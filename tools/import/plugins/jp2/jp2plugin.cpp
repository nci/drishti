#include <QtGui>
#include "common.h"
#include "importmemoryadmission.h"
#include "jp2plugin.h"

#include "openjpeg.h"
#include <QCollator>
#include <QFile>

#include <algorithm>
#include <limits>
using namespace std;

namespace
{
const std::uint64_t kJp2CodecSafetyBytes = 64ULL*1024ULL*1024ULL;
const OPJ_SIZE_T kJp2StreamBufferBytes = 1024U*1024U;

OPJ_SIZE_T readJp2File(void *buffer, OPJ_SIZE_T byteCount, void *userData)
{
  QFile *file = static_cast<QFile*>(userData);
  if (!file || !buffer ||
      byteCount > static_cast<OPJ_SIZE_T>(
        std::numeric_limits<qint64>::max()))
    return static_cast<OPJ_SIZE_T>(-1);
  const qint64 bytesRead = file->read(static_cast<char*>(buffer),
                                      static_cast<qint64>(byteCount));
  return bytesRead > 0 ? static_cast<OPJ_SIZE_T>(bytesRead) :
                         static_cast<OPJ_SIZE_T>(-1);
}

OPJ_OFF_T skipJp2File(OPJ_OFF_T byteCount, void *userData)
{
  QFile *file = static_cast<QFile*>(userData);
  if (!file)
    return -1;
  const qint64 current = file->pos();
  const qint64 offset = static_cast<qint64>(byteCount);
  if (current < 0 ||
      (offset > 0 && current > std::numeric_limits<qint64>::max()-offset) ||
      (offset < 0 &&
       (offset == std::numeric_limits<qint64>::min() || current < -offset)))
    return -1;
  return file->seek(current+offset) ? byteCount : -1;
}

OPJ_BOOL seekJp2File(OPJ_OFF_T position, void *userData)
{
  QFile *file = static_cast<QFile*>(userData);
  if (!file || position < 0 ||
      static_cast<quint64>(position) >
        static_cast<quint64>(std::numeric_limits<qint64>::max()))
    return OPJ_FALSE;
  return file->seek(static_cast<qint64>(position)) ? OPJ_TRUE : OPJ_FALSE;
}

void closeJp2File(void *userData)
{
  delete static_cast<QFile*>(userData);
}

opj_stream_t *createJp2FileStream(const QString &fileName)
{
  QFile *file = new QFile(fileName);
  if (!file->open(QIODevice::ReadOnly) || file->size() < 0)
    {
      delete file;
      return 0;
    }
  opj_stream_t *stream = opj_stream_create(kJp2StreamBufferBytes, OPJ_TRUE);
  if (!stream)
    {
      delete file;
      return 0;
    }
  opj_stream_set_user_data(stream, file, closeJp2File);
  opj_stream_set_user_data_length(stream,
                                  static_cast<OPJ_UINT64>(file->size()));
  opj_stream_set_read_function(stream, readJp2File);
  opj_stream_set_skip_function(stream, skipJp2File);
  opj_stream_set_seek_function(stream, seekJp2File);
  return stream;
}

bool admitJp2Decode(std::uint64_t pixelCount, QString *error)
{
  std::uint64_t requiredBytes = 0;
  if (!checkedImportImageDecodeWorkingSet(pixelCount,
                                          kJp2CodecSafetyBytes,
                                          requiredBytes))
    {
      *error = "JPEG2000 decode working-set calculation overflowed.";
      return false;
    }
  const ImportMemoryAdmission admission =
    evaluateImportMemoryAdmission(requiredBytes);
  if (admission.approved)
    return true;
  *error = QString("JPEG2000 decoding was stopped before pixel allocation. "
                   "Required peak increment: %1 MiB; usable physical "
                   "budget: %2 MiB.")
             .arg(requiredBytes/(1024.0*1024.0), 0, 'f', 1)
             .arg(admission.availablePhysicalBudgetBytes/(1024.0*1024.0),
                  0, 'f', 1);
  return false;
}
}

QStringList
Jp2Plugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "Grayscale JPEG2000 Image Directory";
  regString << "files";
  regString << "Grayscale JPEG2000 Image Files";
  
  return regString;
}

void
Jp2Plugin::init()
{  
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_headerBytes = 0;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_lastError.clear();
  m_lastOperationCanceled = false;
  m_4dvol = false;
}

void
Jp2Plugin::clear()
{
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_headerBytes = 0;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_lastError.clear();
  m_lastOperationCanceled = false;
  m_4dvol = false;
}

void
Jp2Plugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
Jp2Plugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString Jp2Plugin::description() { return m_description; }
int Jp2Plugin::voxelType() { return m_voxelType; }
int Jp2Plugin::voxelUnit() { return m_voxelUnit; }
int Jp2Plugin::headerBytes() { return m_headerBytes; }
QString Jp2Plugin::lastError() const { return m_lastError; }
bool Jp2Plugin::wasCanceled() const { return m_lastOperationCanceled; }

void
Jp2Plugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
}
float Jp2Plugin::rawMin() { return m_rawMin; }
float Jp2Plugin::rawMax() { return m_rawMax; }
QList<uint> Jp2Plugin::histogram() { return m_histogram; }

void
Jp2Plugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
Jp2Plugin::replaceFile(QString flnm)
{
  const QStringList previousFileName = m_fileName;
  const QList<QString> previousImageList = m_imageList;
  const int previousDepth = m_depth;
  const int previousWidth = m_width;
  const int previousHeight = m_height;
  const int previousVoxelType = m_voxelType;
  const int previousHeaderBytes = m_headerBytes;
  const int previousBytesPerVoxel = m_bytesPerVoxel;
  const float previousRawMin = m_rawMin;
  const float previousRawMax = m_rawMax;
  const QList<uint> previousHistogram = m_histogram;

  const bool previous4dVolume = m_4dvol;
  m_4dvol = true;
  const bool loaded = setFile(QStringList() << flnm);
  m_4dvol = previous4dVolume;
  if (!loaded)
    return;

  const bool compatible = previousImageList.isEmpty() ||
    (m_depth == previousDepth && m_width == previousWidth &&
     m_height == previousHeight && m_voxelType == previousVoxelType &&
     m_bytesPerVoxel == previousBytesPerVoxel);
  if (!compatible)
    {
      m_fileName = previousFileName;
      m_imageList = previousImageList;
      m_depth = previousDepth;
      m_width = previousWidth;
      m_height = previousHeight;
      m_voxelType = previousVoxelType;
      m_headerBytes = previousHeaderBytes;
      m_bytesPerVoxel = previousBytesPerVoxel;
      m_rawMin = previousRawMin;
      m_rawMax = previousRawMax;
      m_histogram = previousHistogram;
      m_lastError =
        "Cannot replace JPEG2000 input: stack layout differs from the original.";
      return;
    }

  m_rawMin = previousRawMin;
  m_rawMax = previousRawMax;
  m_histogram = previousHistogram;
}

bool
Jp2Plugin::setImageFiles(QStringList files)
{
  if (files.isEmpty())
    {
      m_lastError = "No JPEG2000 images were found.";
      return false;
    }

  QProgressDialog progress("Validating JPEG2000 images",
			   "Cancel",
			   0, files.size(),
			   0);
  progress.setMinimumDuration(0);

  QStringList imageList;
  const QFileInfo selection(m_fileName.value(0));
  const QDir baseDirectory = selection.isDir() ?
    QDir(selection.absoluteFilePath()) : selection.absoluteDir();
  int expectedWidth = 0;
  int expectedHeight = 0;
  int expectedVoxelType = 0;
  int expectedBytesPerVoxel = 0;
  for (int i=0; i<files.size(); ++i)
    {
      progress.setValue(i);
      qApp->processEvents();
      if (progress.wasCanceled())
        {
          m_lastOperationCanceled = true;
          m_lastError = "JPEG2000 import canceled.";
          return false;
        }

      QFileInfo fileInfo(files[i]);
      if (fileInfo.isRelative())
        fileInfo.setFile(baseDirectory.filePath(files[i]));
      if (!fileInfo.exists() || !fileInfo.isFile() || !fileInfo.isReadable())
        {
          m_lastError = QString("JPEG2000 image is missing or unreadable: %1")
                          .arg(fileInfo.absoluteFilePath());
          return false;
        }
      imageList.append(fileInfo.absoluteFilePath());
      if (!loadJp2ImageProperties(imageList.constLast()))
        return false;
      if (i == 0)
        {
          expectedWidth = m_width;
          expectedHeight = m_height;
          expectedVoxelType = m_voxelType;
          expectedBytesPerVoxel = m_bytesPerVoxel;
          const std::uint64_t pixelCount =
            static_cast<std::uint64_t>(m_width)*m_height;
          if (!admitJp2Decode(pixelCount, &m_lastError))
            return false;
        }
      else if (m_width != expectedWidth || m_height != expectedHeight ||
               m_voxelType != expectedVoxelType ||
               m_bytesPerVoxel != expectedBytesPerVoxel)
        {
          m_lastError = QString("JPEG2000 stack layout differs at %1")
                          .arg(imageList.constLast());
          return false;
        }
    }

  m_width = expectedWidth;
  m_height = expectedHeight;
  m_voxelType = expectedVoxelType;
  m_bytesPerVoxel = expectedBytesPerVoxel;
  m_imageList = imageList;
  m_depth = m_imageList.size();
  m_headerBytes = 0;

  progress.setValue(files.size());
  qApp->processEvents();
  if (m_4dvol)
    {
      QByteArray pixels(m_width*m_height*m_bytesPerVoxel, 0);
      QProgressDialog decodeProgress("Validating JPEG2000 pixels", "Cancel",
                                     0, m_depth, 0);
      decodeProgress.setMinimumDuration(0);
      for (int i=0; i<m_depth; ++i)
        {
          decodeProgress.setValue(i);
          qApp->processEvents();
          if (decodeProgress.wasCanceled())
            {
              m_lastOperationCanceled = true;
              m_lastError = "JPEG2000 import canceled.";
              return false;
            }
          if (!loadJp2Image(i, reinterpret_cast<uchar*>(pixels.data())))
            return false;
        }
      decodeProgress.setValue(m_depth);
      qApp->processEvents();
    }
  else
    {
      findMinMaxandGenerateHistogram();
      if (!m_lastError.isEmpty())
        return false;
    }

  return true;
}

bool
Jp2Plugin::setFile(QStringList files)
{
  m_lastError.clear();
  m_lastOperationCanceled = false;
  if (files.isEmpty())
    {
      m_lastError = "No JPEG2000 input was selected.";
      return false;
    }

  const QStringList previousFileName = m_fileName;
  const QList<QString> previousImageList = m_imageList;
  const int previousDepth = m_depth;
  const int previousWidth = m_width;
  const int previousHeight = m_height;
  const int previousVoxelType = m_voxelType;
  const int previousBytesPerVoxel = m_bytesPerVoxel;
  const int previousHeaderBytes = m_headerBytes;
  const float previousRawMin = m_rawMin;
  const float previousRawMax = m_rawMax;
  const QList<uint> previousHistogram = m_histogram;

  m_fileName = files;
  m_imageList.clear();

  QFileInfo f(m_fileName[0]);
  if (f.isDir())
    {
      const QStringList candidates = QDir(m_fileName[0]).entryList(
        QDir::NoSymLinks|QDir::NoDotAndDotDot|QDir::Readable|QDir::Files);
      QStringList imgfiles;
      for (const QString &candidate : candidates)
        {
          const QString suffix = QFileInfo(candidate).suffix().toLower();
          if (suffix == "jp2" || suffix == "j2k" || suffix == "jpt")
            imgfiles.append(candidate);
        }
      QCollator collator;
      collator.setCaseSensitivity(Qt::CaseInsensitive);
      collator.setNumericMode(true);
      std::sort(imgfiles.begin(), imgfiles.end(),
                [&collator](const QString &left, const QString &right)
                { return collator.compare(left, right) < 0; });
      
      if (setImageFiles(imgfiles))
        return true;
    }
  else
    {
      if (setImageFiles(files))
        return true;
    }

  m_fileName = previousFileName;
  m_imageList = previousImageList;
  m_depth = previousDepth;
  m_width = previousWidth;
  m_height = previousHeight;
  m_voxelType = previousVoxelType;
  m_bytesPerVoxel = previousBytesPerVoxel;
  m_headerBytes = previousHeaderBytes;
  m_rawMin = previousRawMin;
  m_rawMax = previousRawMax;
  m_histogram = previousHistogram;
  return false;
}

#define MINMAXANDHISTOGRAM()				\
  {							\
    for(int j=0; j<m_width*m_height; j++)		\
      {							\
	int val = ptr[j];				\
	m_rawMin = qMin(m_rawMin, (float)val);		\
	m_rawMax = qMax(m_rawMax, (float)val);		\
							\
	int idx = val-rMin;				\
	m_histogram[idx]++;				\
      }							\
  }


bool
Jp2Plugin::loadJp2ImageProperties(QString filename)
{
  opj_stream_t *stream = createJp2FileStream(filename);
  if (!stream)
    {
      m_lastError = QString("Cannot open JPEG2000 image %1").arg(filename);
      return false;
    }

  opj_codec_t *codec = 0;
  const QString suffix = QFileInfo(filename).suffix().toLower();
  if (suffix == "jp2")
    codec = opj_create_decompress(OPJ_CODEC_JP2);
  else if (suffix == "jpt")
    codec = opj_create_decompress(OPJ_CODEC_JPT);
  else if (suffix == "j2k")
    codec = opj_create_decompress(OPJ_CODEC_J2K);

  if (!codec)
    {
      m_lastError = QString("Unsupported JPEG2000 extension in %1")
                      .arg(filename);
      opj_stream_destroy(stream);
      return false;
    }

  opj_dparameters_t parameters;
  opj_set_default_decoder_parameters(&parameters);
  if (!opj_setup_decoder(codec, &parameters))
    {
      m_lastError = QString("Cannot initialize the JPEG2000 decoder for %1")
                      .arg(filename);
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      return false;
    }

  opj_image_t *image = 0;
  if (!opj_read_header(stream, codec, &image))
    {
      m_lastError = QString("Cannot read the JPEG2000 header in %1")
                      .arg(filename);
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      if (image) opj_image_destroy(image);
      return false;
    }

  bool valid = image && image->numcomps == 1 && image->comps &&
               image->comps[0].w > 0 && image->comps[0].h > 0 &&
               image->comps[0].sgnd == 0 && image->comps[0].prec > 0 &&
               image->comps[0].prec <= 16;
  const quint64 width = valid ? image->comps[0].w : 0;
  const quint64 height = valid ? image->comps[0].h : 0;
  const int bytesPerVoxel = valid && image->comps[0].prec > 8 ? 2 : 1;
  if (!valid || width > static_cast<quint64>(std::numeric_limits<int>::max()) ||
      height > static_cast<quint64>(std::numeric_limits<int>::max()) ||
      width > std::numeric_limits<quint64>::max()/height ||
      width*height > std::numeric_limits<quint64>::max()/bytesPerVoxel ||
      width*height*bytesPerVoxel >
        static_cast<quint64>(std::numeric_limits<int>::max()))
    {
      m_lastError = QString("Unsupported or invalid grayscale JPEG2000 layout in %1")
                      .arg(filename);
      if (image) opj_image_destroy(image);
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      return false;
    }

  m_width = static_cast<int>(width);
  m_height = static_cast<int>(height);
  m_bytesPerVoxel = bytesPerVoxel;
  m_voxelType = bytesPerVoxel == 1 ? _UChar : _UShort;

  opj_image_destroy(image);
  opj_stream_destroy(stream);
  opj_destroy_codec(codec);
  return true;
}

bool
Jp2Plugin::loadJp2Image(int i, uchar* tmp)
{
  if (!tmp || i < 0 || i >= m_imageList.size())
    {
      m_lastError = QString("JPEG2000 slice %1 is invalid.").arg(i);
      return false;
    }
  QString filename = m_imageList[i];

  opj_stream_t *stream = createJp2FileStream(filename);
  if (!stream)
    {
      m_lastError = QString("Cannot open JPEG2000 image %1").arg(filename);
      return false;
    }

  opj_codec_t *codec = 0;
  const QString suffix = QFileInfo(filename).suffix().toLower();
  if (suffix == "jp2")
    codec = opj_create_decompress(OPJ_CODEC_JP2);
  else if (suffix == "jpt")
    codec = opj_create_decompress(OPJ_CODEC_JPT);
  else if (suffix == "j2k")
    codec = opj_create_decompress(OPJ_CODEC_J2K);
  if (!codec)
    {
      m_lastError = QString("Unsupported JPEG2000 extension in %1")
                      .arg(filename);
      opj_stream_destroy(stream);
      return false;
    }

  opj_dparameters_t parameters;
  opj_set_default_decoder_parameters(&parameters);
  if (!opj_setup_decoder(codec, &parameters))
    {
      m_lastError = QString("Cannot initialize the JPEG2000 decoder for %1")
                      .arg(filename);
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      return false;
    }

  opj_image_t *image = 0;
  if (!opj_read_header(stream, codec, &image))
    {
      m_lastError = QString("Cannot read the JPEG2000 header in %1")
                      .arg(filename);
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      if (image) opj_image_destroy(image);
      return false;
    }

  const bool layoutMatches = image && image->numcomps == 1 && image->comps &&
    image->comps[0].w == static_cast<OPJ_UINT32>(m_width) &&
    image->comps[0].h == static_cast<OPJ_UINT32>(m_height) &&
    image->comps[0].sgnd == 0 && image->comps[0].prec > 0 &&
    image->comps[0].prec <= static_cast<OPJ_UINT32>(m_bytesPerVoxel*8);
  if (!layoutMatches || !opj_decode(codec, stream, image) ||
      !image->comps[0].data)
    {
      m_lastError = QString("Cannot decode JPEG2000 pixels from %1")
                      .arg(filename);
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      if (image) opj_image_destroy(image);
      return false;
    }

  if (m_bytesPerVoxel == 1)
    {
      int idx = 0;
      for (int i=0; i<m_height; i++)
	for (int j=0; j<m_width; j++)
	  {
	    tmp[idx] = image->comps[0].data[i*m_width + j];
	    idx++;
	  }
    }
  else
    {
      int idx = 0;
      ushort *tmpUS = (ushort*)tmp;
      for (int i=0; i<m_height; i++)
	for (int j=0; j<m_width; j++)
	  {
	    tmpUS[idx] = image->comps[0].data[i*m_width + j];
	    idx++;
	  }
    }
  opj_image_destroy(image);
  opj_stream_destroy(stream);
  opj_destroy_codec(codec);
  return true;
}

void
Jp2Plugin::findMinMaxandGenerateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  float rSize;
  float rMin;
  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char)
    {
      if (m_voxelType == _UChar) rMin = 0;
      if (m_voxelType == _Char) rMin = -127;
      rSize = 255;
      for(uint i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      if (m_voxelType == _UShort) rMin = 0;
      if (m_voxelType == _Short) rMin = -32767;
      rSize = 65535;
      for(uint i=0; i<65536; i++)
	m_histogram.append(0);
    }
  else
    {
      m_lastError = "Cannot generate a JPEG2000 histogram for this voxel type.";
      return;
    }

  int nbytes = m_width*m_height*m_bytesPerVoxel;
  QByteArray tmp(nbytes, 0);

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();

  for(int i=0; i<m_depth; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)m_depth));
      progress.setLabelText(QString("%1 of %2").arg(i).arg(m_depth-1));
      qApp->processEvents();
      if (progress.wasCanceled())
        {
          m_lastOperationCanceled = true;
          m_lastError = "JPEG2000 import canceled.";
          return;
        }

      if (!loadJp2Image(i, reinterpret_cast<uchar*>(tmp.data())))
        return;

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = reinterpret_cast<uchar*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Char)
	{
	  char *ptr = reinterpret_cast<char*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
      if (m_voxelType == _UShort)
	{
	  ushort *ptr = reinterpret_cast<ushort*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Short)
	{
	  short *ptr = reinterpret_cast<short*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Int)
	{
	  int *ptr = reinterpret_cast<int*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = reinterpret_cast<float*>(tmp.data());
	  MINMAXANDHISTOGRAM();
	}
    }

//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

  progress.setValue(100);
  qApp->processEvents();
}

#define FINDMINMAX()					\
  {							\
    for(int j=0; j<nY*nZ; j++)				\
      {							\
	float val = ptr[j];				\
	m_rawMin = qMin(m_rawMin, val);			\
	m_rawMax = qMax(m_rawMax, val);			\
      }							\
  }

void
Jp2Plugin::findMinMax()
{
  QProgressDialog progress("Finding Min and Max",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int nbytes = nY*nZ*m_bytesPerVoxel;
  QByteArray tmp(nbytes, 0);

  m_rawMin = std::numeric_limits<float>::max();
  m_rawMax = std::numeric_limits<float>::lowest();
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();
      if (progress.wasCanceled())
        {
          m_lastOperationCanceled = true;
          m_lastError = "JPEG2000 import canceled.";
          return;
        }

      if (!loadJp2Image(i, reinterpret_cast<uchar*>(tmp.data())))
        return;

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = reinterpret_cast<uchar*>(tmp.data());
	  FINDMINMAX();
	}
      else if (m_voxelType == _Char)
	{
	  char *ptr = reinterpret_cast<char*>(tmp.data());
	  FINDMINMAX();
	}
      if (m_voxelType == _UShort)
	{
	  ushort *ptr = reinterpret_cast<ushort*>(tmp.data());
	  FINDMINMAX();
	}
      else if (m_voxelType == _Short)
	{
	  short *ptr = reinterpret_cast<short*>(tmp.data());
	  FINDMINMAX();
	}
      else if (m_voxelType == _Int)
	{
	  int *ptr = reinterpret_cast<int*>(tmp.data());
	  FINDMINMAX();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = reinterpret_cast<float*>(tmp.data());
	  FINDMINMAX();
	}
    }

  progress.setValue(100);
  qApp->processEvents();
}

#define GENHISTOGRAM()					\
  {							\
    for(int j=0; j<nY*nZ; j++)				\
      {							\
	float fidx = rSize > 0 ? (ptr[j]-m_rawMin)/rSize : 0; \
	fidx = qBound(0.0f, fidx, 1.0f);		\
	int idx = fidx*histogramSize;			\
	m_histogram[idx]+=1;				\
      }							\
  }

void
Jp2Plugin::generateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  float rSize = m_rawMax-m_rawMin;
  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int nbytes = nY*nZ*m_bytesPerVoxel;
  QByteArray tmp(nbytes, 0);

  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char ||
      m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      for(int i=0; i<rSize+1; i++)
	m_histogram.append(0);
    }
  else
    {      
      for(int i=0; i<65536; i++)
	m_histogram.append(0);
    }

  int histogramSize = m_histogram.size()-1;
  for(int i=0; i<nX; i++)
    {
      progress.setValue((int)(100.0*(float)i/(float)nX));
      qApp->processEvents();
      if (progress.wasCanceled())
        {
          m_lastOperationCanceled = true;
          m_lastError = "JPEG2000 import canceled.";
          return;
        }

      if (!loadJp2Image(i, reinterpret_cast<uchar*>(tmp.data())))
        return;

      if (m_voxelType == _UChar)
	{
	  uchar *ptr = reinterpret_cast<uchar*>(tmp.data());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Char)
	{
	  char *ptr = reinterpret_cast<char*>(tmp.data());
	  GENHISTOGRAM();
	}
      if (m_voxelType == _UShort)
	{
	  ushort *ptr = reinterpret_cast<ushort*>(tmp.data());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Short)
	{
	  short *ptr = reinterpret_cast<short*>(tmp.data());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Int)
	{
	  int *ptr = reinterpret_cast<int*>(tmp.data());
	  GENHISTOGRAM();
	}
      else if (m_voxelType == _Float)
	{
	  float *ptr = reinterpret_cast<float*>(tmp.data());
	  GENHISTOGRAM();
	}
    }

//  while(m_histogram.last() == 0)
//    m_histogram.removeLast();
//  while(m_histogram.first() == 0)
//    m_histogram.removeFirst();

//  QMessageBox::information(0, "",  QString("%1 %2 : %3").\
//			   arg(m_rawMin).arg(m_rawMax).arg(rSize));

  progress.setValue(100);
  qApp->processEvents();
}

void
Jp2Plugin::getDepthSlice(int slc,
			  uchar *slice)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;
  m_lastError.clear();
  m_lastOperationCanceled = false;

  if (!slice || slc < 0 || slc >= m_depth)
    {
      if (slice && nbytes > 0)
	memset(slice, 0, nbytes);
      m_lastError = QString("JPEG2000 slice %1 is invalid.").arg(slc);
      return;
    }

  QByteArray tmp1(nbytes, 0);

  if (!loadJp2Image(slc, reinterpret_cast<uchar*>(tmp1.data())))
    {
      memset(slice, 0, nbytes);
      return;
    }

  if (m_voxelType == _UChar)
    {
      for(int j=0; j<m_width; j++)
	for(int k=0; k<m_height; k++)
	  slice[j*m_height+k] =
	    reinterpret_cast<const uchar*>(tmp1.constData())[k*m_width+j];
    }
  else if (m_voxelType == _UShort)
    {
      ushort *p0 = (ushort*)slice;
      const ushort *p1 = reinterpret_cast<const ushort*>(tmp1.constData());
      for(int j=0; j<m_width; j++)
	for(int k=0; k<m_height; k++)
	  p0[j*m_height+k] = p1[k*m_width+j];
    }
  else if (m_voxelType == _Float)
    {
      float *p0 = (float*)slice;
      const float *p1 = reinterpret_cast<const float*>(tmp1.constData());
      for(int j=0; j<m_width; j++)
	for(int k=0; k<m_height; k++)
	  p0[j*m_height+k] = p1[k*m_width+j];
    }
}

//void
//Jp2Plugin::getWidthSlice(int slc,
//			  uchar *slice)
//{
//  QProgressDialog progress("Extracting Slice",
//			   0,
//			   0, 100,
//			   0);
//  progress.setMinimumDuration(0);
//  uchar *imgSlice = new uchar[m_width*m_height*m_bytesPerVoxel];
//  for(uint i=0; i<m_depth; i++)
//    {
//      progress.setValue((int)(100.0*(float)i/(float)m_depth));
//      qApp->processEvents();
//
//      loadJp2Image(i, imgSlice);
//
//      if (m_voxelType == _UChar)
//	{
//	  for(uint j=0; j<m_height; j++)
//	    slice[i*m_height+j] = imgSlice[j*m_width + slc];
//	}
//      else if (m_voxelType == _UShort)
//	{
//	  ushort *p0 = (ushort*)slice;
//	  ushort *p1 = (ushort*)imgSlice;
//	  for(uint j=0; j<m_height; j++)
//	    p0[i*m_height+j] = p1[j*m_width + slc];
//	}
//      else if (m_voxelType == _Float)
//	{
//	  float *p0 = (float*)slice;
//	  float *p1 = (float*)imgSlice;
//	  for(uint j=0; j<m_height; j++)
//	    p0[i*m_height+j] = p1[j*m_width + slc];
//	}
//    }
//  delete [] imgSlice;
//  progress.setValue(100);
//  qApp->processEvents();
//}
//
//void
//Jp2Plugin::getHeightSlice(int slc,
//			   uchar *slice)
//{
//  uchar *imgSlice = new uchar[m_width*m_height*m_bytesPerVoxel];
//  QProgressDialog progress("Extracting Slice",
//			   0,
//			   0, 100,
//			   0);
//  progress.setMinimumDuration(0);
//  for(uint i=0; i<m_depth; i++)
//    {
//      progress.setValue((int)(100.0*(float)i/(float)m_depth));
//      qApp->processEvents();
//
//      loadJp2Image(i, imgSlice);
//
//      if (m_voxelType == _UChar)
//	{
//	  for(uint j=0; j<m_width; j++)
//	    slice[i*m_width+j] = imgSlice[slc*m_width + j];
//	}
//      else if (m_voxelType == _UShort)
//	{
//	  ushort *p0 = (ushort*)slice;
//	  ushort *p1 = (ushort*)imgSlice;
//	  for(uint j=0; j<m_width; j++)
//	    p0[i*m_width+j] = p1[slc*m_width + j];
//	}
//      else if (m_voxelType == _Float)
//	{
//	  float *p0 = (float*)slice;
//	  float *p1 = (float*)imgSlice;
//	  for(uint j=0; j<m_width; j++)
//	    p0[i*m_width+j] = p1[slc*m_width + j];
//	}
//    }
//  delete [] imgSlice;
//  progress.setValue(100);
//  qApp->processEvents();
//}

QVariant
Jp2Plugin::rawValue(int d, int w, int h)
{
  QVariant v;
  m_lastError.clear();
  m_lastOperationCanceled = false;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }

  QByteArray imgSlice(m_width*m_height*m_bytesPerVoxel, 0);

  if (!loadJp2Image(d, reinterpret_cast<uchar*>(imgSlice.data())))
    return QVariant("ReadError");

  if (m_voxelType == _UChar)
    {
      uchar a = reinterpret_cast<const uchar*>(imgSlice.constData())[h*m_width+w];
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _UShort)
    {
      ushort a = reinterpret_cast<const ushort*>(imgSlice.constData())[h*m_width+w];
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Float)
    {
      float a = reinterpret_cast<const float*>(imgSlice.constData())[h*m_width+w];
      v = QVariant((double)a);
    }

  return v;
}

//void
//Jp2Plugin::saveTrimmed(QString trimFile,
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
//  uchar *tmp1 = new uchar[nbytes];
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
//  //for(uint i=dmin; i<=dmax; i++)
//  for(int i=dmax; i>=dmin; i--)
//    {
//      loadJp2Image(i, tmp1);
//
//      if (m_voxelType == _UChar)
//	{
//	  for(uint j=0; j<m_width; j++)
//	    for(uint k=0; k<m_height; k++)
//	      tmp[j*m_height+k] = tmp1[k*m_width+j];
//	}
//      else if (m_voxelType == _UShort)
//	{
//	  ushort *p0 = (ushort*)tmp;
//	  ushort *p1 = (ushort*)tmp1;
//	  for(uint j=0; j<m_width; j++)
//	    for(uint k=0; k<m_height; k++)
//	      p0[j*m_height+k] = p1[k*m_width+j];
//	}
//      else if (m_voxelType == _Float)
//	{
//	  float *p0 = (float*)tmp;
//	  float *p1 = (float*)tmp1;
//	  for(uint j=0; j<m_width; j++)
//	    for(uint k=0; k<m_height; k++)
//	      p0[j*m_height+k] = p1[k*m_width+j];
//	}
//      
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
//  delete [] tmp1;
//
//  progress.setValue(100);
//
//  m_headerBytes = 13; // to be used for applyMapping function
//}
