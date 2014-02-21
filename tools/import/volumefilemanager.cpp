#include "volumefilemanager.h"

VolumeFileManager::VolumeFileManager()
{
  m_slice = 0;
  reset();
}

VolumeFileManager::~VolumeFileManager() { reset(); }

void
VolumeFileManager::reset()
{
  m_baseFilename.clear();
  m_header = m_slabSize = 0;
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_bytesPerVoxel = 1;

  m_filename.clear();
  m_slabno = m_prevslabno = -1;

  if (m_slice)
    delete [] m_slice;
  m_slice = 0;

  m_slice0AtTop = false;
}


void VolumeFileManager::setSliceZeroAtTop(bool fd) { m_slice0AtTop = fd; }
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

QString VolumeFileManager::fileName() { return m_filename; }

void
VolumeFileManager::removeFile()
{
  int nslabs = m_depth/m_slabSize+1;
  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = m_baseFilename +
	QString(".%1").arg(ns+1, 3, 10, QChar('0'));

      QFile::remove(m_filename);
    }

  reset();
}

bool
VolumeFileManager::exists()
{
  int bps = m_width*m_height*m_bytesPerVoxel;
  int nslabs = m_depth/m_slabSize+1;

  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = m_baseFilename +
	QString(".%1").arg(ns+1, 3, 10, QChar('0'));

      int nslices = qMin(m_slabSize, m_depth-ns*m_slabSize);      
      qint64 fsize = nslices;
      fsize *= bps;

      m_qfile.setFileName(m_filename);
      if (m_qfile.exists() == false ||
	  m_qfile.size() != m_header+fsize)
	return false;
    }

  return true;
}

void
VolumeFileManager::createFile(bool writeHeader)
{
  int bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    m_slice = new uchar[bps];
  memset(m_slice, 0, bps);

  m_slabno = m_prevslabno = -1;
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

  for(int ns=0; ns<nslabs; ns++)
    {
      m_filename = m_baseFilename +
	QString(".%1").arg(ns+1, 3, 10, QChar('0'));

      progress.setLabelText(m_filename);
      qApp->processEvents();

      m_qfile.setFileName(m_filename);
      m_qfile.open(QFile::WriteOnly);

      int nslices = qMin(m_slabSize, m_depth-ns*m_slabSize);      
      if (writeHeader)
	{
	  m_qfile.write((char*)&vt, 1);
	  m_qfile.write((char*)&nslices, 4);
	  m_qfile.write((char*)&m_width, 4);
	  m_qfile.write((char*)&m_height, 4);
	}

//      for(int t=0; t<nslices; t++)
//	{
//	  progress.setValue((int)(100*(float)t/(float)nslices));
//	  qApp->processEvents();
//	  m_qfile.write((char*)m_slice, bps);
//	}

      m_qfile.close();
    }

  progress.setValue(100);
}

uchar*
VolumeFileManager::getSlice(int ds)
{
  int d = ds;
  if (m_slice0AtTop) d = (m_depth-1)-ds;

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  if (!m_slice)
    m_slice = new uchar[bps];

  m_slabno = d/m_slabSize;
  m_filename = m_baseFilename +
	       QString(".%1").arg(m_slabno+1, 3, 10, QChar('0'));
  m_qfile.setFileName(m_filename);
  m_qfile.open(QFile::ReadOnly);
  m_qfile.seek(m_header + (d-m_slabno*m_slabSize)*bps);
  m_qfile.read((char*)m_slice, bps);
  m_qfile.close();

  return m_slice;
}

void
VolumeFileManager::setSlice(int ds, uchar *tmp)
{
  int d = ds;
  if (m_slice0AtTop) d = (m_depth-1)-ds;

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  m_slabno = d/m_slabSize;
  m_filename = m_baseFilename +
               QString(".%1").arg(m_slabno+1, 3, 10, QChar('0'));
  m_qfile.setFileName(m_filename);
  m_qfile.open(QFile::ReadWrite);
  m_qfile.seek(m_header + (d-m_slabno*m_slabSize)*bps);
  m_qfile.write((char*)tmp, bps);
  m_qfile.close();
}
