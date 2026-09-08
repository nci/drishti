#include <QtGui>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include "common.h"
#include "tiffplugin.h"

//#include "libtiff/tiffio.h"
#include <tiffio.h>
using namespace std;

QStringList
TiffPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "Grayscale TIFF Image Directory";
  regString << "files";
  regString << "Grayscale TIFF Image Files";
  
  return regString;
}

void
TiffPlugin::init()
{
  TIFFSetWarningHandler(NULL);
  
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
}

void
TiffPlugin::clear()
{
  m_fileName.clear();
  m_imageList.clear();

  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
}

void
TiffPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
TiffPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString TiffPlugin::description() { return m_description; }
int TiffPlugin::voxelType() { return m_voxelType; }
int TiffPlugin::voxelUnit() { return m_voxelUnit; }
int TiffPlugin::headerBytes() { return m_headerBytes; }

void
TiffPlugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
}
float TiffPlugin::rawMin() { return m_rawMin; }
float TiffPlugin::rawMax() { return m_rawMax; }
QList<uint> TiffPlugin::histogram() { return m_histogram; }

void
TiffPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
TiffPlugin::replaceFile(QString flnm)
{
  m_fileName.clear();
  m_fileName << flnm;

  m_imageList = m_fileName;
}

void
TiffPlugin::setImageFiles(QStringList files)
{
  QProgressDialog progress("Enumerating files - may take some time...",
			   0,
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  QStringList flist;
  flist.clear();
  
  for(uint i=0; i<files.size(); i++)
    {
      progress.setValue(100*(float)i/(float)files.size());
      qApp->processEvents();
      
      QFileInfo fileInfo(m_fileName[0], files[i]);
      QString imgfl = fileInfo.absoluteFilePath();

      m_imageList.append(imgfl);
    }
  
  progress.setValue(100);
  qApp->processEvents();

  m_depth = m_imageList.size();

  TIFF *image;
  image = TIFFOpen((char*)m_imageList[0].toUtf8().data(), "r");
  
  // -- get number of images(directories) within the file
  m_dirCount = 0;
  if (image)
    {
      do {
	m_dirCount ++;
      } while (TIFFReadDirectory(image));
      if (m_dirCount > 1)
	QMessageBox::information(0, "3D Tiff",
				 QString("Number of images : %1").arg(m_dirCount));
      if (m_dirCount > 1)
	m_depth = m_dirCount;
    }

  TIFFGetField(image, TIFFTAG_IMAGEWIDTH, &m_width);
  TIFFGetField(image, TIFFTAG_IMAGELENGTH, &m_height);

  uint16 bitPerSample;
  TIFFGetField(image, TIFFTAG_BITSPERSAMPLE, &bitPerSample);

  TIFFClose(image);

  if (bitPerSample == 1)
    {
      QMessageBox::critical(0, "Image Format Error", "Cannot handle this format : bits per voxel = 1");
      m_imageList.clear();
      return;
    }

  if (bitPerSample == 8)
    {
      m_voxelType = _UChar;

      QStringList vtypes;
      vtypes << "UChar";
      vtypes << "Char";
      QString option = QInputDialog::getItem(0,
					     "Select Voxel Type",
					     "Voxel Type (found 8 bits per voxel)",
					     vtypes,
					     0,
					     false);
      int idx = vtypes.indexOf(option);
      if (idx == 1)
	m_voxelType = _Char;
    }
  else if (bitPerSample == 16)
    {
      m_voxelType = _UShort;

      QStringList vtypes;
      vtypes << "UShort";
      vtypes << "Short";
      QString option = QInputDialog::getItem(0,
					     "Select Voxel Type",
					     "Voxel Type (found 16 bits per voxel)",
					     vtypes,
					     0,
					     false);
      int idx = vtypes.indexOf(option);
      if (idx == 1)
	m_voxelType = _Short;
    }
  else if (bitPerSample == 32)
    {
      m_voxelType = _Float;

      QStringList vtypes;
      vtypes << "Float";
      vtypes << "Int";
      QString option = QInputDialog::getItem(0,
					     "Select Voxel Type",
					     "Voxel Type (found 32 bits per voxel)",
					     vtypes,
					     0,
					     false);
      int idx = vtypes.indexOf(option);
      if (idx == 1)
	m_voxelType = _Int;
    }

  m_headerBytes = 0;

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;

  if (m_voxelType == _UChar ||
      m_voxelType == _Char ||
      m_voxelType == _UShort ||
      m_voxelType == _Short)
    findMinMaxandGenerateHistogram();
  else
    {
      //QMessageBox::information(0, "Error",
      //		       "Currently accepting only 1- and 2-byte images");
      findMinMax();
      generateHistogram();
    }
}

bool
TiffPlugin::setFile(QStringList files)
{
  if (files.size() == 0)
    return false;

  m_fileName = files;

  m_imageList.clear();

  QFileInfo f(m_fileName[0]);
  if (f.isDir())
    {
      // list all image files in the directory
      QStringList imageNameFilter;
      imageNameFilter << "*.tif";
      imageNameFilter << "*.tiff";
      QStringList imgfiles= QDir(m_fileName[0]).entryList(imageNameFilter,
							  QDir::NoSymLinks|
							  QDir::NoDotAndDotDot|
							  QDir::Readable|
							  QDir::Files);
      
      setImageFiles(imgfiles);
    }
  else
    setImageFiles(files);

  return true;
}

void
TiffPlugin::loadTiffImage(int i, uchar* tmp)
{
  TIFF *image;
  if (m_dirCount == 1)
    image = TIFFOpen((char*)m_imageList[i].toUtf8().data(), "r");
  else
    {
      image = TIFFOpen((char*)m_imageList[0].toUtf8().data(), "r");
      TIFFSetDirectory(image, i);
    }

  uint16 photo, bps, spp, fillorder;
  uint32 width;
  tsize_t stripSize;
  unsigned long imageOffset, result;
  int stripMax, stripCount;
  char *buffer, tempbyte;
  unsigned long bufferSize, count;


  // Read in the possibly multiple strips
  stripSize = TIFFStripSize (image);
  stripMax = TIFFNumberOfStrips (image);
  imageOffset = 0;
  
  bufferSize = TIFFNumberOfStrips (image) * stripSize;
  
  for (stripCount = 0; stripCount < stripMax; stripCount++)
    {
      if((result = TIFFReadEncodedStrip (image, stripCount,
					 tmp + imageOffset,
					 stripSize)) == -1)
	{
	  QMessageBox::critical(0, "TIFF Read Error",
				QString("Read error on input strip number %1").\
				arg(stripCount));
	  return;
	}

      imageOffset += result;
    }

  TIFFClose(image);

//  // Deal with fillorder
//  TIFFGetField(image, TIFFTAG_FILLORDER, &fillorder);
//  
//  if(fillorder != FILLORDER_MSB2LSB)
//    {
//      // We need to swap bits -- ABCDEFGH becomes HGFEDCBA
//      for(count = 0; count < bufferSize; count++)
//	{
//	  tempbyte = 0;
//	  if(tmp[count] & 128) tempbyte += 1;
//	  if(tmp[count] & 64) tempbyte += 2;
//	  if(tmp[count] & 32) tempbyte += 4;
//	  if(tmp[count] & 16) tempbyte += 8;
//	  if(tmp[count] & 8) tempbyte += 16;
//	  if(tmp[count] & 4) tempbyte += 32;
//	  if(tmp[count] & 2) tempbyte += 64;
//	  if(tmp[count] & 1) tempbyte += 128;
//	  tmp[count] = tempbyte;
//	}
//    }
  
}

void
TiffPlugin::findMinMaxandGenerateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   0,
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  float rMin;
  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char)
    {
      if (m_voxelType == _UChar) rMin = 0;
      if (m_voxelType == _Char) rMin = -127;
      for(uint i=0; i<256; i++)
	m_histogram.append(0);
    }
  else if (m_voxelType == _UShort ||
      m_voxelType == _Short)
    {
      if (m_voxelType == _UShort) rMin = 0;
      if (m_voxelType == _Short) rMin = -32767;
      for(uint i=0; i<65536; i++)
	m_histogram.append(0);
    }
  else
    {
      QMessageBox::information(0, "Error", "Why am i here ???");
      return;
    }

  const int nX = m_depth;
  const int nY = m_width;
  const int nZ = m_height;
  const int histBins = m_histogram.size();
  const int rMinBin = (int)rMin;
  const int nbytes = nY*nZ*m_bytesPerVoxel;

  m_rawMin = 10000000;
  m_rawMax = -10000000;

  const unsigned hw = std::thread::hardware_concurrency();
  unsigned numThreads = hw ? hw : 1u;
  if (numThreads > (unsigned)nX) numThreads = (unsigned)nX;
  if (numThreads < 1u) numThreads = 1u;

  std::vector<std::vector<qint64> > local((size_t)numThreads,
                                          std::vector<qint64>((size_t)histBins, 0));
  std::vector<float> localMin((size_t)numThreads, 10000000.0f);
  std::vector<float> localMax((size_t)numThreads, -10000000.0f);

  auto countPlane = [&](std::vector<qint64>& bins, float& mn, float& mx,
                        const uchar* tmp) {
      if (m_voxelType == _UChar)
	{
	  const uchar *ptr = tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      int val = (int)ptr[j];
	      if (val < mn) mn = (float)val;
	      if (val > mx) mx = (float)val;
	      int idx = val - rMinBin;
	      if (idx < 0) idx = 0; else if (idx >= histBins) idx = histBins-1;
	      bins[(size_t)idx]++;
	    }
	}
      else if (m_voxelType == _Char)
	{
	  const char *ptr = (const char*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      int val = (int)ptr[j];
	      if (val < mn) mn = (float)val;
	      if (val > mx) mx = (float)val;
	      int idx = val - rMinBin;
	      if (idx < 0) idx = 0; else if (idx >= histBins) idx = histBins-1;
	      bins[(size_t)idx]++;
	    }
	}
      else if (m_voxelType == _UShort)
	{
	  const ushort *ptr = (const ushort*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      int val = (int)ptr[j];
	      if (val < mn) mn = (float)val;
	      if (val > mx) mx = (float)val;
	      int idx = val - rMinBin;
	      if (idx < 0) idx = 0; else if (idx >= histBins) idx = histBins-1;
	      bins[(size_t)idx]++;
	    }
	}
      else if (m_voxelType == _Short)
	{
	  const short *ptr = (const short*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      int val = (int)ptr[j];
	      if (val < mn) mn = (float)val;
	      if (val > mx) mx = (float)val;
	      int idx = val - rMinBin;
	      if (idx < 0) idx = 0; else if (idx >= histBins) idx = histBins-1;
	      bins[(size_t)idx]++;
	    }
	}
    };

  std::vector<std::thread> pool;
  pool.reserve(numThreads);
  std::atomic<qint64> done((qint64)0);
  for (unsigned t = 0; t < numThreads; t++)
    {
      pool.emplace_back([&, t]() {
          const int plane0 = (int)((qint64)nX * t / numThreads);
          const int plane1 = (int)((qint64)nX * (t + 1) / numThreads);
          if (plane1 <= plane0)
            return;

          std::vector<uchar> tmp((size_t)nbytes);
          std::vector<qint64>& bins = local[(size_t)t];
          float mn = 10000000.0f, mx = -10000000.0f;
          for (int p = plane0; p < plane1; p++)
            {
              loadTiffImage(p, tmp.data());
              countPlane(bins, mn, mx, tmp.data());
              done.fetch_add(1, std::memory_order_relaxed);
            }
          localMin[(size_t)t] = mn;
          localMax[(size_t)t] = mx;
        });
    }

  while (done.load(std::memory_order_relaxed) < nX)
    {
      progress.setValue((int)(100.0 *
         (double)done.load(std::memory_order_relaxed) / (double)nX));
      qApp->processEvents();
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  for (auto& th : pool)
    th.join();

  float gMin = 10000000.0f, gMax = -10000000.0f;
  for (int b = 0; b < histBins; b++)
    {
      uint total = 0;
      for (unsigned t = 0; t < numThreads; t++)
        total += (uint)local[(size_t)t][(size_t)b];
      m_histogram[b] = total;
    }
  for (unsigned t = 0; t < numThreads; t++)
    {
      if (localMin[(size_t)t] < gMin) gMin = localMin[(size_t)t];
      if (localMax[(size_t)t] > gMax) gMax = localMax[(size_t)t];
    }

  m_rawMin = gMin;
  m_rawMax = gMax;

  progress.setValue(100);
  qApp->processEvents();
}

void
TiffPlugin::findMinMax()
{
  QProgressDialog progress("Finding Min and Max",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  const int nX = m_depth;
  const int nY = m_width;
  const int nZ = m_height;

  const int nbytes = nY*nZ*m_bytesPerVoxel;

  m_rawMin = 10000000;
  m_rawMax = -10000000;

  const unsigned hw = std::thread::hardware_concurrency();
  unsigned numThreads = hw ? hw : 1u;
  if (numThreads > (unsigned)nX) numThreads = (unsigned)nX;
  if (numThreads < 1u) numThreads = 1u;

  std::vector<float> localMin((size_t)numThreads, 10000000.0f);
  std::vector<float> localMax((size_t)numThreads, -10000000.0f);

  auto findPlane = [&](float& mn, float& mx, const uchar* tmp) {
      if (m_voxelType == _UChar)
	{
	  const uchar *ptr = tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = (float)ptr[j];
	      if (val < mn) mn = val;
	      if (val > mx) mx = val;
	    }
	}
      else if (m_voxelType == _Char)
	{
	  const char *ptr = (const char*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = (float)ptr[j];
	      if (val < mn) mn = val;
	      if (val > mx) mx = val;
	    }
	}
      else if (m_voxelType == _UShort)
	{
	  const ushort *ptr = (const ushort*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = (float)ptr[j];
	      if (val < mn) mn = val;
	      if (val > mx) mx = val;
	    }
	}
      else if (m_voxelType == _Short)
	{
	  const short *ptr = (const short*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = (float)ptr[j];
	      if (val < mn) mn = val;
	      if (val > mx) mx = val;
	    }
	}
      else if (m_voxelType == _Int)
	{
	  const int *ptr = (const int*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = (float)ptr[j];
	      if (val < mn) mn = val;
	      if (val > mx) mx = val;
	    }
	}
      else if (m_voxelType == _Float)
	{
	  const float *ptr = (const float*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = ptr[j];
	      if (val < mn) mn = val;
	      if (val > mx) mx = val;
	    }
	}
    };

  std::vector<std::thread> pool;
  pool.reserve(numThreads);
  std::atomic<qint64> done((qint64)0);
  for (unsigned t = 0; t < numThreads; t++)
    {
      pool.emplace_back([&, t]() {
          const int plane0 = (int)((qint64)nX * t / numThreads);
          const int plane1 = (int)((qint64)nX * (t + 1) / numThreads);
          if (plane1 <= plane0)
            return;

          std::vector<uchar> tmp((size_t)nbytes);
          float mn = 10000000.0f, mx = -10000000.0f;
          for (int p = plane0; p < plane1; p++)
            {
              loadTiffImage(p, tmp.data());
              findPlane(mn, mx, tmp.data());
              done.fetch_add(1, std::memory_order_relaxed);
            }
          localMin[(size_t)t] = mn;
          localMax[(size_t)t] = mx;
        });
    }

  while (done.load(std::memory_order_relaxed) < nX)
    {
      progress.setValue((int)(100.0 *
         (double)done.load(std::memory_order_relaxed) / (double)nX));
      qApp->processEvents();
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  for (auto& th : pool)
    th.join();

  float gMin = 10000000.0f, gMax = -10000000.0f;
  for (unsigned t = 0; t < numThreads; t++)
    {
      if (localMin[(size_t)t] < gMin) gMin = localMin[(size_t)t];
      if (localMax[(size_t)t] > gMax) gMax = localMax[(size_t)t];
    }

  m_rawMin = gMin;
  m_rawMax = gMax;

  progress.setValue(100);
  qApp->processEvents();
}

void
TiffPlugin::generateHistogram()
{
  QProgressDialog progress("Generating Histogram",
			   QString(),
			   0, 100,
			   0);
  progress.setMinimumDuration(0);


  float rSize = m_rawMax-m_rawMin;
  const int nX = m_depth;
  const int nY = m_width;
  const int nZ = m_height;

  const int nbytes = nY*nZ*m_bytesPerVoxel;

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

  const int histogramSize = m_histogram.size()-1;
  const int histBins = m_histogram.size();
  const float omin = m_rawMin;
  const float denomin = rSize;

  const unsigned hw = std::thread::hardware_concurrency();
  unsigned numThreads = hw ? hw : 1u;
  if (numThreads > (unsigned)nX) numThreads = (unsigned)nX;
  if (numThreads < 1u) numThreads = 1u;

  std::vector<std::vector<qint64> > local((size_t)numThreads,
                                          std::vector<qint64>((size_t)histBins, 0));

  auto countPlane = [&](std::vector<qint64>& bins, const uchar* tmp) {
      if (m_voxelType == _UChar)
	{
	  const uchar *ptr = tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = (float)ptr[j];
	      float fidx = (val-omin)/denomin;
	      fidx = qBound(0.0f, fidx, 1.0f);
	      int idx = (int)(fidx*histogramSize);
	      bins[(size_t)idx] += 1;
	    }
	}
      else if (m_voxelType == _Char)
	{
	  const char *ptr = (const char*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = (float)ptr[j];
	      float fidx = (val-omin)/denomin;
	      fidx = qBound(0.0f, fidx, 1.0f);
	      int idx = (int)(fidx*histogramSize);
	      bins[(size_t)idx] += 1;
	    }
	}
      else if (m_voxelType == _UShort)
	{
	  const ushort *ptr = (const ushort*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = (float)ptr[j];
	      float fidx = (val-omin)/denomin;
	      fidx = qBound(0.0f, fidx, 1.0f);
	      int idx = (int)(fidx*histogramSize);
	      bins[(size_t)idx] += 1;
	    }
	}
      else if (m_voxelType == _Short)
	{
	  const short *ptr = (const short*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = (float)ptr[j];
	      float fidx = (val-omin)/denomin;
	      fidx = qBound(0.0f, fidx, 1.0f);
	      int idx = (int)(fidx*histogramSize);
	      bins[(size_t)idx] += 1;
	    }
	}
      else if (m_voxelType == _Int)
	{
	  const int *ptr = (const int*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = (float)ptr[j];
	      float fidx = (val-omin)/denomin;
	      fidx = qBound(0.0f, fidx, 1.0f);
	      int idx = (int)(fidx*histogramSize);
	      bins[(size_t)idx] += 1;
	    }
	}
      else if (m_voxelType == _Float)
	{
	  const float *ptr = (const float*) tmp;
	  for (int j = 0; j < nY*nZ; j++)
	    {
	      float val = ptr[j];
	      float fidx = (val-omin)/denomin;
	      fidx = qBound(0.0f, fidx, 1.0f);
	      int idx = (int)(fidx*histogramSize);
	      bins[(size_t)idx] += 1;
	    }
	}
    };

  std::vector<std::thread> pool;
  pool.reserve(numThreads);
  std::atomic<qint64> done((qint64)0);
  for (unsigned t = 0; t < numThreads; t++)
    {
      pool.emplace_back([&, t]() {
          const int plane0 = (int)((qint64)nX * t / numThreads);
          const int plane1 = (int)((qint64)nX * (t + 1) / numThreads);
          if (plane1 <= plane0)
            return;

          std::vector<uchar> tmp((size_t)nbytes);
          std::vector<qint64>& bins = local[(size_t)t];
          for (int p = plane0; p < plane1; p++)
            {
              loadTiffImage(p, tmp.data());
              countPlane(bins, tmp.data());
              done.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

  while (done.load(std::memory_order_relaxed) < nX)
    {
      progress.setValue((int)(100.0 *
         (double)done.load(std::memory_order_relaxed) / (double)nX));
      qApp->processEvents();
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
  for (auto& th : pool)
    th.join();

  for (int b = 0; b < histBins; b++)
    {
      uint total = 0;
      for (unsigned t = 0; t < numThreads; t++)
        total += (uint)local[(size_t)t][(size_t)b];
      m_histogram[b] = total;
    }

  progress.setValue(100);
  qApp->processEvents();
}

void
TiffPlugin::getDepthSlice(int slc,
			  uchar *slice)
{
  int nbytes = m_width*m_height*m_bytesPerVoxel;

  uchar *tmp1 = new uchar[nbytes];

  loadTiffImage(slc, tmp1);

  if (m_voxelType == _UChar)
    {
      for(uint j=0; j<m_width; j++)
	for(uint k=0; k<m_height; k++)
	  slice[j*m_height+k] = tmp1[k*m_width+j];
    }
  else if (m_voxelType == _UShort)
    {
      ushort *p0 = (ushort*)slice;
      ushort *p1 = (ushort*)tmp1;
      for(uint j=0; j<m_width; j++)
	for(uint k=0; k<m_height; k++)
	  p0[j*m_height+k] = p1[k*m_width+j];
    }
  else if (m_voxelType == _Float)
    {
      float *p0 = (float*)slice;
      float *p1 = (float*)tmp1;
      for(uint j=0; j<m_width; j++)
	for(uint k=0; k<m_height; k++)
	  p0[j*m_height+k] = p1[k*m_width+j];
    }
  delete [] tmp1;
}

//void
//TiffPlugin::getWidthSlice(int slc,
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
//      loadTiffImage(i, imgSlice);
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
//TiffPlugin::getHeightSlice(int slc,
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
//      loadTiffImage(i, imgSlice);
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
TiffPlugin::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }

  uchar *tmp = new uchar[10];
  uchar *imgSlice = new uchar[m_width*m_height*m_bytesPerVoxel];

  loadTiffImage(d, imgSlice);

  if (m_voxelType == _UChar)
    {
      uchar a = imgSlice[h*m_width+w];
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _UShort)
    {
      ushort a = ((ushort*)imgSlice)[h*m_width+w];
      v = QVariant((uint)a);
    }
  else if (m_voxelType == _Float)
    {
      float a = ((float*)imgSlice)[h*m_width+w];
      v = QVariant((uint)a);
    }

  return v;
}

//void
//TiffPlugin::saveTrimmed(QString trimFile,
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
//      loadTiffImage(i, tmp1);
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
