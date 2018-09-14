#include "filehandler.h"

#include <QMessageBox>


FileHandler::FileHandler()
{
  reset();
}
FileHandler::~FileHandler()
{
  reset();
}

void
FileHandler::reset()
{
  m_filename.clear();
  m_baseFilename.clear();
  m_filenames.clear();
  m_header = m_slabSize = 0;
  m_depth = m_width = m_height = 0;
  m_voxelType = 0;
  m_bytesPerVoxel = 1;
  m_volData = 0;
}

void FileHandler::setFilenameList(QStringList flist) { m_filenames = flist; }
void FileHandler::setBaseFilename(QString bfn) { m_baseFilename = bfn; }
void FileHandler::setDepth(int d) { m_depth = d; }
void FileHandler::setWidth(int w) { m_width = w; }
void FileHandler::setHeight(int h) { m_height = h; }
void FileHandler::setHeaderSize(int hs) { m_header = hs; }
void FileHandler::setSlabSize(int ss) { m_slabSize = ss; }
void FileHandler::setVoxelType(int vt)
{
  m_voxelType = vt;
  if (m_voxelType == 0) m_bytesPerVoxel = 1;
  if (m_voxelType == 1) m_bytesPerVoxel = 1;
  if (m_voxelType == 2) m_bytesPerVoxel = 2;
  if (m_voxelType == 3) m_bytesPerVoxel = 2;
  if (m_voxelType == 4) m_bytesPerVoxel = 4;
  if (m_voxelType == 5) m_bytesPerVoxel = 4;
}
void FileHandler::setVolData(uchar *v) { m_volData = v; }

void
FileHandler::saveDataBlock(int dmin, int dmax,
			   int wmin, int wmax,
			   int hmin, int hmax)
{
  if (dmin == -1 || wmin == -1 || hmin == -1 ||
      dmax == -1 || wmax == -1 || hmax == -1)
    {
      //if (m_qfile.isOpen()) m_qfile.close();
      return;
    }

  QMutexLocker locker(&m_mutex);
  
  dmin = qMax(0, dmin);
  wmin = qMax(0, wmin);
  hmin = qMax(0, hmin);

  dmax = qMin(m_depth-1, dmax);
  wmax = qMin(m_width-1, wmax);
  hmax = qMin(m_height-1, hmax);

  int hbts = (hmax-hmin+1)*m_bytesPerVoxel;

  qint64 bps = m_width*m_height*m_bytesPerVoxel;
  QString pflnm = m_filename;

  for(int d=dmin; d<=dmax; d++)
    {
      int slabno = d/m_slabSize;
      if (slabno < m_filenames.count())
	m_filename = m_filenames[slabno];
      else
	m_filename = m_baseFilename +
	  QString(".%1").arg(slabno+1, 3, 10, QChar('0'));
      
      if (pflnm != m_filename ||
	  !m_qfile.isOpen() ||
	  !m_qfile.isWritable())
	{
	  if (m_qfile.isOpen()) m_qfile.close();
	  m_qfile.setFileName(m_filename);
	  m_qfile.open(QFile::ReadWrite);
	}
      for(int w=wmin; w<=wmax; w++)
	{
	  m_qfile.seek((qint64)(m_header +
				(d-slabno*m_slabSize)*bps +
				(w*m_height + hmin)*m_bytesPerVoxel));
	  m_qfile.write((char*)(m_volData + d*bps +
				(w*m_height + hmin)*m_bytesPerVoxel),
			hbts);
	}      
    }

  m_qfile.close();
}

void
FileHandler::saveDepthSlices(QList<int> slices)
{
  int dmin = slices[0];
  int dmax = slices[0];
  for(int i=1; i<slices.count(); i++)
    {
      if (slices[i] == dmax+1)
	dmax++;
      else if (slices[i] == dmin-1)
	dmin--;
      else
	{
	  saveDataBlock(dmin, dmax,
			0, m_width-1,
			0, m_height-1);
	  dmin = slices[i];
	  dmax = slices[i];	  
	}
    }
  saveDataBlock(dmin, dmax,
		0, m_width-1,
		0, m_height-1);  
}

void
FileHandler::saveWidthSlices(QList<int> slices)
{
  int wmin = slices[0];
  int wmax = slices[0];
  for(int i=1; i<slices.count(); i++)
    {
      if (slices[i] == wmax+1)
	wmax++;
      else if (slices[i] == wmin-1)
	wmin--;
      else
	{
	  saveDataBlock(0, m_depth-1,
			wmin, wmax,
			0, m_height-1);
	  wmin = slices[i];
	  wmax = slices[i];	  
	}
    }
  saveDataBlock(0, m_depth-1,
		wmin, wmax,
		0, m_height-1);  
}

void
FileHandler::saveHeightSlices(QList<int> slices)
{
  int hmin = slices[0];
  int hmax = slices[0];
  for(int i=1; i<slices.count(); i++)
    {
      if (slices[i] == hmax+1)
	hmax++;
      else if (slices[i] == hmin-1)
	hmin--;
      else
	{
	  saveDataBlock(0, m_depth-1,
			0, m_width-1,
			hmin, hmax);
	  hmin = slices[i];
	  hmax = slices[i];	  
	}
    }
  saveDataBlock(0, m_depth-1,
		0, m_width-1,
		hmin, hmax);
}
