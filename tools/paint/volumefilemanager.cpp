#include "global.h"
#include "volumefilemanager.h"
#include "staticfunctions.h"
#include "checkpointhandler.h"

#include <QtGui>
#include <QMessageBox>
#include <QFileDialog>
#include <QInputDialog>

#include "blosc.h"

VolumeFileManager::VolumeFileManager()
{
  m_thread = 0;
  m_handler = 0;
  m_slice = 0;
  m_block = 0;
  m_blockSlices = 10;
  m_startBlock = m_endBlock = 0;
  m_filenames.clear();
  m_volData = 0;
  m_memmapped = false;
  m_mcTimes = 0;
  m_saveFreq = 50;
  reset();  
}

VolumeFileManager::~VolumeFileManager()
{
  if (m_thread)
    {
      if (m_handler->savingFile())
	{
	  QMessageBox::information(0, "Saving Mask File", "Please wait a minute and then close this window");
	}
    }
  
//  if (m_thread)
//    {
//      m_thread->terminate();
//      m_thread->wait();
//    }  
//  reset();
}

bool
VolumeFileManager::loadCheckPoint()
{
  QString cflnm = m_filenames[0];
  if (StaticFunctions::checkExtension(cflnm, "mask.sc"))
    cflnm.chop(3);
  cflnm += ".checkpoint";

  return loadCheckPoint(cflnm);
}
bool
VolumeFileManager::loadCheckPoint(QString flnm)
{
  return CheckpointHandler::loadCheckpoint(flnm,
					   m_voxelType,
					   m_depth, m_width, m_height,
					   m_volData);
}
bool
VolumeFileManager::deleteCheckPoint()
{
  QString cflnm = m_filenames[0];
  if (StaticFunctions::checkExtension(cflnm, "mask.sc"))
    cflnm.chop(3);
  cflnm += ".checkpoint";

  return CheckpointHandler::deleteCheckpoint(cflnm,
					     m_voxelType,
					     m_depth, m_width, m_height,
					     m_volData);
}

void
VolumeFileManager::checkPoint()
{
  QString cflnm = m_filenames[0];
  if (StaticFunctions::checkExtension(cflnm, "mask.sc"))
    cflnm.chop(3);
  cflnm += ".checkpoint";
  bool ok;
  QString desc = QInputDialog::getText(0,
				       "Checkpoint",
				       "Description",
				       QLineEdit::Normal,
				       "",
				       &ok);
  desc = desc.trimmed();
  if (!ok || desc.isEmpty())
    {
      QMessageBox::information(0, "Checkpoint", "Empty description not allowed - checkpoint not saved\nPlease try again.");
      return;
    }
  
  CheckpointHandler::saveCheckpoint(cflnm,
				    m_voxelType,
				    m_depth, m_width, m_height,
				    m_volData,
				    desc);
  return;
}

void
VolumeFileManager::startFileHandlerThread()
{
  if (!m_thread)
    {
      qRegisterMetaType< IntList >( "IntList" );

      m_thread = new QThread();
      m_handler = new FileHandler();
      m_handler->moveToThread(m_thread);
      
      connect(m_handler, SIGNAL(doneFileSave()),
	      this, SLOT(doneFileSave()));
      
      connect(this, SIGNAL(saveFile()),
	      m_handler, SLOT(saveMemFile()));
      
      
      m_thread->start();
    }

  if (m_filenames.count() == 1)
    {
      if (StaticFunctions::checkExtension(m_filenames[0], ".mask"))
	{
	  QString mflnm = m_filenames[0];
	  mflnm += ".sc";
	  QStringList mflnms;
	  mflnms << mflnm;
	  m_handler->setFilenameList(mflnms);
	}
      else if (StaticFunctions::checkExtension(m_filenames[0], ".mask.sc"))
	{
	  m_handler->setFilenameList(m_filenames);
	}
    }
  //m_handler->setFilenameList(m_filenames);
  m_handler->setBaseFilename(m_baseFilename);
  m_handler->setDepth(m_depth);
  m_handler->setWidth(m_width);
  m_handler->setHeight(m_height);
  m_handler->setHeaderSize(m_header);
  m_handler->setSlabSize(m_slabSize);
  m_handler->setVoxelType(m_voxelType);
  m_handler->setVolData(m_volData);
}


void VolumeFileManager::setMemMapped(bool b)
{
  m_memmapped = b;

  if (m_volData)
    delete [] m_volData;
  m_volData = 0;

  m_memChanged = false;
  m_mcTimes = 0;
  m_saveDSlices.clear();
  m_saveWSlices.clear();
  m_saveHSlices.clear();
}

bool VolumeFileManager::isMemMapped() { return m_memmapped; }

void VolumeFileManager::setMemChanged(bool b) { m_memChanged = b; }

void
VolumeFileManager::reset()
{
  m_baseFilename.clear();
  m_filenames.clear();
  m_header = m_slabSize = 0;
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_bytesPerVoxel = 1;

  m_filename.clear();
  m_slabno = m_prevslabno = -1;

  if (m_slice)
    delete [] m_slice;
  m_slice = 0;

  if (m_block)
    delete [] m_block;
  m_block = 0;
  m_startBlock = m_endBlock = 0;

  if (m_volData)
    delete [] m_volData;
  m_volData = 0;

  if (m_qfile.isOpen())
    m_qfile.close();

  m_memmapped = false;
  m_memChanged = false;
  m_mcTimes = 0;
  m_saveDSlices.clear();
  m_saveWSlices.clear();
  m_saveHSlices.clear();

  m_fileHandlerBusy = false;
  m_waitingOnFileHandler = false;
}

int VolumeFileManager::depth() { return m_depth; }
int VolumeFileManager::width() { return m_width; }
int VolumeFileManager::height() { return m_height; }

void VolumeFileManager::setFilenameList(QStringList flist) { m_filenames = flist; }
void VolumeFileManager::setBaseFilename(QString bfn) { m_baseFilename = bfn; }
void VolumeFileManager::setDepth(int d) { m_depth = d; }
void VolumeFileManager::setWidth(int w) { m_width = w; }
void VolumeFileManager::setHeight(int h) { m_height = h; }
void VolumeFileManager::setHeaderSize(int hs) { m_header = hs; }
void VolumeFileManager::setSlabSize(int ss) { m_slabSize = ss; }
void VolumeFileManager::setVoxelType(int vt)
{
  m_voxelType = vt;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  if (m_voxelType == _Float) m_bytesPerVoxel = 4;
}

int VolumeFileManager::bytesPerVoxel() { return m_bytesPerVoxel; }

QStringList VolumeFileManager::filenameList() { return m_filenames; }
QString VolumeFileManager::baseFilename() { return m_baseFilename; }
int VolumeFileManager::headerSize() { return m_header; }
int VolumeFileManager::slabSize() { return m_slabSize; }
QString VolumeFileManager::fileName() { return m_filename; }

void
VolumeFileManager::removeFile()
{
  int nslabs = m_depth/m_slabSize;
  if (nslabs*m_slabSize < m_depth) nslabs++;
  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = m_baseFilename +
	QString(".%1").arg(ns+1, 3, 10, QChar('0'));

      QFile::remove(m_filename);
    }

  reset();
}

int VolumeFileManager::voxelType() { return m_voxelType; }

int
VolumeFileManager::readVoxelType()
{
  uchar vt = 0;
  if (m_qfile.isOpen())
    {
      m_qfile.read((char*)&vt, 1);
      m_qfile.close();
    }
  else
    {
      if (m_filenames.count() > 0)
	m_qfile.setFileName(m_filenames[0]);
      else
	m_qfile.setFileName(m_baseFilename + ".001");

      m_qfile.open(QFile::ReadWrite);	
      m_qfile.read((char*)&vt, 1);
      m_qfile.close();
    }
  return vt;
}

bool
VolumeFileManager::exists()
{
  int bps = m_width*m_height*m_bytesPerVoxel;
  int nslabs = m_depth/m_slabSize;
  if (nslabs*m_slabSize < m_depth) nslabs++;

  for(int ns=0; ns<nslabs; ns++)
    {
      if (ns < m_filenames.count())
	m_filename = m_filenames[ns];
      else
	m_filename = m_baseFilename +
	  QString(".%1").arg(ns+1, 3, 10, QChar('0'));

      int nslices = qMin(m_slabSize, m_depth-ns*m_slabSize);
      qint64 fsize = nslices;
      fsize *= bps;

      m_qfile.setFileName(m_filename);

      if (StaticFunctions::checkExtension(m_filename, ".mask.sc"))
	{
	  if (m_qfile.exists() == false)
	    return false;
	  else
	    return true;
	}
      
      if (StaticFunctions::checkExtension(m_filename, ".mask"))
	{ // check for .mask.sc instead
	  QString mflnm = m_filename;
	  mflnm += ".sc";

	  QStringList mflnms;
	  mflnms << mflnm;
	  setFilenameList(mflnms);

	  m_qfile.setFileName(mflnm);
	  if (m_qfile.exists() == false)
	    return false;
	  else
	    return true;
	}
      

      if (m_qfile.exists() == false ||
	  m_qfile.size() != m_header+fsize)
	return false;
    }

  return true;
}

void
VolumeFileManager::createFile(bool writeHeader, bool writeData)
{
  //----------------------
  if (StaticFunctions::checkExtension(m_filenames[0], ".mask.sc"))
    {
      QString mflnm = m_filenames[0];
      mflnm.chop(7);
      mflnm += "mask";
      qint64 fsize = m_depth;
      qint64 bps = m_width*m_height*m_bytesPerVoxel;
      fsize *= bps;

      m_qfile.setFileName(mflnm);

      // load .mask and save to .mask.sc file
      if (m_qfile.exists() &&
	  m_qfile.size() == m_header+fsize)
	{
	  QProgressDialog progress(QString("Loading %1").	\
				   arg(mflnm),
				   "Cancel",
				   0, 100,
				   0,
				   Qt::WindowStaysOnTopHint);
	  progress.setMinimumDuration(0);
	  progress.setCancelButton(0);
	  
	  createMemFile();      

	  qint64 bps = m_width*m_height*m_bytesPerVoxel;
	  m_qfile.open(QFile::ReadOnly);
	  m_qfile.seek((qint64)m_header);	      	      
	  for(int d=0; d<m_depth; d++)
	    {
	      m_qfile.read((char*)(m_volData + (qint64)d*bps), bps);
	      	      
	      progress.setValue((int)(100*(float)d/(float)m_depth));
	      qApp->processEvents();
	    }
	  
	  m_qfile.close(); 
	  progress.setValue(100);

	  m_handler->setVolData(m_volData);
	  m_handler->saveMemFile();

	  // converted to .mask.sc
	  // remove mask file now
	  m_qfile.remove();
	    
	  return;
	}
    }
  //----------------------
  
  // .mask file does not exist, just proceed to create .mask.sc file

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    {
      int a = qMax(m_width, qMax(m_height, m_depth));
      m_slice = new uchar[a*a*m_bytesPerVoxel];
    }

  memset(m_slice, 0, bps);

  m_slabno = m_prevslabno = -1;


  if (m_memmapped)
    {
      createMemFile();      
      m_handler->saveMemFile();
      return;
    }

  int nslabs = m_depth/m_slabSize;
  if (nslabs*m_slabSize < m_depth) nslabs++;
  
  uchar vt;
  if (m_voxelType == _UChar) vt = 0; // unsigned byte
  if (m_voxelType == _Char) vt = 1; // signed byte
  if (m_voxelType == _UShort) vt = 2; // unsigned short
  if (m_voxelType == _Short) vt = 3; // signed short
  if (m_voxelType == _Int) vt = 4; // int
  if (m_voxelType == _Float) vt = 8; // float

  QProgressDialog progress(QString("Allocating space for\n%1\non disk").\
			   arg(m_baseFilename),
			   "Cancel",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  for(int ns=0; ns<nslabs; ns++)
    {
      if (ns < m_filenames.count())
	m_filename = m_filenames[ns];
      else
	m_filename = m_baseFilename +
	  QString(".%1").arg(ns+1, 3, 10, QChar('0'));

      progress.setLabelText(m_filename);
      qApp->processEvents();

      if (m_qfile.isOpen())
	m_qfile.close();

      m_qfile.setFileName(m_filename);
      m_qfile.open(QFile::WriteOnly);

      int nslices = qMin(m_slabSize, m_depth-ns*m_slabSize);      
      if (writeHeader)
	{
	  m_qfile.write((char*)&vt, 1);
	  m_qfile.write((char*)&nslices, 4);
	  m_qfile.write((char*)&m_width, 4);
	  m_qfile.write((char*)&m_height, 4);
	  m_header = 13;
	}

      progress.setValue(10);

      if (writeData)
	{
	  for(int t=0; t<nslices; t++)
	    {
	      m_qfile.write((char*)m_slice, bps);
	      progress.setValue((int)(100*(float)t/(float)nslices));
	      qApp->processEvents();
	    }
	}
    }
  m_qfile.close();

  progress.setValue(100);
//
//  if (m_memmapped)
//    createMemFile();
}

uchar*
VolumeFileManager::getSlice(int d)
{
  QString pflnm = m_filename;

  int bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    {
      int a = qMax(m_width, qMax(m_height, m_depth));
      m_slice = new uchar[a*a*m_bytesPerVoxel];
    }

  m_slabno = d/m_slabSize;

  if (m_slabno < m_filenames.count())
    m_filename = m_filenames[m_slabno];
  else
    m_filename = m_baseFilename +
	         QString(".%1").arg(m_slabno+1, 3, 10, QChar('0'));

  if (pflnm != m_filename ||
      !m_qfile.isOpen() ||
      !m_qfile.isReadable())
    {
      if (m_qfile.isOpen()) m_qfile.close();
      m_qfile.setFileName(m_filename);

      // if we cannot open file in readwrite mode
      // then open it in readonly mode
      if (! m_qfile.open(QFile::ReadWrite))
	m_qfile.open(QFile::ReadWrite);
    }
  m_qfile.seek((qint64)(m_header + (d-m_slabno*m_slabSize)*bps));
  m_qfile.read((char*)m_slice, bps);
  m_qfile.close();

  return m_slice;
}

uchar*
VolumeFileManager::getWidthSlice(int w)
{
  int bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    {
      int a = qMax(m_width, qMax(m_height, m_depth));
      m_slice = new uchar[a*a*m_bytesPerVoxel];
    }

  int pslab = -1;
  for(int d=0; d<m_depth; d++)
    {
      int slab = d/m_slabSize;
      if (pslab != slab)
	{
	  if (pslab > -1) m_qfile.close();

	  if (slab < m_filenames.count())
	    m_filename = m_filenames[slab];
	  else
	    m_filename = m_baseFilename +
	      QString(".%1").arg(slab+1, 3, 10, QChar('0'));

	  m_qfile.setFileName(m_filename);
	  m_qfile.open(QFile::ReadWrite);
	}

      m_qfile.seek((qint64)(m_header +
			    (d-slab*m_slabSize)*bps +
			    w*m_height*m_bytesPerVoxel));
      m_qfile.read((char*)(m_slice + d*m_height*m_bytesPerVoxel),
		   m_height*m_bytesPerVoxel);
    }
  m_qfile.close();

  return m_slice;
}

uchar*
VolumeFileManager::getHeightSlice(int h)
{
  int bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    {
      int a = qMax(m_width, qMax(m_height, m_depth));
      m_slice = new uchar[a*a*m_bytesPerVoxel];
    }

  int it = 0;
  int pslab = -1;
  for(int d=0; d<m_depth; d++)
    {
      int slab = d/m_slabSize;
      if (pslab != slab)
	{
	  if (pslab > -1) m_qfile.close();

	  if (slab < m_filenames.count())
	    m_filename = m_filenames[slab];
	  else
	    m_filename = m_baseFilename +
	      QString(".%1").arg(slab+1, 3, 10, QChar('0'));

	  m_qfile.setFileName(m_filename);
	  m_qfile.open(QFile::ReadWrite);
	}

      for(int j=0; j<m_width; j++, it++)
	{
	  m_qfile.seek((qint64)(m_header +
				(d-slab*m_slabSize)*bps +
				(j*m_height + h)*m_bytesPerVoxel));
	  m_qfile.read((char*)(m_slice + it*m_bytesPerVoxel),
		       m_bytesPerVoxel);
	}
    }
  m_qfile.close();

  return m_slice;
}

void
VolumeFileManager::setSlice(int d, uchar *tmp)
{
  QString pflnm = m_filename;

  int bps = m_width*m_height*m_bytesPerVoxel;
  m_slabno = d/m_slabSize;
  if (m_slabno < m_filenames.count())
    m_filename = m_filenames[m_slabno];
  else
    m_filename = m_baseFilename +
                 QString(".%1").arg(m_slabno+1, 3, 10, QChar('0'));

  if (pflnm != m_filename ||
      !m_qfile.isOpen() ||
      !m_qfile.isWritable())
    {
      if (m_qfile.isOpen()) m_qfile.close();
      m_qfile.setFileName(m_filename);
      m_qfile.open(QFile::ReadWrite);
    }
  m_qfile.seek((qint64)(m_header + (d-m_slabno*m_slabSize)*bps));
  m_qfile.write((char*)tmp, bps);
  m_qfile.close();
}

void
VolumeFileManager::setWidthSlice(int w, uchar *tmp)
{
  int bps = m_width*m_height*m_bytesPerVoxel;

  if (m_qfile.isOpen())
    m_qfile.close();
  
  int pslab = -1;
  for(int d=0; d<m_depth; d++)
    {
      int slab = d/m_slabSize;
      if (pslab != slab)
	{
	  if (pslab > -1) m_qfile.close();

	  if (slab < m_filenames.count())
	    m_filename = m_filenames[slab];
	  else
	    m_filename = m_baseFilename +
	      QString(".%1").arg(slab+1, 3, 10, QChar('0'));

	  m_qfile.setFileName(m_filename);
	  m_qfile.open(QFile::ReadWrite);
	}

      m_qfile.seek((qint64)(m_header +
			    (d-slab*m_slabSize)*bps +
			    w*m_height*m_bytesPerVoxel));
      m_qfile.write((char*)(tmp + d*m_height*m_bytesPerVoxel),
		    m_height*m_bytesPerVoxel);
    }
  m_qfile.close();
}

void
VolumeFileManager::setHeightSlice(int h, uchar *tmp)
{
  int bps = m_width*m_height*m_bytesPerVoxel;

  if (m_qfile.isOpen())
    m_qfile.close();
  
  int it = 0;
  int pslab = -1;
  for(int d=0; d<m_depth; d++)
    {
      int slab = d/m_slabSize;
      if (pslab != slab)
	{
	  if (pslab > -1) m_qfile.close();

	  if (slab < m_filenames.count())
	    m_filename = m_filenames[slab];
	  else
	    m_filename = m_baseFilename +
	      QString(".%1").arg(slab+1, 3, 10, QChar('0'));

	  m_qfile.setFileName(m_filename);
	  m_qfile.open(QFile::ReadWrite);
	}

      for(int j=0; j<m_width; j++, it++)
	{
	  m_qfile.seek((qint64)(m_header +
				(d-slab*m_slabSize)*bps +
				(j*m_height + h)*m_bytesPerVoxel));
	  m_qfile.write((char*)(tmp + it*m_bytesPerVoxel),
			m_bytesPerVoxel);
	}
    }
  m_qfile.close();
}

uchar*
VolumeFileManager::rawValue(int d, int w, int h)
{
  int bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    {
      int a = qMax(m_width, qMax(m_height, m_depth));
      m_slice = new uchar[a*a*m_bytesPerVoxel];
    }

  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return m_slice;

  QString pflnm = m_filename;

  m_slabno = d/m_slabSize;
  if (m_slabno < m_filenames.count())
    m_filename = m_filenames[m_slabno];
  else
    m_filename = m_baseFilename +
	         QString(".%1").arg(m_slabno+1, 3, 10, QChar('0'));

  if (pflnm != m_filename ||
      !m_qfile.isOpen() ||
      !m_qfile.isReadable())
    {
      if (m_qfile.isOpen()) m_qfile.close();
      m_qfile.setFileName(m_filename);
      m_qfile.open(QFile::ReadWrite);
    }
  m_qfile.seek((qint64)(m_header +
			(d-m_slabno*m_slabSize)*bps +
			(w*m_height + h)*m_bytesPerVoxel));
  m_qfile.read((char*)m_slice, m_bytesPerVoxel);
  m_qfile.close();
  return m_slice;
}

#define interpVal(T)					\
  T *v[8];						\
  for(int i=0; i<8; i++)				\
    v[i] = (T*)(rv + i*m_bytesPerVoxel);		\
							\
  T vb = ((1-dd)*(1-ww)*(1-hh)*(*v[0]) +		\
	  (1-dd)*(1-ww)*(  hh)*(*v[1]) +		\
	  (1-dd)*(  ww)*(1-hh)*(*v[2]) +		\
	  (1-dd)*(  ww)*(  hh)*(*v[3]) +		\
	  (  dd)*(1-ww)*(1-hh)*(*v[4]) +		\
	  (  dd)*(1-ww)*(  hh)*(*v[5]) +		\
	  (  dd)*(  ww)*(1-hh)*(*v[6]) +		\
	  (  dd)*(  ww)*(  hh)*(*v[7]));		\
  memcpy(m_slice, &vb, sizeof(T));


uchar*
VolumeFileManager::interpolatedRawValue(float dv, float wv, float hv)
{
  int d = dv;
  int w = wv;
  int h = hv;
  int d1 = d+1;
  int w1 = w+1;
  int h1 = h+1;
  float dd = dv-d;
  float ww = wv-w;
  float hh = hv-h;
  
  int bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    {
      int a = qMax(m_width, qMax(m_height, m_depth));
      m_slice = new uchar[a*a*m_bytesPerVoxel];
    }

  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d1 >= m_depth ||
      w < 0 || w1 >= m_width ||
      h < 0 || h1 >= m_height)
    return m_slice;

  int da[8], wa[8], ha[8];
  da[0]=d;  wa[0]=w;  ha[0]=h;
  da[1]=d;  wa[1]=w;  ha[1]=h1;
  da[2]=d;  wa[2]=w1; ha[2]=h;
  da[3]=d;  wa[3]=w1; ha[3]=h1;
  da[4]=d1; wa[4]=w;  ha[4]=h;
  da[5]=d1; wa[5]=w;  ha[5]=h1;
  da[6]=d1; wa[6]=w1; ha[6]=h;
  da[7]=d1; wa[7]=w1; ha[7]=h1;

  uchar *rv = new uchar[8*m_bytesPerVoxel];

  int pslno = -1;
  for(int i=0; i<8; i++)
    {
      m_slabno = da[i]/m_slabSize;
      if (m_slabno != pslno)
	{
	  QString pflnm = m_filename;

	  if (m_slabno < m_filenames.count())
	    m_filename = m_filenames[m_slabno];
	  else
	    m_filename = m_baseFilename +
	      QString(".%1").arg(m_slabno+1, 3, 10, QChar('0'));

	  if (pflnm != m_filename ||
	      !m_qfile.isOpen() ||
	      !m_qfile.isReadable())
	    {
	      if (m_qfile.isOpen()) m_qfile.close();
	      m_qfile.setFileName(m_filename);
	      m_qfile.open(QFile::ReadWrite);
	    }
	}

      m_qfile.seek((qint64)(m_header +
			    (da[i]-m_slabno*m_slabSize)*bps +
			    (wa[i]*m_height + ha[i])*m_bytesPerVoxel));
      m_qfile.read((char*)(rv+i*m_bytesPerVoxel), m_bytesPerVoxel);

      pslno = m_slabno;
    }
  
  if (m_voxelType == _UChar)
    {
      interpVal(uchar);
    }
  else if (m_voxelType == _Char)
    {
      interpVal(char);
    }
  else if (m_voxelType == _UShort)
    {
      interpVal(ushort);
    }
  else if (m_voxelType == _Short)
    {
      interpVal(short);
    }
  else if (m_voxelType == _Int)
    {
      interpVal(int);
    }
  else if (m_voxelType == _Float)
    {
      interpVal(float);
    }
  
  delete [] rv;

  m_qfile.close();

  return m_slice;
}

void
VolumeFileManager::startBlockInterpolation()
{
  if (m_block)
    delete [] m_block;

  int bps = m_width*m_height*m_bytesPerVoxel;
  m_block = new uchar[m_blockSlices*bps];

  readBlocks(0);
}

void
VolumeFileManager::endBlockInterpolation()
{
  if (m_block)
    delete [] m_block;

  m_block = 0;
  m_startBlock = m_endBlock = 0;
}

void
VolumeFileManager::readBlocks(int d)
{
  int bps = m_width*m_height*m_bytesPerVoxel;

  if (!m_block)
      m_block = new uchar[m_blockSlices*bps];

  int dstart = d;
  int dend = d+m_blockSlices;

  if (m_startBlock != m_endBlock)
    {
      if (d >= m_startBlock && d < m_endBlock)
	{
	  for(int dd=d; dd<m_endBlock; dd++)
	    memcpy(m_block + (dd-d)*bps,
		   m_block + (dd-m_startBlock)*bps,
		   bps);
	  
	  dstart = m_endBlock;
	  dend = d+m_blockSlices;
	}
      else if (d < m_startBlock && dend > m_startBlock)
	{
	  for(int dd=dend-1; dd>=m_startBlock; dd--)
	    memcpy(m_block + (m_blockSlices-(dend-dd))*bps,
		   m_block + (dd-m_startBlock)*bps,
		   bps);
	  
	  dstart = d;
	  dend = m_startBlock;
	}
    }

  m_startBlock = d;
  m_endBlock = d+m_blockSlices;

  int pslno = -1;
  for(int i=dstart; i<dend; i++)
    {
      if (i>=0 && i<m_depth)
	{
	  m_slabno = i/m_slabSize;
	  if (m_slabno != pslno)
	    {
	      QString pflnm = m_filename;

	      if (m_slabno < m_filenames.count())
		m_filename = m_filenames[m_slabno];
	      else
		m_filename = m_baseFilename +
		  QString(".%1").arg(m_slabno+1, 3, 10, QChar('0'));
	      
	      if (pflnm != m_filename ||
		  !m_qfile.isOpen() ||
		  !m_qfile.isReadable())
		{
		  if (m_qfile.isOpen()) m_qfile.close();
		  m_qfile.setFileName(m_filename);
		  m_qfile.open(QFile::ReadWrite);
		}
	    }
	  
	  m_qfile.seek((qint64)(m_header + (i-m_slabno*m_slabSize)*bps));
	  m_qfile.read((char*)(m_block+(i-m_startBlock)*bps), bps);
	  
	  pslno = m_slabno;
	}
      else
	memset(m_block + (i-m_startBlock)*bps, 0, bps);
    }

  m_qfile.close();
}

uchar*
VolumeFileManager::blockInterpolatedRawValue(float dv, float wv, float hv)
{
  int d = dv;
  int w = wv;
  int h = hv;
  int d1 = d+1;
  int w1 = w+1;
  int h1 = h+1;
  float dd = dv-d;
  float ww = wv-w;
  float hh = hv-h;
  
  int bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    {
      int a = qMax(m_width, qMax(m_height, m_depth));
      m_slice = new uchar[a*a*m_bytesPerVoxel];
    }

  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d1 >= m_depth ||
      w < 0 || w1 >= m_width ||
      h < 0 || h1 >= m_height)
    return m_slice;

  int da[8], wa[8], ha[8];
  da[0]=d;  wa[0]=w;  ha[0]=h;
  da[1]=d;  wa[1]=w;  ha[1]=h1;
  da[2]=d;  wa[2]=w1; ha[2]=h;
  da[3]=d;  wa[3]=w1; ha[3]=h1;
  da[4]=d1; wa[4]=w;  ha[4]=h;
  da[5]=d1; wa[5]=w;  ha[5]=h1;
  da[6]=d1; wa[6]=w1; ha[6]=h;
  da[7]=d1; wa[7]=w1; ha[7]=h1;

  uchar *rv = new uchar[8*m_bytesPerVoxel];

  if (!m_block)
    readBlocks(da[0]);

  for(int i=0; i<8; i++)
    {
      if (da[i] < m_startBlock ||
	  da[i] >= m_endBlock)
	readBlocks(da[i]);

      memcpy((char*)rv+i*m_bytesPerVoxel,
	     m_block + (da[i]-m_startBlock)*bps +
	               (wa[i]*m_height + ha[i])*m_bytesPerVoxel,
	     m_bytesPerVoxel);      
    }
  
  if (m_voxelType == _UChar)
    {
      interpVal(uchar);
    }
  else if (m_voxelType == _Char)
    {
      interpVal(char);
    }
  else if (m_voxelType == _UShort)
    {
      interpVal(ushort);
    }
  else if (m_voxelType == _Short)
    {
      interpVal(short);
    }
  else if (m_voxelType == _Int)
    {
      interpVal(int);
    }
  else if (m_voxelType == _Float)
    {
      interpVal(float);
    }
  
  delete [] rv;

  return m_slice;
}

void
VolumeFileManager::checkFileSave()
{
  if (m_waitingOnFileHandler)
      emit saveFile();

  m_waitingOnFileHandler = false;
}

void
VolumeFileManager::doneFileSave()
{
  m_fileHandlerBusy = false;

//  if (m_waitingOnFileHandler)
//    emit saveFile();
//  else    
//    m_waitingOnFileHandler = false;
}

void
VolumeFileManager::saveSlicesToFile()
{
  if (m_thread)
    {
      if (!m_fileHandlerBusy) // fileHandler is not busy, so push the write
	{
	  emit saveFile();
	  m_mcTimes = 0;
	  m_saveDSlices.clear();

	  m_fileHandlerBusy = true;
	  m_waitingOnFileHandler = false;
	}
      else // file Handler is busy, so wait for it to finish
	m_waitingOnFileHandler = true;

      return;
    }

  


  uchar vt;
  if (m_voxelType == _UChar) vt = 0; // unsigned byte
  if (m_voxelType == _Char) vt = 1; // signed byte
  if (m_voxelType == _UShort) vt = 2; // unsigned short
  if (m_voxelType == _Short) vt = 3; // signed short
  if (m_voxelType == _Int) vt = 4; // int
  if (m_voxelType == _Float) vt = 8; // float

  QProgressDialog progress(QString("Saving %1").\
			   arg(m_baseFilename),
			   "Cancel",
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  qint64 bps = m_width*m_height*m_bytesPerVoxel;

  for(int i=0; i<m_saveDSlices.count(); i++)
    {
      progress.setValue((int)(100*(float)i/(float)m_saveDSlices.count()));
      qApp->processEvents();

      int d = m_saveDSlices[i];
      int ns = d/m_slabSize;
      
      QString pflnm = m_filename;
      m_slabno = d/m_slabSize;
      if (m_slabno < m_filenames.count())
	m_filename = m_filenames[m_slabno];
      else
	m_filename = m_baseFilename +
                     QString(".%1").arg(m_slabno+1, 3, 10, QChar('0'));

      if (pflnm != m_filename ||
	  !m_qfile.isOpen() ||
	  !m_qfile.isWritable())
	{
	  if (m_qfile.isOpen()) m_qfile.close();
	  m_qfile.setFileName(m_filename);
	  m_qfile.open(QFile::ReadWrite);
	}
      
      m_qfile.seek((qint64)(m_header + (d-m_slabno*m_slabSize)*bps));
      m_qfile.write((char*)(m_volData + (qint64)d*bps), bps);
    }

  m_qfile.close();

  progress.setValue(100);

  m_memChanged = false;
  m_mcTimes = 0;
  m_saveDSlices.clear();
}

void
VolumeFileManager::saveMemFile()
{
  if (!m_memChanged)
    return;

  //--------------------
  // for volume saved in separate thread
  if (m_thread)
    {
      if (!m_fileHandlerBusy) // fileHandler is not busy, so push the write
	{
	  m_waitingOnFileHandler = false;
	  emit saveFile();
	  m_mcTimes = 0;	  
	}
      else // file Handler is busy, so wait for it to finish
	m_waitingOnFileHandler = true;

      return;
    }
  //--------------------
  
  
  if (m_saveDSlices.count() > 0)
    {
      saveSlicesToFile();
      return;
    }


  QMessageBox::information(0,
			   "Why are we here ?",
			   "We really shouldn't be in this part of the code !!");
    
}

QString
VolumeFileManager::exportMask()
{
  QString flnm;
  flnm = QFileDialog::getSaveFileName(0,
				      "Export to .raw file",
				      Global::previousDirectory(),
				      "Mask Files (*.raw)",
				      0,
				      QFileDialog::DontUseNativeDialog);

  
  if (flnm.isEmpty())
    return QString();

  if (!StaticFunctions::checkExtension(flnm, ".raw"))
    flnm += ".raw";

  uchar vt;
  if (m_voxelType == _UChar) vt = 0; // unsigned byte
  if (m_voxelType == _Char) vt = 1; // signed byte
  if (m_voxelType == _UShort) vt = 2; // unsigned short
  if (m_voxelType == _Short) vt = 3; // signed short
  if (m_voxelType == _Int) vt = 4; // int
  if (m_voxelType == _Float) vt = 8; // float

  QProgressDialog progress(QString("Saving %1").arg(flnm),
			   "Cancel",
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  qint64 bps = m_width*m_height*m_bytesPerVoxel;

  if (m_qfile.isOpen())
    m_qfile.close();

  m_qfile.setFileName(flnm);
  m_qfile.open(QFile::WriteOnly);
  m_qfile.write((char*)&vt, 1);
  m_qfile.write((char*)&m_depth, 4);
  m_qfile.write((char*)&m_width, 4);
  m_qfile.write((char*)&m_height, 4);
  for(int d=0; d<m_depth; d++)
    {
      m_qfile.write((char*)(m_volData + (qint64)d*bps), bps);      
      progress.setValue((int)(100*(float)d/(float)m_depth));
      qApp->processEvents();
    }
  
  m_qfile.close();

  progress.setValue(100);

  return flnm;
}

void
VolumeFileManager::undo()
{
  m_handler->undo();
}

void
VolumeFileManager::loadRawFile(QString flnm)
{
  QProgressDialog progress(QString("Loading %1").\
			   arg(flnm),
			   "Cancel",
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  int d = -1;
  m_qfile.setFileName(flnm);
  m_qfile.open(QFile::ReadOnly);
  m_qfile.seek((qint64)13);
  m_qfile.read((char*)m_volData, (qint64)m_depth*(qint64)m_width*(qint64)m_height);
  m_qfile.close();

  progress.setValue(100);
}

void
VolumeFileManager::loadMemFile()
{
  if (!m_memmapped)
    return;

  
  m_memChanged = false;
  m_mcTimes = 0;
  m_saveDSlices.clear();
  m_saveWSlices.clear();
  m_saveHSlices.clear();

  
  createMemFile();

  // --------------------
  // .mask.sc file loading here
  if (m_filenames.count() > 0)
    {
      if (StaticFunctions::checkExtension(m_filenames[0], ".mask.sc"))
	{
	  m_qfile.setFileName(m_filenames[0]);
	  if (m_qfile.exists())
	    {
	      m_handler->loadMemFile();
	      return;
	    }
	}
    }
  // --------------------

  QProgressDialog progress(QString("Loading %1").\
			   arg(m_baseFilename),
			   "Cancel",
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  int d = -1;
  int nslabs = m_depth/m_slabSize;
  if (nslabs*m_slabSize < m_depth) nslabs++;
  for(int ns=0; ns<nslabs; ns++)
    {
      if (ns < m_filenames.count())
	m_filename = m_filenames[ns];
      else
	m_filename = m_baseFilename +
	  QString(".%1").arg(ns+1, 3, 10, QChar('0'));

      m_qfile.setFileName(m_filename);
      m_qfile.open(QFile::ReadOnly);
      m_qfile.seek((qint64)m_header);
      
      int slast = (m_depth-1-d);

      progress.setLabelText(QString("%1 : %2 %3").arg(m_filename).\
			    arg(d+1).arg(d+1+slast));

      for(int s=0; s<qMin(m_slabSize, (qint64)slast); s++)
	{
	  d++;
	  m_qfile.read((char*)(m_volData + (qint64)d*bps), bps);

	  
	  progress.setValue((int)(100*(float)d/(float)m_depth));
	  qApp->processEvents();
	}

      m_qfile.close(); 
    }
  progress.setValue(100);

  // --------------------
  // -- convert .mask to .mask.sc file here
  if (m_filenames.count() > 0)
    {
      if (StaticFunctions::checkExtension(m_filenames[0], ".mask"))
	{
	  QMessageBox::information(0, "", "saveMemFile - from loadMemFile");
	  m_handler->saveMemFile();
	}
    }
  // --------------------
}

void
VolumeFileManager::createMemFile()
{
  if (m_volData)
    return;
    
//  if (m_volData)
//    delete [] m_volData;

  qint64 vsize = m_width*m_height*m_bytesPerVoxel;
  vsize *= m_depth;
  m_volData = new uchar[vsize];
  memset(m_volData, 0, vsize);

  if (m_handler)
    m_handler->setVolData(m_volData);
}

void
VolumeFileManager::setDepthSliceMem(int d, uchar *tmp)
{
  if (!m_memmapped)
    {
      setSlice(d, tmp);
      return;
    }    

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  memcpy(m_volData+d*bps, tmp, bps);

  m_saveDSlices << d;
  
  m_memChanged = true;
  m_mcTimes++;
  if (m_mcTimes > m_saveFreq)
    saveMemFile();
  //saveSlicesToFile();
}
void
VolumeFileManager::setWidthSliceMem(int w, uchar *tmp)
{
  if (!m_memmapped)
    {
      setWidthSlice(w, tmp);
      return;
    }    

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  for(int d=0; d<m_depth; d++)
    memcpy(m_volData + d*bps + w*m_height*m_bytesPerVoxel,
	   tmp + d*m_height*m_bytesPerVoxel,
	   m_height*m_bytesPerVoxel);

  m_saveWSlices << w;

  m_memChanged = true;
  m_mcTimes++;
  if (m_mcTimes > m_saveFreq)
    saveMemFile();
}
void
VolumeFileManager::setHeightSliceMem(int h, uchar *tmp)
{
  if (!m_memmapped)
    {
      setHeightSlice(h, tmp);
      return;
    }    

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  
  int it = 0;
  for(int d=0; d<m_depth; d++)
    {
      for(int j=0; j<m_width; j++, it++)
	memcpy(m_volData + d*bps + (j*m_height + h)*m_bytesPerVoxel,
	       tmp + it*m_bytesPerVoxel,
	       m_bytesPerVoxel);
    }


  m_saveHSlices << h;

  m_memChanged = true;
  m_mcTimes++;
  if (m_mcTimes > m_saveFreq)
    saveMemFile();
}


uchar*
VolumeFileManager::getDepthSliceMem(int d)
{
  if (!m_memmapped)
    return getSlice(d);

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    {
      int a = qMax(m_width, qMax(m_height, m_depth));
      m_slice = new uchar[a*a*m_bytesPerVoxel];
    }

  memcpy(m_slice, m_volData+d*bps, bps);

  return m_slice;
}

uchar*
VolumeFileManager::getWidthSliceMem(int w)
{
  if (!m_memmapped)
    return getWidthSlice(w);

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    {
      int a = qMax(m_width, qMax(m_height, m_depth));
      m_slice = new uchar[a*a*m_bytesPerVoxel];
    }

  for(int d=0; d<m_depth; d++)
    memcpy(m_slice + d*m_height*m_bytesPerVoxel,
	   m_volData + d*bps + w*m_height*m_bytesPerVoxel,
	   m_height*m_bytesPerVoxel);

  return m_slice;
}

uchar*
VolumeFileManager::getHeightSliceMem(int h)
{
  if (!m_memmapped)
    return getHeightSlice(h);

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    {
      int a = qMax(m_width, qMax(m_height, m_depth));
      m_slice = new uchar[a*a*m_bytesPerVoxel];
    }

  int it = 0;
  for(int d=0; d<m_depth; d++)
    {
      for(int j=0; j<m_width; j++, it++)
	memcpy(m_slice + it*m_bytesPerVoxel,
	       m_volData + d*bps + (j*m_height + h)*m_bytesPerVoxel,
	       m_bytesPerVoxel);
    }


  
  return m_slice;
}
uchar*
VolumeFileManager::rawValueMem(int d, int w, int h)
{
  if (!m_memmapped)
    return rawValue(d,w,h);

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    {
      int a = qMax(m_width, qMax(m_height, m_depth));
      m_slice = new uchar[a*a*m_bytesPerVoxel];
    }

  // at most we will be reading an 8 byte value
  // initialize first 8 bytes to 0
  memset(m_slice, 0, 8);

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return m_slice;

  memcpy(m_slice,
	 m_volData + (d*bps + (w*m_height + h)*m_bytesPerVoxel),
	 m_bytesPerVoxel);

  return m_slice;
}

bool
VolumeFileManager::setValueMem(int d, int w, int h, int val)
{
  if (!m_memmapped)
    return false;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    return false;

  if (m_bytesPerVoxel == 1)
    m_volData[d*m_width*m_height + w*m_height + h] = val;
  else if (m_bytesPerVoxel == 2)
    ((ushort*)m_volData)[d*m_width*m_height + w*m_height + h] = val;
  
//  QMessageBox::information(0, "", QString("%1 %2 %3 : %4").\
//			   arg(d).arg(w).arg(h).arg(m_volData[d*m_width*m_height + w*m_height + h]));

  m_memChanged = true;
  m_mcTimes++;
  if (m_mcTimes > m_saveFreq)
    saveMemFile();

  return true;
}

void
VolumeFileManager::saveBlock()
{
  if (!m_memmapped)
    return;

  if (m_thread)
    {
      if (!m_fileHandlerBusy) // fileHandler is not busy, so push the write
	{
	  //m_handler->genUndo();	  
	  emit saveFile();
	  m_fileHandlerBusy = true;
	  m_waitingOnFileHandler = false;
	}
      else // file Handler is busy, so wait for it to finish
	m_waitingOnFileHandler = true;

      return;
    }
}
