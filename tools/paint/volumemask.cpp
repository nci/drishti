#include "volumemask.h"
#include "global.h"
#include <QDomDocument>
#include <QFileInfo>
#include <QSaveFile>

#include <limits>

VolumeMask::VolumeMask()
{
  m_maskfile.clear();
  m_maskslice = 0;
  m_depth = m_width = m_height = 0;
}

VolumeMask::~VolumeMask()
{
  (void)reset();
}

bool
VolumeMask::reset()
{
  if (!m_maskfile.isEmpty() && !m_maskFileManager.saveMemFile())
    {
      m_lastError = m_maskFileManager.lastError();
      return false;
    }

  if (!m_maskFileManager.reset())
    {
      m_lastError = m_maskFileManager.lastError();
      return false;
    }

  m_maskfile.clear();
  m_lastError.clear();
  if (m_maskslice) delete [] m_maskslice;
  m_maskslice = 0;
  m_depth = m_width = m_height = 0;
  return true;
}

bool
VolumeMask::exportMask()
{
  m_lastError.clear();
  QString maskfile = m_maskFileManager.exportMask();
  if (maskfile.isEmpty())
    {
      m_lastError = m_maskFileManager.lastError();
      return false;
    }
  if (!createPvlNc(maskfile))
    return false;

  QMessageBox::information(0, "Export",
                           QString("Exported labeled data to %1 and associated pvl.nc file")
                             .arg(maskfile));
  return true;
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

bool
VolumeMask::offloadMemFile()
{
  if (!m_maskFileManager.setMemMapped(false))
    {
      m_lastError = m_maskFileManager.lastError();
      return false;
    }
  return true;
}

bool
VolumeMask::loadRawFile(QString flnm)
{
  return m_maskFileManager.loadRawFile(flnm);
}

bool
VolumeMask::loadMemFile()
{
  if (!m_maskFileManager.setMemMapped(true))
    return false;
  if (!m_maskFileManager.loadMemFile())
    return false;
  return m_maskFileManager.startFileHandlerThread();
}

bool
VolumeMask::checkFileSave()
{
  return m_maskFileManager.checkFileSave();
}

bool
VolumeMask::exiting()
{
  return m_maskFileManager.exiting();
}

bool
VolumeMask::saveIntermediateResults(bool forceSave)
{
  if (forceSave)
    {
      m_maskFileManager.setMemChanged(true);
      return m_maskFileManager.flushPendingChanges();
    }
  return m_maskFileManager.requestSave();
}

bool
VolumeMask::saveMaskBlock(int d, int w, int h, int rad)
{
  Q_UNUSED(d);
  Q_UNUSED(w);
  Q_UNUSED(h);
  Q_UNUSED(rad);
  return m_maskFileManager.saveBlock();
}

bool
VolumeMask::saveMaskBlock(QList< QList<int> > bl)
{
  if (bl.count() == 0)
    return true;

  return m_maskFileManager.saveBlock();
}

bool
VolumeMask::setFile(QString mfile, bool inMem)
{
  if (!reset())
    return false;
  m_maskfile = mfile;
  QStringList tflnms;
  tflnms << mfile;
  m_maskFileManager.setFilenameList(tflnms);
  if (!m_maskFileManager.setMemMapped(inMem))
    {
      m_lastError = m_maskFileManager.lastError();
      return false;
    }
  return true;
}

bool
VolumeMask::setGridSize(int d, int w, int h, int slabsize)
{
  m_lastError.clear();
  Q_UNUSED(slabsize);
  if (d <= 0 || w <= 0 || h <= 0 ||
      d == std::numeric_limits<int>::max())
    return false;

  m_depth = d;
  m_width = w;
  m_height= h;  
  
  m_maskFileManager.setDepth(m_depth);
  m_maskFileManager.setWidth(m_width);
  m_maskFileManager.setHeight(m_height);
  m_maskFileManager.setHeaderSize(13);
  // do not split data across multiple files
  m_maskFileManager.setSlabSize(m_depth+1);

  bool ready = false;
  if (m_maskFileManager.exists())
    ready = m_maskFileManager.loadMemFile();
  else
    {
      const QStringList files = m_maskFileManager.filenameList();
      if (!files.isEmpty() && QFileInfo::exists(files[0]))
        return false; // Existing but invalid masks must never be overwritten.
      ready = checkMaskFile();
    }

  if (!ready)
    return false;
  if (m_maskFileManager.isMemMapped())
    return m_maskFileManager.startFileHandlerThread();
  return true;
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

bool
VolumeMask::createPvlNc(QString maskfile, QString headerBase)
{
      m_lastError.clear();
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
	tn0 = doc.createTextNode("unsigned short");
	de0.appendChild(tn0);
	topElement.appendChild(de0);
      }
      {      
	QDomElement de0 = doc.createElement("pvlvoxeltype");
	QDomText tn0;
	tn0 = doc.createTextNode("unsigned short");
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
      
      QString pvlfile = headerBase.isEmpty() ? maskfile : headerBase;
      pvlfile += ".pvl.nc";
      QSaveFile pf(pvlfile);
      if (!pf.open(QIODevice::WriteOnly))
	{
	  m_lastError = QString("create mask header: cannot open '%1': %2")
	                  .arg(pvlfile, pf.errorString());
	  return false;
	}
      QTextStream out(&pf);
      doc.save(out, 2);
      out.flush();
      if (out.status() != QTextStream::Ok)
	{
	  m_lastError = QString("create mask header: cannot write '%1': %2")
	                  .arg(pvlfile, pf.errorString());
	  pf.cancelWriting();
	  return false;
	}
      if (!pf.commit())
	{
	  m_lastError = QString("create mask header: cannot commit '%1': %2")
	                  .arg(pvlfile, pf.errorString());
	  return false;
	}
      return true;
}

bool
VolumeMask::checkMaskFile()
{
  if (m_maskFileManager.exists())
    return true;

  const QStringList files = m_maskFileManager.filenameList();
  if (!files.isEmpty() && QFileInfo::exists(files[0]))
    return false;

  if (!m_maskFileManager.createFile(true, true))
    return false;

  const QStringList createdFiles = m_maskFileManager.filenameList();
  const QString dataFile = createdFiles.isEmpty() ? m_maskfile : createdFiles[0];
  if (createPvlNc(dataFile, m_maskfile))
    {
      if (dataFile != m_maskfile && QFileInfo::exists(m_maskfile))
        QFile::remove(m_maskfile);
      return true;
    }

  m_maskFileManager.removeFile();
  return false;
} 

bool
VolumeMask::setMaskDepthSlice(int slc, uchar* tagData)
{
  return checkMaskFile() &&
         m_maskFileManager.setDepthSliceMem(slc, tagData);
}

uchar*
VolumeMask::getMaskDepthSliceImage(int slc)
{
  if (!checkMaskFile())
    return 0;
  return m_maskFileManager.getDepthSliceMem(slc);
}

uchar*
VolumeMask::getMaskWidthSliceImage(int slc)
{
  if (!checkMaskFile())
    return 0;
  return m_maskFileManager.getWidthSliceMem(slc);
}

uchar*
VolumeMask::getMaskHeightSliceImage(int slc)
{
  if (!checkMaskFile())
    return 0;
  return m_maskFileManager.getHeightSliceMem(slc);
}

ushort
VolumeMask::maskValue(int d, int w, int h)
{
  if (!checkMaskFile())
    return 0;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return 0;
  
  ushort tmp = 0;
  uchar *mslice = m_maskFileManager.rawValueMem(d, w, h);
  if (mslice)
    tmp = ((ushort*)mslice)[0];

  return tmp;
}

bool
VolumeMask::tagDSlice(int d, uchar *tags)
{
  return checkMaskFile() && m_maskFileManager.setDepthSliceMem(d, tags);
}
bool
VolumeMask::tagWSlice(int w, uchar *tags)
{
  return checkMaskFile() && m_maskFileManager.setWidthSliceMem(w, tags);
}
bool
VolumeMask::tagHSlice(int h, uchar *tags)
{
  return checkMaskFile() && m_maskFileManager.setHeightSliceMem(h, tags);
}
