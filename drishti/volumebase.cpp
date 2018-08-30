#include "global.h"
#include "volumebase.h"
#include "staticfunctions.h"
#include "volumefilemanager.h"
#include "mainwindowui.h"
#include "xmlheaderfunctions.h"
#include "volumeinformation.h"

int VolumeBase::pvlVoxelType() { return m_pvlVoxelType; }
Vec VolumeBase::getFullVolumeSize() { return m_fullVolumeSize; }
Vec VolumeBase::getLowresVolumeSize() { return m_lowresVolumeSize; }
Vec VolumeBase::getLowresTextureVolumeSize() { return m_lowresTextureVolumeSize; }
int VolumeBase::getLowresSubsamplingLevel() { return m_subSamplingLevel; }

int* VolumeBase::getLowres1dHistogram() { return m_1dHistogram; }
int* VolumeBase::getLowres2dHistogram()
{ return m_2dHistogram; }

unsigned char* VolumeBase::getLowresVolume() { return m_lowresVolume; }
unsigned char* VolumeBase::getLowresTextureVolume() { return m_lowresTextureVolume; }

VolumeBase::VolumeBase()
{
  m_pvlVoxelType = 0;
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
  m_depth = nx;
  m_width = ny;
  m_height = nz;

  m_fullVolumeSize = Vec(m_height, m_width, m_depth);

  int px2, py2, pz2;
  px2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.x);
  py2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.y);
  pz2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.z);

  m_subSamplingLevel = 1;
  while (px2+py2+pz2 > 22) // limit lowres volume size
    {
      m_subSamplingLevel *= 2;
      px2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.x/m_subSamplingLevel);
      py2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.y/m_subSamplingLevel);
      pz2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.z/m_subSamplingLevel);
    }

  int nsubX, nsubY, nsubZ;
  nsubX = (int)pow((float)2, (float)px2);
  nsubY = (int)pow((float)2, (float)py2);
  nsubZ = (int)pow((float)2, (float)pz2);

  m_lowresTextureVolumeSize = Vec(nsubX, nsubY, nsubZ);
  m_lowresVolumeSize = m_fullVolumeSize/m_subSamplingLevel;

  m_1dHistogram = new int[256];
  memset(m_1dHistogram, 0, 256*4);

  m_2dHistogram = new int[256*256];
  memset(m_2dHistogram, 0, 256*256*4);

  return true;
}

bool
VolumeBase::loadVolume(const char* volfile, bool redo)
{
  m_volumeFile = volfile;

  if (!VolumeInformation::xmlHeaderFile(volfile))
    {
      QMessageBox::information(0, "Error",
	QString("%1 is not a valid preprocessed volume file").arg(m_volumeFile));
      return false;
    }

  XmlHeaderFunctions::getDimensionsFromHeader(m_volumeFile, m_depth, m_width, m_height);
  m_fullVolumeSize = Vec(m_height, m_width, m_depth);

  m_pvlVoxelType = XmlHeaderFunctions::getPvlVoxelTypeFromHeader(m_volumeFile);
  Global::setPvlVoxelType(m_pvlVoxelType);

  createLowresVolume(redo);
  createLowresTextureVolume();
  
  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle("Drishti - Volume Exploration and Presentation Tool");

  return true;
}

void
VolumeBase::createLowresVolume(bool redo)
{
  if (Global::volumeType() == Global::DummyVolume)
    {      
      m_subSamplingLevel = 1;
      m_lowresVolumeSize = m_fullVolumeSize/m_subSamplingLevel;
      return;
    }


  int px2, py2, pz2;
  px2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.x);
  py2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.y);
  pz2 = StaticFunctions::getPowerOf2(m_fullVolumeSize.z);

  int lod = 1;

  int gtsz = StaticFunctions::getPowerOf2(Global::textureSizeLimit());
  gtsz --; // be on the lower side
  if (px2 > gtsz || py2 > gtsz || pz2 > gtsz)
    {
      int maxp2, df;
      maxp2 = qMax(px2, qMax(py2, pz2));
      df = maxp2 - gtsz;  // 2^gtsz = Global::textureSizeLimit()
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
  m_subSamplingLevel = (int)pow((float)2, (float)lod-1);

  // max hardware texture size is 512x512x512
  if ((px2 > 9 || py2 > 9 || pz2 > 9) &&
      m_subSamplingLevel == 1)
    m_subSamplingLevel = 2;


  m_lowresVolumeSize = m_fullVolumeSize/m_subSamplingLevel;
  int height= m_lowresVolumeSize.x;
  int width = m_lowresVolumeSize.y;
  int depth = m_lowresVolumeSize.z;

  int bpv = 1;
  if (m_pvlVoxelType > 0) bpv = 2;

  if (m_lowresVolume) delete [] m_lowresVolume;
  m_lowresVolume = new unsigned char[bpv*height*width*depth];
  memset(m_lowresVolume, 0, bpv*height*width*depth);

  int iend,jend,kend;      
  iend = height;
  jend = width;
  kend = depth;

  unsigned char *tmp;
  tmp = new unsigned char [bpv*jend*iend];
  memset(tmp, 0, bpv*jend*iend);
  
  //-----------
  uchar *g0, *g1, *g2;
  g0 = new unsigned char [bpv*m_width*m_height];
  g1 = new unsigned char [bpv*m_width*m_height];
  g2 = new unsigned char [bpv*m_width*m_height];
  
  if (m_1dHistogram) delete [] m_1dHistogram;
  if (m_2dHistogram) delete [] m_2dHistogram;
  
  m_1dHistogram = new int[256];
  memset(m_1dHistogram, 0, 256*4);
  
  m_2dHistogram = new int[256*256];
  memset(m_2dHistogram, 0, 256*256*4);
  
  float *flhist1D = new float[256];
  memset(flhist1D, 0, 256*4);
  float *flhist2D = new float[256*256];
  memset(flhist2D, 0, 256*256*4);
  //-----------
  
  VolumeFileManager pvlFileManager;
  int slabSize = XmlHeaderFunctions::getSlabsizeFromHeader(m_volumeFile);
  int headerSize = XmlHeaderFunctions::getPvlHeadersizeFromHeader(m_volumeFile);
  QStringList pvlnames = XmlHeaderFunctions::getPvlNamesFromHeader(m_volumeFile);
  if (pvlnames.count() > 0)
    pvlFileManager.setFilenameList(pvlnames);
  pvlFileManager.setBaseFilename(m_volumeFile);
  pvlFileManager.setVoxelType(m_pvlVoxelType);
  pvlFileManager.setDepth(m_depth);
  pvlFileManager.setWidth(m_width);
  pvlFileManager.setHeight(m_height);
  pvlFileManager.setHeaderSize(headerSize); // default is 13 bytes
  pvlFileManager.setSlabSize(slabSize);
  if (!pvlFileManager.exists())
    QMessageBox::information(0, "", "Some problem with pvl.nc files");


  MainWindowUI::mainWindowUI()->menubar->parentWidget()->\
    setWindowTitle(QString("Generating Lowres Version"));
  Global::progressBar()->show();

  int nbytes = bpv*m_width*m_height;
  for(int kslc=0; kslc<kend; kslc++)
    {
      int k = kslc*m_subSamplingLevel;

      Global::progressBar()->setValue((int)(100.0*(float)k/(float)m_depth));
      if (k%10==0) qApp->processEvents();

      uchar *vslice = pvlFileManager.getSlice(k);
      memcpy(g2, vslice, nbytes);

      if (bpv == 1) // uchar
	{
	  for(int j=0; j<m_width*m_height; j++)
	    flhist1D[g2[j]]++;
      
	  if (kslc >= 2)
	    {
	      for(int j=1; j<m_width-1; j++)
		for(int i=1; i<m_height-1; i++)
		  {
		    int gx = (g1[j*m_height+(i+1)] - g1[j*m_height+(i-1)]);
		    int gy = (g1[(j+1)*m_height+i] - g1[(j-1)*m_height+i]);
		    int gz = (g2[j*m_height+i] - g0[j*m_height+i]);
		    int gsum = sqrtf(gx*gx+gy*gy+gz*gz);
		    gsum = qBound(0, gsum, 255);
		    int v = g1[j*m_height+i];
		    flhist2D[gsum*256 + v]++;
		  }
	    }
	}
      else // ushort
	{
	  for(int j=0; j<m_width*m_height; j++)
	    flhist1D[((ushort*)g2)[j]/256]++;
      
	  for(int j=0; j<m_width*m_height; j++)
	    flhist2D[((ushort*)g2)[j]]++;

//	  if (kslc >= 2)
//	    {
//	      for(int j=1; j<m_width-1; j++)
//		for(int i=1; i<m_height-1; i++)
//		  {
//		    int v = ((ushort*)g1)[j*m_height+i];
//		    //flhist2D[v]++;
//		    int gsum = v/256;
//		    int vg = v-gsum*256;
//		    flhist2D[gsum*256 + vg]++;
//		  }
//	    }
	}
      
      uchar *gt = g0;
      g0 = g1;
      g1 = g2;
      g2 = gt;

	
      if (m_subSamplingLevel > 1)
	{
	  int ji=0;
	  if (bpv == 1)
	    {
	      for(int j=0; j<jend; j++)
		{ 
		  int y = j*m_subSamplingLevel;
		  for(int i=0; i<iend; i++) 
		    { 
		      int x = i*m_subSamplingLevel; 
		      tmp[ji] = g2[y*m_height+x];
		      ji++;
		    } 
		}
	    }
	  else
	    {
	      for(int j=0; j<jend; j++)
		{ 
		  int y = j*m_subSamplingLevel;
		  for(int i=0; i<iend; i++) 
		    { 
		      int x = i*m_subSamplingLevel; 
		      ((ushort*)tmp)[ji] = ((ushort*)g2)[y*m_height+x];
		      ji++;
		    } 
		}
	    }
	  memcpy(m_lowresVolume + bpv*kslc*jend*iend,
		 tmp,
		 bpv*jend*iend);
	}	  
      else
	memcpy(m_lowresVolume + bpv*kslc*jend*iend,
	       g2,
	       bpv*jend*iend);
    }

//  int actualdepth = StaticFunctions::getScaledown(m_subSamplingLevel, m_depth);
//  if (actualdepth < depth)
//    {
//      // replicate the data
//      for (int dd=actualdepth; dd<depth; dd++)
//	memcpy(m_lowresVolume + bpv*dd*jend*iend,
//	       m_lowresVolume + bpv*(dd-1)*jend*iend,
//	       bpv*jend*iend);
//    }

  delete [] g0;
  delete [] g1;
  delete [] g2;
  delete [] tmp;
  
  if (m_pvlVoxelType == 0)
    StaticFunctions::generateHistograms(flhist1D, flhist2D,
					m_1dHistogram, m_2dHistogram);
  else // just copy
    {
      for(int i=0; i<256; i++)
	m_1dHistogram[i] = flhist1D[i];
      for(int i=0; i<256*256; i++)
	m_2dHistogram[i] = flhist2D[i];
    }

  delete [] flhist1D;
  delete [] flhist2D;
  
  Global::progressBar()->setValue(100);
  Global::hideProgressBar();
  qApp->processEvents();
}

void
VolumeBase::createLowresTextureVolume()
{
  if (Global::volumeType() == Global::DummyVolume)
    return;

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

  int bpv = 1;
  if (m_pvlVoxelType > 0) bpv = 2;

  int texsize = bpv*nsubX*nsubY*nsubZ;

  if (m_lowresTextureVolume) delete [] m_lowresTextureVolume;
  m_lowresTextureVolume = new unsigned char[texsize];
  memset(m_lowresTextureVolume, 0, texsize);

//  Vec shift = (m_lowresTextureVolumeSize - m_lowresVolumeSize)/2;
//  int sx,sy,sz;
//  sx = shift.x;
//  sy = shift.y;
//  sz = shift.z;

  int vx,vy,vz;
  vx = m_lowresVolumeSize.x;
  vy = m_lowresVolumeSize.y;
  vz = m_lowresVolumeSize.z;

  int lenx = m_lowresVolumeSize.x;
  for(int k=0; k<vz; k++)
    {
      Global::progressBar()->setValue((int)(100.0*(float)k/(float)vz));
      if (k%10==0) qApp->processEvents();
      for(int j=0; j<vy; j++)
	{
	  memcpy(m_lowresTextureVolume + bpv*(k*nsubY*nsubX +
					      j*nsubX),
		 m_lowresVolume + bpv*(k*vy*vx + j*vx),
		 bpv*lenx);
	}
    }

  Global::progressBar()->setValue(100);
  Global::hideProgressBar();
  qApp->processEvents();
}
