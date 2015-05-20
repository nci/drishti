#include "volume.h"
#include "staticfunctions.h"
#include "global.h"

void Volume::setBitmapThread(BitmapThread *bt) {thread = bt;}

void Volume::saveIntermediateResults() { m_mask.saveIntermediateResults(); }

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

  m_nonZeroVoxels = 0;
  m_bitmask.clear();
  m_connectedbitmask.clear();

  connect(&m_mask, SIGNAL(progressChanged(int)),
	  this, SIGNAL(progressChanged(int)));
  connect(&m_mask, SIGNAL(progressReset()),
	  this, SIGNAL(progressReset()));
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
  m_bitmask.clear();
  m_connectedbitmask.clear();
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
  float inmemGB = 0.3+((float)m_depth*m_width*m_height*2.5)/((float)1024*1024*1024);
  bool inMem = true;
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

  m_bitmask.resize((qint64)m_depth*m_width*m_height);
  m_bitmask.fill(false);
  m_connectedbitmask.resize((qint64)m_depth*m_width*m_height);
  m_connectedbitmask.fill(false);

  m_valid = true;

  return true;
}

//void
//Volume::createGradVolume()
//{
//  if (m_gradFileManager.exists())
//    return;
//
//  m_gradFileManager.createFile(true);
//
//  int minx = 0;
//  int miny = 0;
//  int minz = 0;
//  int maxx = m_height;
//  int maxy = m_width;
//  int maxz = m_depth;
//
//  int lenx = m_height;
//  int leny = m_width;
//  int lenz = m_depth;
//  
//  QProgressDialog progress(QString("saving val+grad volume"),
//			   "Cancel",
//			   0, 100,
//			   0);
//  progress.setMinimumDuration(0);
//
//
//  int nbytes = m_width*m_height;
//  uchar *tmp, *g0, *g1, *g2;
//  tmp = new uchar [nbytes];
//  g0  = new uchar [nbytes];
//  g1  = new uchar [nbytes];
//  g2  = new uchar [nbytes];
//
//  memset(g0, 0, nbytes);
//  memset(g1, 0, nbytes);
//  memset(g2, 0, nbytes);
//
//  for(int kslc=0; kslc<m_depth; kslc++)
//    {
//      progress.setValue((int)(100.0*(float)kslc/(float)m_depth));
//      qApp->processEvents();
//
//      uchar *vslice;
//      vslice = m_pvlFileManager.getSlice(kslc);
//
//      memcpy(g2, vslice, nbytes);
//
//      memset(tmp, 0, nbytes);
//
//      if (kslc >= 2)
//	{
//	  for(int j=1; j<m_width-1; j++)
//	    for(int i=1; i<m_height-1; i++)
//	      {
//		int gx = g1[j*m_height+(i+1)] - g1[j*m_height+(i-1)];
//		int gy = g1[(j+1)*m_height+i] - g1[(j-1)*m_height+i];
//		int gz = g2[j*m_height+i] - g0[j*m_height+i];
//		int gsum = sqrtf(gx*gx+gy*gy+gz*gz);
//		gsum = qBound(0, gsum, 255);
//		tmp[j*m_height+i] = gsum;
//	      }
//	}
//
//      m_gradFileManager.setSlice(kslc, tmp);
//
//      uchar *gt = g0;
//      g0 = g1;
//      g1 = g2;
//      g2 = gt;
//    }
//
//  delete [] tmp;
//  delete [] g0;
//  delete [] g1;
//  delete [] g2;
//
//  progress.setValue(100);
//  qApp->processEvents();
//}

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

      //vslice = m_gradFileManager.getSlice(slc);
      
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

//  vslice = m_gradFileManager.getSlice(slc);
//  for(int t=0; t<nbytes; t++)
//    m_slice[2*t+1] = vslice[t];

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

//  vslice = m_gradFileManager.getWidthSlice(slc);
//  for(int t=0; t<nbytes; t++)
//    m_slice[2*t+1] = vslice[t];

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

//  vslice = m_gradFileManager.getHeightSlice(slc);
//  for(int t=0; t<nbytes; t++)
//    m_slice[2*t+1] = vslice[t];

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
//  vslice = m_gradFileManager.rawValue(d, w, h);
//  vgt << vslice[0];
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
Volume::dilateVolume()
{
  m_mask.dilate(m_bitmask);
}
void
Volume::dilateVolume(int mind, int maxd,
		     int minw, int maxw,
		     int minh, int maxh)
{
  m_mask.dilate(mind, maxd,
		minw, maxw,
		minh, maxh,
		m_bitmask);
}

void
Volume::erodeVolume()
{
  m_mask.erode(m_bitmask);
}
void
Volume::erodeVolume(int mind, int maxd,
		    int minw, int maxw,
		    int minh, int maxh)
{
  m_mask.erode(mind, maxd,
	       minw, maxw,
	       minh, maxh,
	       m_bitmask);
}

void
Volume::fillVolume(int mind, int maxd,
		   int minw, int maxw,
		   int minh, int maxh,
		   QList<int> dwh,
		   bool sliceOnly)
{
  findConnectedRegion(mind, maxd,
		      minw, maxw,
		      minh, maxh,
		      dwh,
		      sliceOnly);

  m_mask.tagUsingBitmask(dwh,
			 m_connectedbitmask);
}

void
Volume::tagAllVisible(int mind, int maxd,
		      int minw, int maxw,
		      int minh, int maxh)
{
  markVisibleRegion(mind, maxd,
		    minw, maxw,
		    minh, maxh);

  // dummy pos
  QList<int> pos;
  pos.clear();

  m_mask.tagUsingBitmask(pos, m_bitmask);
}

void
Volume::createBitmask()
{
  return;
}

void
Volume::tagDSlice(int currslice, uchar *usermask, bool flag)
{
  if (flag)
    m_mask.tagDSlice(currslice, usermask);
  else
    m_mask.tagDSlice(currslice, m_bitmask, usermask);
}

void
Volume::tagWSlice(int currslice, uchar *usermask, bool flag)
{
  if (flag)
    m_mask.tagWSlice(currslice, usermask);
  else
    m_mask.tagWSlice(currslice, m_bitmask, usermask);
}

void
Volume::tagHSlice(int currslice, uchar *usermask, bool flag)
{
  if (flag)
    m_mask.tagHSlice(currslice, usermask);
  else
    m_mask.tagHSlice(currslice, m_bitmask, usermask);
}


void
Volume::findConnectedRegion(int mind, int maxd,
			    int minw, int maxw,
			    int minh, int maxh,
			    QList<int> pos,
			    bool sliceOnly)
{
  m_connectedbitmask.fill(false);

  uchar *lut = Global::lut();
  QStack<int> stack;

  uchar *vslice;
  uchar v0, g0;
  int d = pos[0];
  int w = pos[1];
  int h = pos[2];
  vslice = m_pvlFileManager.rawValueMem(d, w, h);
  v0 = vslice[0];
//  vslice = m_gradFileManager.rawValue(d, w, h);
//  g0 = vslice[0];
  g0 = 0;


  // put the seeds in
  for(int pi=0; pi<pos.size()/3; pi++)
    {
      int d = pos[3*pi];
      int w = pos[3*pi+1];
      int h = pos[3*pi+2];
      if (d >= mind && d <= maxd &&
	  w >= minw && w <= maxw &&
	  h >= minh && h <= maxh)
	{
	  qint64 idx = d*m_width*m_height + w*m_height + h;
	  if (m_bitmask.testBit(idx))
	    {
	      m_connectedbitmask.setBit(idx);
	      stack.push(d);
	      stack.push(w);
	      stack.push(h);
	    }
	}
    }

  if (sliceOnly)
    stack.clear();

  int pv = 0;
  int progv = 0;
  while(!stack.isEmpty())
    {
      progv++;
      if (progv == 1000)
	{
	  progv = 0;
	  pv = (++pv % 100);
	  emit progressChanged(pv);	  
	}

      int h = stack.pop();
      int w = stack.pop();
      int d = stack.pop();
      qint64 idx = d*m_width*m_height + w*m_height + h;
      if (m_bitmask.testBit(idx))
	{
	  int d0 = qMax(d-1, 0);
	  int d1 = qMin(d+1, m_depth-1);
	  int w0 = qMax(w-1, 0);
	  int w1 = qMin(w+1, m_width-1);
	  int h0 = qMax(h-1, 0);
	  int h1 = qMin(h+1, m_height-1);
	      
	  for(int d2=d0; d2<=d1; d2++)
	    for(int w2=w0; w2<=w1; w2++)
	      for(int h2=h0; h2<=h1; h2++)
		{
		  if (d2 >= mind && d2 <= maxd &&
		      w2 >= minw && w2 <= maxw &&
		      h2 >= minh && h2 <= maxh)
		    {
		      qint64 idx = d2*m_width*m_height +
			           w2*m_height + h2;
		      if ( m_bitmask.testBit(idx) &&
			   !m_connectedbitmask.testBit(idx) )
			{
//			  uchar v, g;
//			  vslice = m_pvlFileManager.rawValue(d2, w2, h2);
//			  v = vslice[0];
//			  vslice = m_gradFileManager.rawValue(d2, w2, h2);
//			  g = vslice[0];
//
//			  if (qAbs(v-v0) < Global::deltaV() &&
//			      g < Global::deltaG())
			    {
			      m_connectedbitmask.setBit(idx);
			      stack.push(d2);
			      stack.push(w2);
			      stack.push(h2);
			    }
			}
		    }
		}
	}
    } // end find connected
  //------------------------------------------------------

  emit progressReset();
}

void
Volume::markVisibleRegion(int mind, int maxd,
			  int minw, int maxw,
			  int minh, int maxh)
{
  m_connectedbitmask.fill(false);

  for(int d=mind; d<=maxd; d++)
    for(int w=minw; w<=maxw; w++)
      for(int h=minh; h<=maxh; h++)
	{
	  qint64 idx = d*m_width*m_height + w*m_height + h;
	  // copy bitmask to connectedbitmask
	  m_connectedbitmask.setBit(idx,
				    m_bitmask.testBit(idx));
	}
}

