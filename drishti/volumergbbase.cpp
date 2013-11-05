#include <QtGui>

#include "volumergbbase.h"
#include "global.h"
#include "staticfunctions.h"
#include "volumefilemanager.h"
#include "mainwindowui.h"
#include "xmlheaderfunctions.h"
#include "volumeinformation.h"

#include <fstream>
using namespace std;


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
  else if (vn == 1)
    return m_1dHistogramG;
  else if (vn == 2)
    return m_1dHistogramB;
  else if (vn == 3)
    return m_1dHistogramA;

  return NULL;
}

int*
VolumeRGBBase::getLowres2dHistogram(int vn)
{
  if (vn == 0)
    return m_2dHistogramR;
  else if (vn == 1)
    return m_2dHistogramG;
  else if (vn == 2)
    return m_2dHistogramB;
  else if (vn == 3)
    return m_2dHistogramA;

  return NULL;
}


VolumeRGBBase::VolumeRGBBase()
{
  m_1dHistogramR = new int[256];
  memset(m_1dHistogramR, 0, 256*4);

  m_2dHistogramR = new int[256*256];
  memset(m_2dHistogramR, 0, 256*256*4);

  m_1dHistogramG = new int[256];
  memset(m_1dHistogramG, 0, 256*4);

  m_2dHistogramG = new int[256*256];
  memset(m_2dHistogramG, 0, 256*256*4);

  m_1dHistogramB = new int[256];
  memset(m_1dHistogramB, 0, 256*4);

  m_2dHistogramB = new int[256*256];
  memset(m_2dHistogramB, 0, 256*256*4);

  m_1dHistogramA = new int[256];
  memset(m_1dHistogramA, 0, 256*4);

  m_2dHistogramA = new int[256*256];
  memset(m_2dHistogramA, 0, 256*256*4);

  m_lowresVolume = m_lowresTextureVolume = 0;
}

VolumeRGBBase::~VolumeRGBBase()
{
  if (m_1dHistogramR) delete [] m_1dHistogramR;
  if (m_2dHistogramR) delete [] m_2dHistogramR;
  if (m_1dHistogramG) delete [] m_1dHistogramG;
  if (m_2dHistogramG) delete [] m_2dHistogramG;
  if (m_1dHistogramB) delete [] m_1dHistogramB;
  if (m_2dHistogramB) delete [] m_2dHistogramB;
  if (m_1dHistogramA) delete [] m_1dHistogramA;
  if (m_2dHistogramA) delete [] m_2dHistogramA;

  if (m_lowresVolume) delete [] m_lowresVolume;
  if (m_lowresTextureVolume) delete [] m_lowresTextureVolume;

  m_1dHistogramR = m_2dHistogramR = 0;
  m_1dHistogramG = m_2dHistogramG = 0;
  m_1dHistogramB = m_2dHistogramB = 0;
  m_1dHistogramA = m_2dHistogramA = 0;

  m_lowresVolume = m_lowresTextureVolume = 0;
}

bool
VolumeRGBBase::loadVolume(const char* volfile, bool redo)
{
  if (!VolumeInformation::xmlHeaderFile(volfile))
    {
      QMessageBox::information(0, "Error",
	   QString("%1 is not a valid colour volume file").arg(volfile));
      return false;
    }

  m_volumeFile = volfile;
  XmlHeaderFunctions::getDimensionsFromHeader(m_volumeFile,
					      m_depth, m_width, m_height);
  m_fullVolumeSize = Vec(m_height, m_width, m_depth);


  generateHistograms(redo);
  createLowresVolume(redo);
  createLowresTextureVolume();

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle("Drishti - Volume Exploration and Presentation Tool");

  return true;
}

void
VolumeRGBBase::generateHistograms(bool redo)
{
  //---------------
  int nRGB = 3;
  if (Global::volumeType() == Global::RGBAVolume)
    nRGB = 4;
  //---------------

  memset(m_1dHistogramR, 0, 256*4);
  memset(m_2dHistogramR, 0, 256*256*4);
  memset(m_1dHistogramG, 0, 256*4);
  memset(m_2dHistogramG, 0, 256*256*4);
  memset(m_1dHistogramB, 0, 256*4);
  memset(m_2dHistogramB, 0, 256*256*4);
  memset(m_1dHistogramA, 0, 256*4);
  memset(m_2dHistogramA, 0, 256*256*4);


  float *flhist1DR, *flhist2DR;
  float *flhist1DG, *flhist2DG;
  float *flhist1DB, *flhist2DB;
  float *flhist1DA, *flhist2DA;

  flhist1DR = new float[256];       memset(flhist1DR, 0, 256*4);
  flhist2DR = new float[256*256];   memset(flhist2DR, 0, 256*256*4);
  flhist1DG = new float[256];       memset(flhist1DG, 0, 256*4);
  flhist2DG = new float[256*256];   memset(flhist2DG, 0, 256*256*4);
  flhist1DB = new float[256];       memset(flhist1DB, 0, 256*4);
  flhist2DB = new float[256*256];   memset(flhist2DB, 0, 256*256*4);
  if (nRGB == 4)
    {
      flhist1DA = new float[256];       memset(flhist1DA, 0, 256*4);
      flhist2DA = new float[256*256];   memset(flhist2DA, 0, 256*256*4);
    }
  //----------------------------- process data ------------------------
  uchar *tmp[3];
  tmp[0] = new unsigned char [m_width*m_height];  
  tmp[1] = new unsigned char [m_width*m_height];  
  tmp[2] = new unsigned char [m_width*m_height];  
  if (nRGB==4)
    tmp[3] = new unsigned char [m_width*m_height];  



  int slabSize = XmlHeaderFunctions::getSlabsizeFromHeader(m_volumeFile);
  QString rgbfile = m_volumeFile;
  rgbfile.chop(6);
  VolumeFileManager rgbaFileManager[4];
  QString rFilename = rgbfile + QString("red");
  QString gFilename = rgbfile + QString("green");
  QString bFilename = rgbfile + QString("blue");
  QString aFilename = rgbfile + QString("alpha");

  rgbaFileManager[0].setBaseFilename(rFilename);
  rgbaFileManager[1].setBaseFilename(gFilename);
  rgbaFileManager[2].setBaseFilename(bFilename);
  rgbaFileManager[3].setBaseFilename(aFilename);
  for(int a=0; a<nRGB; a++)
    {
      rgbaFileManager[a].setDepth(m_depth);
      rgbaFileManager[a].setWidth(m_width);
      rgbaFileManager[a].setHeight(m_height);
      rgbaFileManager[a].setHeaderSize(13);
      rgbaFileManager[a].setSlabSize(slabSize);
    }


  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Reading Volume"));
  Global::progressBar()->show();

  int nbytes = m_width*m_height;
  for(int k=0; k<m_depth; k++)
    {
      Global::progressBar()->setValue((int)((float)k/(float)m_depth));
      qApp->processEvents();
      for (int a=0; a<nRGB; a++)
	{
	  uchar *vslice = rgbaFileManager[a].getSlice(k);
	  memcpy(tmp[a], vslice, nbytes);
	}

      for (int j=0; j<m_width*m_height; j++)
	{
	  uchar r = tmp[0][j];
	  uchar g = tmp[1][j];
	  uchar b = tmp[2][j];

	  flhist1DR[r]++; flhist2DR[g*256 + r]++;
	  flhist1DG[g]++; flhist2DG[b*256 + g]++;
	  flhist1DB[b]++; flhist2DB[r*256 + b]++;

	  if (nRGB == 4)
	    {
	      uchar a = tmp[3][j];
	      uchar rgb = qMax(r, qMax(g, b));
	      flhist1DA[a]++; flhist2DA[rgb*256 + a]++;
	    }
	}
    }

  Global::progressBar()->setValue(100);
  Global::hideProgressBar();

  delete [] tmp[0];
  delete [] tmp[1];
  delete [] tmp[2];
  if (nRGB == 4)
    delete [] tmp[3];

  StaticFunctions::generateHistograms(flhist1DR, flhist2DR,
				      m_1dHistogramR, m_2dHistogramR);

  StaticFunctions::generateHistograms(flhist1DG, flhist2DG,
				      m_1dHistogramG, m_2dHistogramG);

  StaticFunctions::generateHistograms(flhist1DB, flhist2DB,
				      m_1dHistogramB, m_2dHistogramB);

  if (nRGB == 4)
    StaticFunctions::generateHistograms(flhist1DA, flhist2DA,
					m_1dHistogramA, m_2dHistogramA);
  delete [] flhist1DR;
  delete [] flhist2DR;
  delete [] flhist1DG;
  delete [] flhist2DG;
  delete [] flhist1DB;
  delete [] flhist2DB;
  if (nRGB == 4)
    {
      delete [] flhist1DA;
      delete [] flhist2DA;
    }
}

void
VolumeRGBBase::createLowresVolume(bool redo)
{
  //---------------
  int nRGB = 3;
  if (Global::volumeType() == Global::RGBAVolume)
    nRGB = 4;
  //---------------


  int px2, py2, pz2;
  px2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.x);
  py2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.y);
  pz2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.z);

  m_subSamplingLevel = 1;
  while (px2+py2+pz2 > qMin(26, Global::textureSize()-1)) // limit lowres volume size
    {
      m_subSamplingLevel *= 2;
      px2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.x/m_subSamplingLevel);
      py2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.y/m_subSamplingLevel);
      pz2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.z/m_subSamplingLevel);
    }

  // max hardware texture size is 512x512x512
  if ((px2 > 9 || py2 > 9 || pz2 > 9) &&
      m_subSamplingLevel == 1)
    m_subSamplingLevel = 2;


  m_lowresVolumeSize = m_fullVolumeSize/m_subSamplingLevel;
  int height= m_lowresVolumeSize.x;
  int width = m_lowresVolumeSize.y;
  int depth = m_lowresVolumeSize.z;

  if (m_lowresVolume) delete [] m_lowresVolume;
  
  m_lowresVolume = new unsigned char[nRGB*height*width*depth];

  int slabSize = XmlHeaderFunctions::getSlabsizeFromHeader(m_volumeFile);
  QString rgbfile = m_volumeFile;
  rgbfile.chop(6);
  VolumeFileManager rgbaFileManager[4];
  QString rFilename = rgbfile + QString("red");
  QString gFilename = rgbfile + QString("green");
  QString bFilename = rgbfile + QString("blue");
  QString aFilename = rgbfile + QString("alpha");

  rgbaFileManager[0].setBaseFilename(rFilename);
  rgbaFileManager[1].setBaseFilename(gFilename);
  rgbaFileManager[2].setBaseFilename(bFilename);
  rgbaFileManager[3].setBaseFilename(aFilename);
  for(int a=0; a<nRGB; a++)
    {
      rgbaFileManager[a].setDepth(m_depth);
      rgbaFileManager[a].setWidth(m_width);
      rgbaFileManager[a].setHeight(m_height);
      rgbaFileManager[a].setHeaderSize(13);
      rgbaFileManager[a].setSlabSize(slabSize);
    }


  int nbytes = m_width*m_height;
  if (m_subSamplingLevel == 1)
    {
      uchar *tmp;
      tmp = new uchar [m_width*m_height];  
      for(int k=0; k<m_depth; k++)
	{
	  for (int a=0; a<nRGB; a++)
	    {
	      uchar *vslice = rgbaFileManager[a].getSlice(k);
	      memcpy(tmp, vslice, nbytes);

	      for (int j=0; j<m_width*m_height; j++)
		m_lowresVolume[nRGB*(k*m_width*m_height+j) + a] = tmp[j];
	    }
	}
    }
  else
    {
      int iend,jend,kend; 
      iend = height;
      jend = width;
      kend = depth;

      uchar *tmp;
      tmp = new uchar [m_width*m_height];

      int totcount = 2*m_subSamplingLevel-1;
      int count=0;
      uchar **volX;
      volX = 0;
      if (m_subSamplingLevel > 1)
	{
	  volX = new unsigned char*[totcount];
	  for(int i=0; i<totcount; i++)
	    volX[i] = new unsigned char[nRGB*jend*iend];
	}
      uchar *tmp1;
      ushort *tmp1i;
      tmp1 = new uchar [nRGB*jend*iend];
      tmp1i = new ushort [nRGB*jend*iend];

      MainWindowUI::mainWindowUI()->menubar->parentWidget()->		\
	setWindowTitle(QString("Generating Lowres Version"));
      Global::progressBar()->show();

      int kslc=0;
      for(int k=0; k<m_depth; k++)
	{
	  Global::progressBar()->setValue((int)(100.0*(float)k/(float)m_depth));
	  qApp->processEvents();
      
	  // box-filter and scaledown the slice
	  for (int a=0; a<nRGB; a++)
	    {
	      uchar *vslice = rgbaFileManager[a].getSlice(k);
	      memcpy(tmp, vslice, nbytes);

	      int ji=0;
	      for(int j=0; j<jend; j++)
		{ 
		  int y = j*m_subSamplingLevel;
		  int loy = qMax(0, y-m_subSamplingLevel+1);
		  int hiy = qMin(m_width-1, y+m_subSamplingLevel-1);
		  for(int i=0; i<iend; i++) 
		    { 
		      int x = i*m_subSamplingLevel; 
		      int lox = qMax(0, x-m_subSamplingLevel+1);
		      int hix = qMin(m_height-1, x+m_subSamplingLevel-1);
		      
		      float sum = 0;
		      for(int jy=loy; jy<=hiy; jy++) 
			{
			  for(int ix=lox; ix<=hix; ix++) 
			    {
			      int idx = jy*m_height+ix;
			      sum += tmp[idx];
			    }
			}
		      
		      tmp1[nRGB*ji+a] = sum/((hiy-loy+1)*(hix-lox+1)); 
		      
		      ji++;
		    } 
		}
	    }

	  unsigned char *vptr;
	  vptr = volX[0];
	  for (int c=0; c<totcount-1; c++)
	    volX[c] = volX[c+1];
	  volX[totcount-1] = vptr;
	  
	  memcpy(volX[totcount-1], tmp1, nRGB*jend*iend);
      
	  count ++;
	  if (count == totcount)
	    {
	      memset(tmp1i, 0, 2*nRGB*jend*iend);

	      for(int x=0; x<totcount; x++)
		for(int j=0; j<nRGB*jend*iend; j++)
		  tmp1i[j]+=volX[x][j];	      

	      for(int j=0; j<nRGB*jend*iend; j++)
		tmp1[j] = tmp1i[j]/totcount;
		  
	      memcpy(m_lowresVolume + kslc*nRGB*jend*iend,
		     tmp1,
		     nRGB*jend*iend);

	      count = totcount/2;		  
	      // increment slice number
	      kslc ++;
	    }
	}

      int actualdepth = StaticFunctions::getScaledown(m_subSamplingLevel, m_depth);
      if (actualdepth < depth)
	{
	  // replicate the data
	  for (int dd=actualdepth; dd<depth; dd++)
	    memcpy(m_lowresVolume + nRGB*dd*jend*iend,
		   m_lowresVolume + nRGB*(dd-1)*jend*iend,
		   nRGB*jend*iend);
	}

      
      delete [] tmp;
      delete [] tmp1;
      delete [] tmp1i;
      if (m_subSamplingLevel > 1)
	{
	  for(int i=0; i<totcount; i++)
	    delete [] volX[i];
	  delete [] volX;
	}      
    }
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();
}

void
VolumeRGBBase::createLowresTextureVolume()
{
  //---------------
  int nRGB = 3;
  if (Global::volumeType() == Global::RGBAVolume)
    nRGB = 4;
  //---------------

  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Generating Lowres Texture Volume"));
  Global::progressBar()->show();

  int px2, py2, pz2;
  px2 = StaticFunctions::getPowerOf2(m_lowresVolumeSize.x);
  py2 = StaticFunctions::getPowerOf2(m_lowresVolumeSize.y);
  pz2 = StaticFunctions::getPowerOf2(m_lowresVolumeSize.z);

  int nsubX, nsubY, nsubZ;
  nsubX = (int)pow((float)2, (float)px2);
  nsubY = (int)pow((float)2, (float)py2);
  nsubZ = (int)pow((float)2, (float)pz2);

  m_lowresTextureVolumeSize = Vec(nsubX, nsubY, nsubZ);

  int texsize = nRGB*nsubX*nsubY*nsubZ;

  if (m_lowresTextureVolume) delete [] m_lowresTextureVolume;
  m_lowresTextureVolume = new unsigned char[texsize];
  memset(m_lowresTextureVolume, 0, texsize);

  Vec shift = (m_lowresTextureVolumeSize - m_lowresVolumeSize)/2;
  int sx,sy,sz;
  sx = shift.x;
  sy = shift.y;
  sz = shift.z;

  int vx,vy,vz;
  vx = m_lowresVolumeSize.x;
  vy = m_lowresVolumeSize.y;
  vz = m_lowresVolumeSize.z;

  int lenx = m_lowresVolumeSize.x;
  for(int k=0; k<vz; k++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)k/(float)vz));
      qApp->processEvents();

      for(int j=0; j<vy; j++)
	{
	  memcpy(m_lowresTextureVolume + nRGB*(k*nsubY*nsubX +
					       j*nsubX),
		 m_lowresVolume + nRGB*(k*vy*vx + j*vx),
		 nRGB*lenx);
	}
    }

  Global::progressBar()->setValue(100);
  Global::hideProgressBar();
}
