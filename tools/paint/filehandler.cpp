#include "filehandler.h"

#include "blosc.h"

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

  m_savingFile = false;
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
FileHandler::loadMemFile()
{
  qint64 vsz = m_depth;
  vsz *= m_width;
  vsz *= m_height;
  int mb100, nblocks;
  char chkver[10];
  memset(chkver, 0, 10);

  m_qfile.setFileName(m_filenames[0]);
  m_qfile.open(QFile::ReadOnly);
  m_qfile.read((char*)chkver, 6);
  m_qfile.read((char*)&nblocks, 4);
  m_qfile.read((char*)&mb100, 4);
  uchar *vBuf = new uchar[mb100];
  for(qint64 i=0; i<nblocks; i++)
    {
      int vbsize;
      m_qfile.read((char*)&vbsize, 4);
      m_qfile.read((char*)vBuf, vbsize);
      int bufsize = blosc_decompress(vBuf, m_volData+i*mb100, mb100);
      if (bufsize < 0)
	{
	  QMessageBox::information(0, "", "Error in decompression : .mask.sc file not read");
	  m_qfile.close();
	  return;
	}
    }
  m_qfile.close();
  delete [] vBuf;
}


void
FileHandler::saveMemFile()
{
  QMutexLocker locker(&m_mutex);
  
  m_savingFile = true;
  
  int nthreads, pnthreads;
  nthreads = 4;
  blosc_init();
  // use nthreads for compression
  // previously using threads in pnthreads
  pnthreads = blosc_set_nthreads(nthreads);

  qint64 vsz = m_depth;
  vsz *= m_width;
  vsz *= m_height;


  // -----
  int mb100 = 100*1024*1024;
  uchar *vBuf = new uchar[mb100];
  int nblocks = vsz/mb100;
  if (nblocks * mb100 < vsz) nblocks++;
  char chkver[10];
  memset(chkver,0,10);
  // drishti paint checkpoint v100
  sprintf(chkver,"dpc100");

  m_qfile.setFileName(m_filenames[0]);
  m_qfile.open(QFile::ReadWrite);
  m_qfile.write((char*)chkver, 6);
  m_qfile.write((char*)&nblocks, 4);
  m_qfile.write((char*)&mb100, 4);
  for(qint64 i=0; i<nblocks; i++)
    {
      int bsz = mb100;
      if ((i+1)*mb100 > vsz)
	bsz = vsz-(i*mb100);

      int bufsize;
      bufsize = blosc_compress(9, // compression level
			       BLOSC_SHUFFLE, // bit/byte-wise shuffle
			       8, // typesize
			       bsz, // input size
			       m_volData+i*mb100,
			       vBuf,
			       mb100); // destination size
      if (bufsize < 0)
	{
	  QMessageBox::information(0, "", "Error in compression : .mask.sc file not saved");
	  m_qfile.close();
	  return;
	}
      m_qfile.write((char*)&bufsize, 4);
      m_qfile.write((char*)vBuf, bufsize);
    }
  // to truncate the file at current position
  // file size will change based on compression achieved
  m_qfile.resize(m_qfile.pos());

  m_qfile.close();  
  // -----

  delete [] vBuf;

  m_savingFile = false;
}

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

  //-----------  
  saveMemFile();
  return;
  //-----------
  
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
