#include "checkpointhandler.h"
#include "blosc.h"

#include <QMessageBox>
#include <QInputDialog>
#include <QProgressDialog>
#include <QBuffer>
#include <QFile>

void
CheckpointHandler::saveCheckpoint(QString flnm,
				  int voxelType,
				  int depth, int width, int height,
				  uchar* volData,
				  QString descriptor)
{  
  int nthreads, pnthreads;
  nthreads = 4;
  blosc_init();
  // use nthreads for compression
  // previously using threads in pnthreads
  pnthreads = blosc_set_nthreads(nthreads);

  qint64 vsz = depth;
  vsz *= width;
  vsz *= height;

  
  // -----
  int mb100 = 100*1024*1024;
  uchar *vBuf = new uchar[mb100];
  int nblocks = vsz/mb100;
  if (nblocks * mb100 < vsz) nblocks++;

  // dump data to checkpoint buffer before storing to file
  QBuffer buffer;
  buffer.open(QBuffer::WriteOnly);
  buffer.write((char*)&voxelType, 1);
  buffer.write((char*)&depth, 4);
  buffer.write((char*)&width, 4);
  buffer.write((char*)&height, 4);
  buffer.write((char*)&nblocks, 4);
  buffer.write((char*)&mb100, 4);
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
			       volData+i*mb100,
			       vBuf,
			       mb100); // destination size
      if (bufsize < 0)
	{
	  QMessageBox::information(0, "", "Error in compression : .mask.sc file not saved");
	  buffer.close();
	  return;
	}


      buffer.write((char*)&bufsize, 4);
      buffer.write((char*)vBuf, bufsize);      
    }
  buffer.close();

  delete [] vBuf;
  //-----------------

  //-----------------
  // store the checkpoint buffer record to file
  QFile qfile;
  qfile.setFileName(flnm);
  qfile.open(QFile::ReadWrite);
  
  if (qfile.size() == 0)
    { // create the file
      int nrecords = 0;
      qfile.write((char*)&nrecords, 4);
      char fat[100];
      memset(fat, 0, 100);
      // write FAT - max 10000 checkpoint records per file
      // after that ask user to rename the current file
      // a new file will be created when next time checkpoint is done
      for(int i=0; i<10000; i++)
	qfile.write((char*)&fat, 100); // FAT records
      qfile.seek(0);
    }
  
  int nrecords;
  qfile.read((char*)&nrecords, 4);

  if (nrecords > 9998)
    {
      QMessageBox::information(0, "Checkpoint", QString("Number of checkpoint records in this file : %1\nPlease rename this checkpoint file to start a new checkpoint file.").arg(nrecords));
      return;
    }

  // FAT listing contains the following :
  // 8-byte record pointer, 8-byte checkpoint buffer size, 84-byte text description
  // FAT record is 100-bytes long
  
  // seek to save record entry in FAT
  // each FAT entry is 100-bytes long
  qfile.seek(4+nrecords*100);
  qint64 fpos = qfile.size();
  qint64 bufsize = buffer.size();

  // write FAT record
  qfile.write((char*)&fpos, 8);
  qfile.write((char*)&bufsize, 8);
  qfile.write((char*)descriptor.toLatin1().data(), qMin(84, descriptor.size()));


  // update the number of FAT records
  qfile.seek(0);
  nrecords++;
  qfile.write((char*)&nrecords, 4);


  // seek to end to write checkpoint data
  qfile.seek(fpos);
  qfile.write((char*)(buffer.buffer().data()), bufsize);


  qfile.close();
  // -----

  QMessageBox::information(0, "Checkpoint", QString("Saved checkpoint information to\n%1").arg(flnm));
}


bool
CheckpointHandler::loadCheckpoint(QString flnm,
				  int voxelType,
				  int depth, int width, int height,
				  uchar* volData)
{  
  // load the checkpoint buffer record from file
  QFile qfile;
  qfile.setFileName(flnm);
  if (!qfile.open(QFile::ReadOnly))
    {
      QMessageBox::information(0, "Checkpoint Error", "Cannot read checkpoint file "+flnm);
      return false;
    }
    
  int nrecords;
  qfile.read((char*)&nrecords, 4);
  QMessageBox::information(0, "", QString("Number of checkpoint records : %1").arg(nrecords));
  
  QList<qint64> rfpos;
  QList<qint64> rbufsize;
  QStringList records;

  for(int r=0; r<nrecords; r++)
    {
      qint64 fpos;
      qint64 bufsize;
      char desc[100];
      memset(desc, 0, 100);      
      qfile.read((char*)&fpos, 8);
      qfile.read((char*)&bufsize, 8);
      qfile.read((char*)&desc, 84);
      rfpos << fpos;
      rbufsize << bufsize;
      records << QString(desc);
    }

  bool ok;
  QString item = QInputDialog::getItem(0,
				       "Checkpoint Records",
				       "Select record",
				       records,
				       0,
				       false,
				       &ok);
  if (!ok)
    return false;
  
  int rid = records.indexOf(item);
  if (rid < 0)
    {
      QMessageBox::information(0, "Checkpoint Error", "Cannot find record : "+item);
      return false;
    }


  QProgressDialog progress("Restoring checkpoint "+item,
			   "Cancel",
			   0, 100,
			   0,
			   Qt::WindowStaysOnTopHint);
  progress.setMinimumDuration(0);
  progress.setCancelButton(0);
  
  qint64 fpos = rfpos[rid];
  qint64 bufsize = rbufsize[rid];

  qfile.seek(fpos);

  uchar vt;
  int dpt, wdt, ht;
  qfile.read((char*)&vt, 1);
  qfile.read((char*)&dpt, 4);
  qfile.read((char*)&wdt, 4);
  qfile.read((char*)&ht, 4);
  if (dpt != depth ||
      wdt != width ||
      ht != height)
    {
      QMessageBox::information(0, "Error",
			       QString("Cannot load checkpoint file : Grid sizes do not match - %1 %2 %3").arg(ht).arg(wdt).arg(dpt));
      qfile.close();
      return false;
    }

  progress.setValue(10);
  qApp->processEvents();

  int mb100, nblocks;
  qfile.read((char*)&nblocks, 4);
  qfile.read((char*)&mb100, 4);
  uchar *vBuf = new uchar[mb100];
  for(qint64 i=0; i<nblocks; i++)
    {
      progress.setValue(100*i/nblocks);
      qApp->processEvents();

      int vbsize;
      qfile.read((char*)&vbsize, 4);
      qfile.read((char*)vBuf, vbsize);
      int bufsize = blosc_decompress(vBuf, volData+i*mb100, mb100);
      if (bufsize < 0)
	{
	  QMessageBox::information(0, "", "Error in decompression : .mask.sc file not read");
	  qfile.close();
	  return false;
	}
    }
  qfile.close();

  delete [] vBuf;

  progress.setValue(100);
  qApp->processEvents();

  return true;
}
