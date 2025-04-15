#include "filehandler.h"

#include "blosc.h"
#include "global.h"

#include <QMainWindow>
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

void FileHandler::setFilenameList(QStringList flist)
{
  m_filenames = flist;
  m_tempFile = m_filenames[0]+".tmp";

}
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
  loadMemFile(m_filenames[0]);
}

void
FileHandler::loadMemFile(QString flnm)
{
  qint64 vsz = m_depth;
  vsz *= m_width;
  vsz *= m_height;
  int mb100, nblocks;
  char chkver[10];
  memset(chkver, 0, 10);
  
  m_qfile.setFileName(flnm);
  m_qfile.open(QFile::ReadOnly);
  m_qfile.read((char*)chkver, 6);
  uchar vt;
  int dpt, wdt, ht;
  m_qfile.read((char*)&vt, 1);
  m_qfile.read((char*)&dpt, 4);
  m_qfile.read((char*)&wdt, 4);
  m_qfile.read((char*)&ht, 4);
  if (dpt != m_depth ||
      wdt != m_width ||
      ht != m_height)
    {
      QMessageBox::information(0, "Error",
			       QString("Cannot load mask.sc file : Grid sizes do not match - %1 %2 %3").arg(ht).arg(wdt).arg(dpt));
      m_qfile.close();
      return;
    }
  m_qfile.read((char*)&nblocks, 4);
  m_qfile.read((char*)&mb100, 4);
  uchar *vBuf = new uchar[mb100];
  for(qint64 i=0; i<nblocks; i++)
    {
      int vbsize;
      m_qfile.read((char*)&vbsize, 4);
      if (vbsize < 0)
	{
	  QMessageBox::information(0, "", "Error in reading : .mask.sc file corrupted");
	  m_qfile.close();
	  return;
	}

      m_qfile.read((char*)vBuf, vbsize);
      int bufsize = blosc_decompress(vBuf, m_volData+i*mb100, mb100);
      if (bufsize < 0)
	{
	  QMessageBox::information(0, "", "Error in decompression : .mask.sc file corrupted");
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

  ((QMainWindow *)Global::mainWindow())->statusBar()->showMessage("saving ...");
    
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
  // drishti paint mask v100
  sprintf(chkver,"dpm100");

  m_qfile.setFileName(m_filenames[0]);
  m_qfile.open(QFile::ReadWrite);
  m_qfile.write((char*)chkver, 6);
  m_qfile.write((char*)&m_voxelType, 1);
  m_qfile.write((char*)&m_depth, 4);
  m_qfile.write((char*)&m_width, 4);
  m_qfile.write((char*)&m_height, 4);
  m_qfile.write((char*)&nblocks, 4);
  m_qfile.write((char*)&mb100, 4);
  for(qint64 i=0; i<nblocks; i++)
    {
      ((QMainWindow *)Global::mainWindow())->statusBar()->showMessage(QString("saving ... %1/%2").\
								      arg(i).arg(nblocks));

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

  m_qfile.flush();
  m_qfile.close();  
  // -----

  delete [] vBuf;

  m_savingFile = false;

  ((QMainWindow *)Global::mainWindow())->statusBar()->showMessage("");

  emit doneFileSave();
}

void
FileHandler::genUndo()
{
  QMutexLocker locker(&m_mutex);
  
  if (QFile::exists(m_tempFile))
    QFile::remove(m_tempFile);
      
  QFile::copy(m_filenames[0], m_tempFile);
}

void
FileHandler::undo()
{
  QMutexLocker locker(&m_mutex);
  
  if (QFile::exists(m_tempFile))
    {
      QFile::remove(m_filenames[0]);
      QFile::copy(m_tempFile, m_filenames[0]);
      loadMemFile();
    }
}

void
FileHandler::saveDataBlock()
{
  if (!m_savingFile)
    saveMemFile();
}
