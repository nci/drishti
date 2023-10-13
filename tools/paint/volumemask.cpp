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
VolumeMask::exportMask()
{
  QString maskfile = m_maskFileManager.exportMask();
  if (!maskfile.isEmpty())
    createPvlNc(maskfile);
}
void
VolumeMask::checkPoint()
{
  m_maskFileManager.checkPoint();
}
bool
VolumeMask::loadCheckPoint()
{
  return m_maskFileManager.loadCheckPoint();
}
bool
VolumeMask::loadCheckPoint(QString flnm)
{
  return m_maskFileManager.loadCheckPoint(flnm);
}
bool
VolumeMask::deleteCheckPoint()
{
  return m_maskFileManager.deleteCheckPoint();
}

void
VolumeMask::offLoadMemFile()
{
  m_maskFileManager.setMemMapped(false);
}

void
VolumeMask::loadRawFile(QString flnm)
{
  m_maskFileManager.loadRawFile(flnm);
}

void
VolumeMask::loadMemFile()
{
  m_maskFileManager.loadMemFile();
}

void
VolumeMask::checkFileSave()
{
  m_maskFileManager.checkFileSave();
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
  m_maskFileManager.saveBlock();
}

void
VolumeMask::saveMaskBlock(QList< QList<int> > bl)
{
  if (bl.count() == 0)
    return;

  
  m_maskFileManager.saveBlock();
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

  m_maskFileManager.startFileHandlerThread();

  if (m_maskFileManager.exists())
    m_maskFileManager.loadMemFile();
  else
    checkMaskFile();
}


QStringList
VolumeMask::loadTagNames()
{
  QStringList tagNames;
  
  QString pvlfile = m_maskfile;
  pvlfile += ".pvl.nc";

  //QMessageBox::information(0, "", pvlfile);

  QDomDocument doc;
  QFile f(pvlfile);
  if (f.open(QIODevice::ReadOnly))
    {
      doc.setContent(&f);
      f.close();
    }

  int replace = -1;
  QDomElement topElement = doc.documentElement();
  QDomNodeList dlist = topElement.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "tagNames")
	{
	  QDomNodeList clist = dlist.at(i).childNodes();
	  for(int j=0; j<clist.count(); j++)
	    tagNames << (clist.at(j).toElement().text());

	  return tagNames;
	}
    }

  return tagNames;
}

void
VolumeMask::saveTagNames(QStringList tagNames)
{
  QString pvlfile = m_maskfile;
  pvlfile += ".pvl.nc";

  //QMessageBox::information(0, "", pvlfile);

  QDomDocument doc;
  QFile f(pvlfile);
  if (f.open(QIODevice::ReadOnly))
    {
      doc.setContent(&f);
      f.close();
    }

  int replace = -1;
  QDomElement topElement = doc.documentElement();
  QDomNodeList dlist = topElement.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "tagNames")
	{
	  replace = i;
	  break;
	}
    }

  QDomElement de = doc.createElement("tagNames");
  for(int n=0; n<tagNames.count(); n++)
    {
      QDomElement di = doc.createElement("item");
      QDomText tn = doc.createTextNode(tagNames[n]);
      di.appendChild(tn);
      de.appendChild(di);
    }
  
  
  if (replace > -1)
    topElement.replaceChild(de, dlist.at(replace));
  else
    topElement.appendChild(de);


  // save file
  QFile pf(pvlfile.toUtf8().data());
  if (pf.open(QIODevice::WriteOnly))
    {
      QTextStream out(&pf);
      doc.save(out, 2);
      pf.close();
    }      
}

void
VolumeMask::createPvlNc(QString maskfile)
{
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
	QFileInfo fileInfo(maskfile);
	QDir direc = fileInfo.absoluteDir();
	QString vstr = direc.relativeFilePath(maskfile);
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
      
      QString pvlfile = maskfile;
      pvlfile += ".pvl.nc";
      QFile pf(pvlfile.toUtf8().data());
      if (pf.open(QIODevice::WriteOnly))
	{
	  QTextStream out(&pf);
	  doc.save(out, 2);
	  pf.close();
	}      
}

void
VolumeMask::checkMaskFile()
{
  // create mask file if not present
  if (!m_maskFileManager.exists())
    {
      m_maskFileManager.createFile(true, true);

      createPvlNc(m_maskfile);
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
