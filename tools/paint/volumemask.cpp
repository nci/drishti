#include "volumemask.h"
#include "global.h"
#include <QDomDocument>

VolumeMask::VolumeMask()
{
  m_maskfile.clear();
  m_maskslice = 0;
  m_depth = m_width = m_height = 0;
}

VolumeMask::~VolumeMask()
{
  reset();
}

void
VolumeMask::reset()
{
  if (!m_maskfile.isEmpty())
    m_maskFileManager.saveMemFile();    

  m_maskFileManager.reset();

  m_maskfile.clear();
  if (m_maskslice) delete [] m_maskslice;
  m_maskslice = 0;
  m_depth = m_width = m_height = 0;
}

void
VolumeMask::checkPoint()
{
  m_maskFileManager.checkPoint();
}
void
VolumeMask::loadCheckPoint()
{
  m_maskFileManager.loadCheckPoint();
}

void
VolumeMask::offLoadMemFile()
{
  m_maskFileManager.setMemMapped(false);
}

void
VolumeMask::loadMemFile()
{
  m_maskFileManager.loadMemFile();
}

void
VolumeMask::saveIntermediateResults(bool forceSave)
{
  if (forceSave)
    m_maskFileManager.setMemChanged(true);

  m_maskFileManager.saveMemFile();
}

void
VolumeMask::saveMaskBlock(int d, int w, int h, int rad)
{
  int dmin, dmax, wmin, wmax, hmin, hmax;
  dmin = qMax(0, d-rad);
  wmin = qMax(0, w-rad);
  hmin = qMax(0, h-rad);

  dmax = qMin(m_depth-1, d+rad);
  wmax = qMin(m_width-1, w+rad);
  hmax = qMin(m_height-1, h+rad);

  m_maskFileManager.saveBlock(dmin, dmax, wmin, wmax, hmin, hmax);
}

void
VolumeMask::saveMaskBlock(QList< QList<int> > bl)
{
  if (bl.count() == 0)
    return;

  int dmin, dmax, wmin, wmax, hmin, hmax;
  dmin = wmin = hmin = 10000000;
  dmax = wmax = hmax = 0;

  if (bl[0].count() == 3)
    {
      dmin = bl[0][0];
      wmin = bl[0][1];
      hmin = bl[0][2];
      dmax = bl[1][0];
      wmax = bl[1][1];
      hmax = bl[1][2];
    }
  else
    {
      for (int i=0; i<bl.count(); i++)
	{
	  QList<int> bwhr = bl[i];
	  int d,w,h,rad;
	  if (bwhr.count() < 4)
	    {
	      QMessageBox::information(0, "Error",
				       QString("Error saving mask block list : %1").\
				       arg(bwhr.count()));
	      return;
	    }
	  d = bwhr[0];
	  w = bwhr[1];
	  h = bwhr[2];
	  rad = bwhr[3];
	  
	  dmin = qMin(dmin, d-rad);
	  wmin = qMin(wmin, w-rad);
	  hmin = qMin(hmin, h-rad);
	  
	  dmax = qMax(dmax, d+rad);
	  wmax = qMax(wmax, w+rad);
	  hmax = qMax(hmax, h+rad);
	}
    }

  dmin = qBound(0, dmin, m_depth-1);
  wmin = qBound(0, wmin, m_width-1);
  hmin = qBound(0, hmin, m_height-1);
  dmax = qBound(0, dmax, m_depth-1);
  wmax = qBound(0, wmax, m_width-1);
  hmax = qBound(0, hmax, m_height-1);

  m_maskFileManager.saveBlock(dmin, dmax, wmin, wmax, hmin, hmax);

  // flush out and close just to make sure data is stored to disk
  m_maskFileManager.saveBlock(-1,-1,-1,-1,-1,-1);
}

void
VolumeMask::setFile(QString mfile, bool inMem)
{
  reset();
  m_maskfile = mfile;
  QStringList tflnms;
  tflnms << mfile;
  m_maskFileManager.setFilenameList(tflnms);
  m_maskFileManager.setMemMapped(inMem);
}

void
VolumeMask::setGridSize(int d, int w, int h, int slabsize)
{
  m_depth = d;
  m_width = w;
  m_height= h;  
  
  m_maskFileManager.setDepth(m_depth);
  m_maskFileManager.setWidth(m_width);
  m_maskFileManager.setHeight(m_height);
  m_maskFileManager.setHeaderSize(13);
  // do not split data across multiple files
  m_maskFileManager.setSlabSize(m_depth+1);

  if (m_maskFileManager.exists())
    m_maskFileManager.loadMemFile();
  else
    checkMaskFile();

  m_maskFileManager.startFileHandlerThread();
}

void
VolumeMask::checkMaskFile()
{
  // create mask file if not present
  if (!m_maskFileManager.exists())
    {
      m_maskFileManager.createFile(true, true);

      QDomDocument doc("Drishti_Header");
      
      QDomElement topElement = doc.createElement("PvlDotNcFileHeader");
      doc.appendChild(topElement);
      
      {      
	QDomElement de0 = doc.createElement("rawfile");
	QDomText tn0;
	tn0 = doc.createTextNode("");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("pvlnames");
	QDomText tn0;
	QFileInfo fileInfo(m_maskfile);
	QDir direc = fileInfo.absoluteDir();
	QString vstr = direc.relativeFilePath(m_maskfile);
	tn0 = doc.createTextNode(vstr);
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("description");
	QDomText tn0;
	tn0 = doc.createTextNode("");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("voxeltype");
	QDomText tn0;
	tn0 = doc.createTextNode("unsigned char");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("voxelunit");
	QDomText tn0;
	tn0 = doc.createTextNode("no units");
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
	QDomElement de0 = doc.createElement("gridsize");
	QDomText tn0;
	tn0 = doc.createTextNode(QString("%1 %2 %3").arg(m_depth).arg(m_width).arg(m_height));
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("slabsize");
	QDomText tn0;
	tn0 = doc.createTextNode(QString("%1").arg(m_depth+1));
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("rawmap");
	QDomText tn0;
	tn0 = doc.createTextNode("0 255");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("pvlmap");
	QDomText tn0;
	tn0 = doc.createTextNode("0 255");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      
      QString pvlfile = m_maskfile;
      //pvlfile.chop(4);
      pvlfile += ".pvl.nc";
      QFile pf(pvlfile.toLatin1().data());
      if (pf.open(QIODevice::WriteOnly))
	{
	  QTextStream out(&pf);
	  doc.save(out, 2);
	  pf.close();
	}
      
    }
} 

void
VolumeMask::setMaskDepthSlice(int slc, uchar* tagData)
{
  checkMaskFile();
  m_maskFileManager.setDepthSliceMem(slc, tagData);
}

uchar*
VolumeMask::getMaskDepthSliceImage(int slc)
{
  checkMaskFile();

  if (m_maskslice) delete [] m_maskslice;

  int nbytes = m_width*m_height;
  m_maskslice = new uchar[nbytes];

  uchar *mslice = m_maskFileManager.getDepthSliceMem(slc);
  memcpy(m_maskslice, mslice, nbytes);

  return m_maskslice;
}

uchar*
VolumeMask::getMaskWidthSliceImage(int slc)
{
  checkMaskFile();

  if (m_maskslice) delete [] m_maskslice;

  int nbytes = m_depth*m_height;
  m_maskslice = new uchar[nbytes];

  uchar *mslice = m_maskFileManager.getWidthSliceMem(slc);
  memcpy(m_maskslice, mslice, nbytes);

  return m_maskslice;
}

uchar*
VolumeMask::getMaskHeightSliceImage(int slc)
{
  checkMaskFile();

  if (m_maskslice) delete [] m_maskslice;

  int nbytes = m_depth*m_width;
  m_maskslice = new uchar[nbytes];

  uchar *mslice = m_maskFileManager.getHeightSliceMem(slc);
  memcpy(m_maskslice, mslice, nbytes);

  return m_maskslice;
}

uchar
VolumeMask::maskValue(int d, int w, int h)
{
  checkMaskFile();

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return 0;
  
  uchar tmp = 0;
  uchar *mslice = m_maskFileManager.rawValueMem(d, w, h);
  if (mslice)
    tmp = mslice[0];

  return tmp;
}

void
VolumeMask::tagDSlice(int d, uchar *tags)
{
  checkMaskFile();
  m_maskFileManager.setDepthSliceMem(d, tags);
}
void
VolumeMask::tagWSlice(int w, uchar *tags)
{
  checkMaskFile();
  m_maskFileManager.setWidthSliceMem(w, tags);
}
void
VolumeMask::tagHSlice(int h, uchar *tags)
{
  checkMaskFile();
  m_maskFileManager.setHeightSliceMem(h, tags);
}
