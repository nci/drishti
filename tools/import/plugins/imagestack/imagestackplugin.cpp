#include <QtGui>
#include <QDomDocument>
#include "common.h"
#include "volumefilemanager.h"
#include "imagestackplugin.h"

void ImageStackPlugin::generateHistogram() {} // to satisfy the interface

QStringList
ImageStackPlugin::registerPlugin()
{
  QStringList regString;
  regString << "directory";
  regString << "Standard Image Directory";
  regString << "files";
  regString << "Standard Image Files";
  
  return regString;
}

void
ImageStackPlugin::init()
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
ImageStackPlugin::clear()
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
ImageStackPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
ImageStackPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString ImageStackPlugin::description() { return m_description; }
int ImageStackPlugin::voxelType() { return m_voxelType; }
int ImageStackPlugin::voxelUnit() { return m_voxelUnit; }
int ImageStackPlugin::headerBytes() { return m_headerBytes; }

void
ImageStackPlugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
}
float ImageStackPlugin::rawMin() { return m_rawMin; }
float ImageStackPlugin::rawMax() { return m_rawMax; }
QList<uint> ImageStackPlugin::histogram() { return m_histogram; }

void
ImageStackPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
ImageStackPlugin::replaceFile(QString flnm)
{
  m_fileName.clear();
  m_fileName << flnm;
}

void
ImageStackPlugin::setImageFiles(QStringList files)
{
  m_imageList = files;

  m_depth = m_imageList.size();
  QImage img = QImage(m_imageList[0]);
  m_height = img.width();
  m_width = img.height();
  m_headerBytes = 0;

  //--------------
  QStringList dtypes;
  dtypes << "Grayscale Images"
	 << "RGB Images"
	 << "RGBA Images";
  QString option = QInputDialog::getItem(0,
					 "Select Image Type",
					 "Image Type",
					 dtypes,
					 0,
					 false);
  m_voxelType = _UChar;
  m_bytesPerVoxel = 1;

  if (option == "RGB Images")
    {
      m_voxelType = _Rgb;
      m_bytesPerVoxel = 3;
    }
  else if (option == "RGBA Images")
    {
      m_voxelType = _Rgba;
      m_bytesPerVoxel = 4;
    }
  //--------------


  m_histogram.clear();
  for(int i=0; i<256; i++)
    m_histogram.append(0);
  
  m_rawMin = 0;
  m_rawMax = 255;
}

bool
ImageStackPlugin::setFile(QStringList files)
{
  if (files.size() == 0)
    return false;

  m_fileName = files;

  QFileInfo f(m_fileName[0]);
  if (f.isDir())
    {
      // list all image files in the directory
      QStringList imageNameFilter;
      imageNameFilter << "*.bmp";
      imageNameFilter << "*.gif";
      imageNameFilter << "*.jpg";
      imageNameFilter << "*.jpeg";
      imageNameFilter << "*.png";
      imageNameFilter << "*.pbm";
      imageNameFilter << "*.pgm";
      imageNameFilter << "*.ppm";
      imageNameFilter << "*.tif";
      imageNameFilter << "*.tiff";
      imageNameFilter << "*.xbm";
      imageNameFilter << "*.xpm";
      QStringList imgfiles= QDir(m_fileName[0]).entryList(imageNameFilter,
							  QDir::NoSymLinks|
							  QDir::NoDotAndDotDot|
							  QDir::Readable|
							  QDir::Files);
      QStringList imageList;
      for(uint i=0; i<imgfiles.size(); i++)
	{
	  QFileInfo fileInfo(m_fileName[0], imgfiles[i]);
	  QString imgfl = fileInfo.absoluteFilePath();
	  imageList.append(imgfl);
	}      
      if (imageList.size() == 0)
	return false;

      setImageFiles(imageList);
    }
  else
    setImageFiles(files);

  return true;
}

void
ImageStackPlugin::getDepthSlice(int slc,
				uchar *slice)
{
  QImage imgL = QImage(m_imageList[slc]);
  if (imgL.format() != QImage::Format_ARGB32)
    imgL = imgL.convertToFormat(QImage::Format_ARGB32);

  uchar *imgbits = imgL.bits();
  if (m_voxelType == _UChar)
    {
      for(uint j=0; j<m_width*m_height; j++)
	slice[j] = imgbits[4*j];
    }
  else
    {
      for(uint j=0; j<m_width*m_height; j++)
	{
	  slice[4*j+0] = imgbits[4*j+0];
	  slice[4*j+1] = imgbits[4*j+1];
	  slice[4*j+2] = imgbits[4*j+2];
	  slice[4*j+3] = imgbits[4*j+3];
	}
    }

}

void
ImageStackPlugin::getWidthSlice(int slc,
				uchar *slice)
{
  for(uint i=0; i<m_depth; i++)
    {
      QImage imgL = QImage(m_imageList[i]);
      if (imgL.format() != QImage::Format_ARGB32)
	imgL = imgL.convertToFormat(QImage::Format_ARGB32);

      uchar *imgbits = imgL.bits();
      
      if (m_voxelType == _UChar)
	{
	  for(uint j=0; j<m_height; j++)
	    slice[i*m_height+j] = imgbits[4*(slc*m_height+j)];
	}      
      else
	{
	  for(uint j=0; j<m_height; j++)
	    {
	      slice[4*(i*m_height+j)+0] = imgbits[4*(slc*m_height+j)+0];
	      slice[4*(i*m_height+j)+1] = imgbits[4*(slc*m_height+j)+1];
	      slice[4*(i*m_height+j)+2] = imgbits[4*(slc*m_height+j)+2];
	      slice[4*(i*m_height+j)+3] = imgbits[4*(slc*m_height+j)+3];
	    }
	}
    }
}

void
ImageStackPlugin::getHeightSlice(int slc,
				 uchar *slice)
{
  for(uint i=0; i<m_depth; i++)
    {
      QImage imgL = QImage(m_imageList[i]);
      if (imgL.format() != QImage::Format_ARGB32)
	imgL = imgL.convertToFormat(QImage::Format_ARGB32);

      uchar *imgbits = imgL.bits();
      if (m_voxelType == _UChar)
	{
	  for(uint j=0; j<m_width; j++)
	    slice[i*m_width+j] = imgbits[4*(j*m_height+slc)];
	}
      else
	{
	  for(uint j=0; j<m_width; j++)
	    {
	      slice[4*(i*m_width+j)+0] = imgbits[4*(j*m_height+slc)+0];
	      slice[4*(i*m_width+j)+1] = imgbits[4*(j*m_height+slc)+1];
	      slice[4*(i*m_width+j)+2] = imgbits[4*(j*m_height+slc)+2];
	      slice[4*(i*m_width+j)+3] = imgbits[4*(j*m_height+slc)+3];
	    }
	}
    }
}

QVariant
ImageStackPlugin::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }

  QImage imgL = QImage(m_imageList[d]);
  if (imgL.format() != QImage::Format_RGB32)
    imgL = imgL.convertToFormat(QImage::Format_RGB32);

  uchar *imgbits = imgL.bits();

  if (m_voxelType == _Rgb || m_voxelType == _Rgba)
    {
      uchar r = imgbits[4*(w*m_height+h)+0];
      uchar g = imgbits[4*(w*m_height+h)+1];
      uchar b = imgbits[4*(w*m_height+h)+2];
      uchar a = imgbits[4*(w*m_height+h)+3];
      
      v = QVariant(QString(" (%1 %2 %3 %4)").\
			    arg(r).arg(g).arg(b).arg(a));
      return v;
    }

  uint val = imgbits[4*(w*m_height+h)];
  v = QVariant((uint)val);

  return v;
}

void
ImageStackPlugin::saveTrimmed(QString trimFile,
			      int dmin, int dmax,
			      int wmin, int wmax,
			      int hmin, int hmax)
{
  if (m_voxelType == _Rgb || m_voxelType == _Rgba)
    {
      saveTrimmedRGB(trimFile,
		     dmin, dmax,
		     wmin, wmax,
		     hmin, hmax);
      return;
    }

  QProgressDialog progress("Saving trimmed volume",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int mX, mY, mZ;
  mX = dmax-dmin+1;
  mY = wmax-wmin+1;
  mZ = hmax-hmin+1;

  int bpv = m_bytesPerVoxel;
  int nbytes = nY*nZ*bpv;
  uchar *tmp = new uchar[nbytes];

  uchar vt = 0;

  QFile fout(trimFile);
  fout.open(QFile::WriteOnly);

  fout.write((char*)&vt, 1);
  fout.write((char*)&mX, 4);
  fout.write((char*)&mY, 4);
  fout.write((char*)&mZ, 4);

  //for(uint i=dmin; i<=dmax; i++)
  for(int i=dmax; i>=dmin; i--)
    {

      QImage imgL = QImage(m_imageList[i]);
      if (imgL.format() != QImage::Format_ARGB32)
	imgL = imgL.convertToFormat(QImage::Format_ARGB32);
      
      uchar *imgbits = imgL.bits();
      for(uint j=0; j<m_width*m_height; j++)
	tmp[j] = imgbits[4*j];

      for(uint j=wmin; j<=wmax; j++)
	{
	  memcpy(tmp+(j-wmin)*mZ*bpv,
		 tmp+(j*nZ + hmin)*bpv,
		 mZ*bpv);
	}
      
      fout.write((char*)tmp, mY*mZ*bpv);
      
      progress.setValue((int)(100*(float)(i-dmin)/(float)mX));
      qApp->processEvents();
    }

  fout.close();

  delete [] tmp;

  m_headerBytes = 13; // to be used for applyMapping function
}

void
ImageStackPlugin::savePvlHeader(QString pvlFilename,
				int d, int w, int h,
				QString voxelType,
				int slabSize)
{
  QString xmlfile = pvlFilename;

  QDomDocument doc("Drishti_Header");

  QDomElement topElement = doc.createElement("PvlDotNcFileHeader");
  doc.appendChild(topElement);

  {      
    QDomElement de0 = doc.createElement("gridsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1 %2 %3").arg(d).arg(w).arg(h));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QString vstr = "no units";    
    QDomElement de0 = doc.createElement("voxelunit");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(vstr));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QString vstr = voxelType;    
    QDomElement de0 = doc.createElement("voxeltype");
    QDomText tn0;
    tn0 = doc.createTextNode(voxelType);
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QDomElement de0 = doc.createElement("voxelsize");
    QDomText tn0;
    tn0 = doc.createTextNode("1 1 1");
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }
  
  {
    QDomElement de0 = doc.createElement("description");
    QDomText tn0;
    tn0 = doc.createTextNode("Colour volume");
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }

  {      
    QDomElement de0 = doc.createElement("slabsize");
    QDomText tn0;
    tn0 = doc.createTextNode(QString("%1").arg(slabSize));
    de0.appendChild(tn0);
    topElement.appendChild(de0);
  }  
  
  QFile f(xmlfile.toAscii().data());
  if (f.open(QIODevice::WriteOnly))
    {
      QTextStream out(&f);
      doc.save(out, 2);
      f.close();
    }
}

void
ImageStackPlugin::saveTrimmedRGB(QString trimFile,
				 int dmin, int dmax,
				 int wmin, int wmax,
				 int hmin, int hmax)
{
  QStringList dtypes;
  dtypes << "No"
	 << "Yes";

  QString option = QInputDialog::getItem(0,
					 "Save Alpha Channel",
					 "Alpha Channel",
					 dtypes,
					 0,
					 false);
  
  bool saveAlpha = false;
  if (option == "Yes")
    saveAlpha = true;


  QProgressDialog progress("Saving trimmed RGB volume",
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  int nX, nY, nZ;
  nX = m_depth;
  nY = m_width;
  nZ = m_height;

  int d, w, h;
  d = dmax-dmin+1;
  w = wmax-wmin+1;
  h = hmax-hmin+1;

  int nbytes = nY*nZ;
  uchar *tmpR = new uchar[nbytes];
  uchar *tmpG = new uchar[nbytes];
  uchar *tmpB = new uchar[nbytes];
  uchar *tmpA;
  if (saveAlpha)
    tmpA = new uchar[nbytes];


  QString voxelType = "RGB";
  if (saveAlpha) voxelType = "RGBA";
  
  //*** max 1Gb per slab
  int slabSize;
  slabSize = (1024*1024*1024)/(w*h);

  savePvlHeader(trimFile,
		d, w, h,
		voxelType,
		slabSize);			       

  VolumeFileManager rFileManager;
  VolumeFileManager gFileManager;
  VolumeFileManager bFileManager;
  VolumeFileManager aFileManager;

  QString pvlfile = trimFile;
  pvlfile.chop(6);

  QString rFilename = pvlfile + QString("red");
  QString gFilename = pvlfile + QString("green");
  QString bFilename = pvlfile + QString("blue");
  QString aFilename = pvlfile + QString("alpha");

  rFileManager.setBaseFilename(rFilename);
  rFileManager.setDepth(d);
  rFileManager.setWidth(w);
  rFileManager.setHeight(h);
  rFileManager.setVoxelType(0);
  rFileManager.setHeaderSize(13);
  rFileManager.setSlabSize(slabSize);
  rFileManager.createFile(true);
  rFileManager.createFile(true);

  gFileManager.setBaseFilename(gFilename);
  gFileManager.setDepth(d);
  gFileManager.setWidth(w);
  gFileManager.setHeight(h);
  gFileManager.setVoxelType(0);
  gFileManager.setHeaderSize(13);
  gFileManager.setSlabSize(slabSize);
  gFileManager.createFile(true);
  gFileManager.createFile(true);

  bFileManager.setBaseFilename(bFilename);
  bFileManager.setDepth(d);
  bFileManager.setWidth(w);
  bFileManager.setHeight(h);
  bFileManager.setVoxelType(0);
  bFileManager.setHeaderSize(13);
  bFileManager.setSlabSize(slabSize);
  bFileManager.createFile(true);
  bFileManager.createFile(true);

  if (saveAlpha)
    {
      aFileManager.setBaseFilename(aFilename);
      aFileManager.setDepth(d);
      aFileManager.setWidth(w);
      aFileManager.setHeight(h);
      aFileManager.setVoxelType(0);
      aFileManager.setHeaderSize(13);
      aFileManager.setSlabSize(slabSize);
      aFileManager.createFile(true);
      aFileManager.createFile(true);
    }


  for(int i=dmax; i>=dmin; i--)
    {

      QImage imgL = QImage(m_imageList[i]);
      if (imgL.format() != QImage::Format_ARGB32)
	imgL = imgL.convertToFormat(QImage::Format_ARGB32);
      
      uchar *imgbits = imgL.bits();
      for(uint j=0; j<m_width*m_height; j++)
	{
	  tmpR[j] = imgbits[4*j+2];
	  tmpG[j] = imgbits[4*j+1];
	  tmpB[j] = imgbits[4*j+0];
	}

      if (saveAlpha)
	{
	  for(uint j=0; j<m_width*m_height; j++)
	    tmpA[j] = imgbits[4*j+3];
	}

      for(uint j=wmin; j<=wmax; j++)
	memcpy(tmpR+(j-wmin)*h,
	       tmpR+(j*nZ + hmin),
	       h);

      for(uint j=wmin; j<=wmax; j++)
	memcpy(tmpG+(j-wmin)*h,
	       tmpG+(j*nZ + hmin),
	       h);

      for(uint j=wmin; j<=wmax; j++)
	memcpy(tmpB+(j-wmin)*h,
	       tmpB+(j*nZ + hmin),
	       h);

      if (saveAlpha)
	{
	  for(uint j=wmin; j<=wmax; j++)
	    memcpy(tmpA+(j-wmin)*h,
		   tmpA+(j*nZ + hmin),
		   h);
	}

      rFileManager.setSlice(dmax-i, tmpR);
      gFileManager.setSlice(dmax-i, tmpG);
      bFileManager.setSlice(dmax-i, tmpB);
      if (saveAlpha)
	aFileManager.setSlice(dmax-i, tmpA);

      progress.setValue((int)(100*(float)(dmax-i)/(float)d));
      qApp->processEvents();
    }


  delete [] tmpR;
  delete [] tmpG;
  delete [] tmpB;

  if (saveAlpha)
    delete [] tmpA;

  progress.setValue(100);
}

//-------------------------------
//-------------------------------
Q_EXPORT_PLUGIN2(imagestackplugin, ImageStackPlugin);
