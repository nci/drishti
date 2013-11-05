#include "bitmapthread.h"

BitmapThread::BitmapThread(QObject *parent) : QThread(parent)
{
  m_interrupt = false;
  m_depth = m_height = m_width = 0;
  m_bitmask.clear();
}

BitmapThread::~BitmapThread()
{
  mutex.lock();
  m_interrupt = true;
  condition.wakeOne();
  mutex.unlock();

  wait();
}

void
BitmapThread::setFiles(QString pvlfile, QString gradfile,
		       int depth, int width, int height,
		       int slabSize)
{
  m_depth = depth;
  m_width = width;
  m_height = height;

  m_pvlFileManager.setBaseFilename(pvlfile);
  m_pvlFileManager.setDepth(depth);
  m_pvlFileManager.setWidth(width);
  m_pvlFileManager.setHeight(height);
  m_pvlFileManager.setHeaderSize(13);
  m_pvlFileManager.setSlabSize(slabSize);

//  m_gradFileManager.setBaseFilename(gradfile);
//  m_gradFileManager.setDepth(depth);
//  m_gradFileManager.setWidth(width);
//  m_gradFileManager.setHeight(height);
//  m_gradFileManager.setHeaderSize(13);
//  m_gradFileManager.setSlabSize(slabSize);
}

QBitArray BitmapThread::bitmask() { return m_bitmask; }

void
BitmapThread::createBitmask(uchar *lut)
{
  QMutexLocker locker(&mutex);
  memcpy(m_lut, lut, 4*256*256);

  if (!isRunning())
    start();
  else
    {
      m_interrupt = true;
      condition.wakeOne();
    }
  
}

void
BitmapThread::run()
{
  forever
    {
      mutex.lock();
      uchar lut[4*256*256];
      memcpy(lut, m_lut, 4*256*256);
      mutex.unlock();

      QBitArray bitmask;
      bitmask.resize(m_depth*m_width*m_height);
      bitmask.fill(false);
      
      int nbytes = m_width*m_height;
      uchar *tmp = new uchar [2*nbytes];
      
      int nonZeroVoxels = 0;
      
      int bidx = 0;
      for(int d=0; d<m_depth; d++)
	{
	  if (m_interrupt)
	    break;
	  
	  emit progressChanged((int)(100.0*(float)d/(float)m_depth));
	  
	  memset(tmp, 0, 2*nbytes);

	  uchar *vslice;
	  vslice = m_pvlFileManager.getSlice(d);
	  for(int t=0; t<nbytes; t++)
	    tmp[2*t] = vslice[t];
	  
	  //vslice = m_gradFileManager.getSlice(d);
	  //for(int t=0; t<nbytes; t++)
	  //  tmp[2*t+1] = vslice[t];
	  
	  for(int w=0; w<m_width; w++)
	    for(int h=0; h<m_height; h++)
	      {
		unsigned char v,g;
		int idx = (w*m_height + h);
		v = tmp[2*idx];
		g = tmp[2*idx+1];
		bool op = (lut[4*(256*g + v)+3] > 0);	   
		if (op)
		  {
		    bitmask.setBit(bidx);
		    
		    // count number of nonzero voxels
		    // that will give us volume
		    nonZeroVoxels++;		    
		  }
		bidx++;
	      }
	}
      
      delete [] tmp;
      
      emit progressReset();

      m_bitmask = bitmask;

      // now that we have finished our work
      // go to sleep by waiting for interrupt to become true
      // it interrupt is already true then continue with
      // the next job
      mutex.lock();
      if (!m_interrupt)
	condition.wait(&mutex);
      m_interrupt = false;
      mutex.unlock();
    }
}
