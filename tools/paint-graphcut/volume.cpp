#include "volume.h"
#include "staticfunctions.h"
#include "global.h"
#include "getmemorysize.h"

void
Volume::saveIntermediateResults(bool forceSave)
{
  m_mask.saveIntermediateResults(forceSave);
}

void
Volume::saveMaskBlock(int d, int w, int h, int rad)
{
  m_mask.saveMaskBlock(d, w, h, rad);
}

void
Volume::saveMaskBlock(QList< QList<int> > bl)
{
  m_mask.saveMaskBlock(bl);
}

void
Volume::offLoadMemFile()
{
  m_mask.offLoadMemFile();
  m_pvlFileManager.setMemMapped(false);
}

void
Volume::loadMemFile()
{
  m_pvlFileManager.loadMemFile();
  m_mask.loadMemFile();
}

void
Volume::setMaskDepthSlice(int slc, uchar* tagData)
{
  m_mask.setMaskDepthSlice(slc, tagData);
}

uchar*
Volume::getMaskDepthSliceImage(int slc)
{
  return m_mask.getMaskDepthSliceImage(slc);
}

uchar*
Volume::getMaskWidthSliceImage(int slc)
{
  return m_mask.getMaskWidthSliceImage(slc);
}

uchar*
Volume::getMaskHeightSliceImage(int slc)
{
  return m_mask.getMaskHeightSliceImage(slc);
}

Volume::Volume()
{
  m_valid = false;

  m_depth = m_height = m_width = 0;
  m_slice = 0;
  m_fileName.clear();

  m_mask.reset();

  m_1dHistogram = 0;
  m_2dHistogram = 0;
  m_histImageData1D = 0;
  m_histImageData2D = 0;

  m_histogramImage1D = QImage(256, 256, QImage::Format_RGB32);
  m_histogramImage2D = QImage(256, 256, QImage::Format_RGB32);
}

Volume::~Volume() { reset(); }

bool Volume::isValid() { return m_valid; }

void Volume::reset()
{
  m_valid = false;

  m_pvlFileManager.reset();

  m_mask.reset();

  m_fileName.clear();

  if (m_slice) delete [] m_slice;
  if (m_1dHistogram) delete [] m_1dHistogram;
  if (m_2dHistogram) delete [] m_2dHistogram;
  if (m_histImageData1D) delete m_histImageData1D;
  if (m_histImageData2D) delete m_histImageData2D;

  m_slice = 0;
  m_depth = m_height = m_width = 0;
  m_1dHistogram = 0;
  m_2dHistogram = 0;
  m_histImageData1D = 0;
  m_histImageData2D = 0;
}


void
Volume::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

bool
Volume::setFile(QString volfile)
{
  reset();

  if (!StaticFunctions::xmlHeaderFile(volfile))
    {
      QMessageBox::information(0, "Error",
	QString("%1 is not a valid preprocessed volume file").
			       arg(volfile));
      return false;
    }

  StaticFunctions::getDimensionsFromHeader(volfile,
					   m_depth, m_width, m_height);

  int slabSize = StaticFunctions::getSlabsizeFromHeader(volfile);

  m_fileName = volfile;

  int headerSize = StaticFunctions::getPvlHeadersizeFromHeader(volfile);
  QStringList pvlnames = StaticFunctions::getPvlNamesFromHeader(volfile);
  if (pvlnames.count() > 0)
    m_pvlFileManager.setFilenameList(pvlnames);
  m_pvlFileManager.setBaseFilename(m_fileName);
  m_pvlFileManager.setDepth(m_depth);
  m_pvlFileManager.setWidth(m_width);
  m_pvlFileManager.setHeight(m_height);
  m_pvlFileManager.setHeaderSize(headerSize);
  m_pvlFileManager.setSlabSize(slabSize);

  //----------------
  float memSize = getMemorySize();
  memSize/=1024;
  memSize/=1024;
  memSize/=1024;
//  QMessageBox::information(0, "", QString("Physical Memory : %1 GB").arg(memSize));
  float inmemGB = 0.3+((float)m_depth*m_width*m_height*2.5)/((float)1024*1024*1024);
  bool inMem = true;
  if (inmemGB > memSize) // ask when memory requirements greater than physical memory detected
    {
      bool ok;
      QStringList dtypes;
      dtypes.clear();
      dtypes << "Yes"
	     << "No";
      QString option = QInputDialog::getItem(0,
					     "Memory Mapped File",
					     QString("Load volume in memory for fast operations ?\nYou will need atleast %1 Gb").arg(inmemGB),
					     dtypes,
					     0,
					     false,
					     &ok);
      if (ok && option == "No") inMem = false;
    }
  //----------------
  m_pvlFileManager.setMemMapped(inMem);


  m_pvlFileManager.loadMemFile();

  QString mfile = m_fileName;
  mfile.chop(6);
  mfile += QString("mask");
  m_mask.setFile(mfile, inMem);
  m_mask.setGridSize(m_depth, m_width, m_height, slabSize);

  genHistogram();
  generateHistogramImage();

  m_valid = true;

  return true;
}

void
Volume::genHistogram()
{
  // evaluate 1D & 2D histograms

  if (! m_1dHistogram) m_1dHistogram = new int[256];
  if (! m_2dHistogram) m_2dHistogram = new int[256*256];
  memset(m_1dHistogram, 0, 256*4);
  memset(m_2dHistogram, 0, 256*256*4);

  // if available read histogram from file
  QString hfilename = m_fileName;
  hfilename.chop(6);
  hfilename += QString("hist");
  QFile hfile;
  hfile.setFileName(hfilename);
  if (hfile.exists() == true &&
      hfile.size() == 256*4)    
    {
      hfile.open(QFile::ReadOnly);
      hfile.read((char*)m_1dHistogram, 256*4);      
      hfile.close();
      return;
    }

  float *flhist1D = new float[256];
  float *flhist2D = new float[256*256];
  memset(flhist1D, 0, 256*4);      
  memset(flhist2D, 0, 256*256*4);

  QProgressDialog progress("Histogram Generation",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);
  qApp->processEvents();

  int nbytes = m_width*m_height;
  uchar *v = new unsigned char [nbytes];
  char *tmp = new char[2*nbytes];

  for(int slc=0; slc<m_depth; slc++)
    {
      progress.setValue((int)(100.0*(float)slc/(float)m_depth));
      qApp->processEvents();

      uchar *vslice;
      vslice = m_pvlFileManager.getSliceMem(slc);
      memcpy(v, vslice, nbytes);

      for(int j=0; j<nbytes; j++)
	flhist1D[v[j]]++;
      
      for(int j=0; j<nbytes; j++)
	flhist2D[vslice[j]*256 + v[j]]++;
    }

  delete [] v;

  StaticFunctions::generateHistograms(flhist1D, flhist2D,
				      m_1dHistogram, m_2dHistogram);

  delete [] flhist1D;
  delete [] flhist2D;

  hfile.open(QFile::WriteOnly);
  hfile.write((char*)m_1dHistogram, 256*4);      
  hfile.close();

  progress.setValue(100);
}

uchar*
Volume::getDepthSliceImage(int slc)
{
  if (m_slice) delete [] m_slice;
  
  int nbytes = m_width*m_height;
  m_slice = new uchar[2*nbytes];
  memset(m_slice, 0, 2*nbytes);

  uchar *vslice;
  vslice = m_pvlFileManager.getSliceMem(slc);
  for(int t=0; t<nbytes; t++)
    m_slice[2*t] = vslice[t];

  return m_slice;
}

uchar*
Volume::getWidthSliceImage(int slc)
{
  if (m_slice) delete [] m_slice;

  int nbytes = m_depth*m_height;
  m_slice = new uchar[2*nbytes];
  memset(m_slice, 0, 2*nbytes);

  uchar *vslice;
  vslice = m_pvlFileManager.getWidthSliceMem(slc);
  for(int t=0; t<nbytes; t++)
    m_slice[2*t] = vslice[t];

  return m_slice;
}

uchar*
Volume::getHeightSliceImage(int slc)
{
  if (m_slice) delete [] m_slice;

  int nbytes = m_depth*m_width;
  m_slice = new uchar[2*nbytes];
  memset(m_slice, 0, 2*nbytes);

  uchar *vslice;
  vslice = m_pvlFileManager.getHeightSliceMem(slc);
  for(int t=0; t<nbytes; t++)
    m_slice[2*t] = vslice[t];

  return m_slice;
}


QList<uchar>
Volume::rawValue(int d, int w, int h)
{
  QList<uchar> vgt;
  vgt.clear();

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return vgt;

  uchar *vslice;
  vslice = m_pvlFileManager.rawValueMem(d, w, h);
  vgt << vslice[0];
  vgt << 0;
  
  vgt << m_mask.maskValue(d,w,h);

  return vgt;
}

void
Volume::generateHistogramImage()
{
  if(! m_histImageData1D)
    m_histImageData1D = new uchar[256*256*4];
  if(! m_histImageData2D)
    m_histImageData2D = new uchar[256*256*4];

  memset(m_histImageData1D, 0, 256*256*4);
  memset(m_histImageData2D, 0, 256*256*4);

  int *hist2D = m_2dHistogram;
  for (int i=0; i<256*256; i++)
    {
      m_histImageData2D[4*i + 3] = 255;
      m_histImageData2D[4*i + 0] = hist2D[i];
      m_histImageData2D[4*i + 1] = hist2D[i];
      m_histImageData2D[4*i + 2] = hist2D[i];
    }
  m_histogramImage2D = QImage(m_histImageData2D,
			      256, 256,
			      QImage::Format_ARGB32);
  m_histogramImage2D = m_histogramImage2D.mirrored();  


  int *hist1D = m_1dHistogram;
  memset(m_histImageData1D, 0, 4*256*256);
  for (int i=0; i<256; i++)
    {
      for (int j=0; j<256; j++)
	{
	  int idx = 256*j + i;
	  m_histImageData1D[4*idx + 3] = 255;
	}

      int h = hist1D[i];
      for (int j=0; j<h; j++)
	{
	  int idx = 256*j + i;
	  m_histImageData1D[4*idx + 0] = 255*j/h;
	  m_histImageData1D[4*idx + 1] = 255*j/h;
	  m_histImageData1D[4*idx + 2] = 255*j/h;
	}
    }
  m_histogramImage1D = QImage(m_histImageData1D,
			      256, 256,
			      QImage::Format_ARGB32);
  m_histogramImage1D = m_histogramImage1D.mirrored();  
}

void
Volume::tagDSlice(int currslice, uchar *usermask)
{
  m_mask.tagDSlice(currslice, usermask);
}

void
Volume::tagWSlice(int currslice, uchar *usermask)
{
  m_mask.tagWSlice(currslice, usermask);
}

void
Volume::tagHSlice(int currslice, uchar *usermask)
{
  m_mask.tagHSlice(currslice, usermask);
}
