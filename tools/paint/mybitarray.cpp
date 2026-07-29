#include "mybitarray.h"
#include <QMessageBox>
#include "blosc.h"

MyBitArray::MyBitArray()
{
  m_size = 0;
  m_bits = 0;
}

MyBitArray::~MyBitArray() { clear(); }

void
MyBitArray::clear()
{
  m_size = 0;
  if (m_bits) delete [] m_bits;
  m_bits = 0;
}

qint64
MyBitArray::resize(qint64 sz)
{
  if (sz <= 0)
    { 
      clear();
      return 0;
    }
  
  m_size = sz;
  qint64 size = (1 + (m_size+7)/8); // allocate 1 extra byte
  m_bits = new uchar[size];
  fill(false);

  return size;
}

bool
MyBitArray::testBit(qint64 i)
{
  if (i < 0 || i > m_size)
    return false;

  return (*(m_bits + (i>>3)) & (1 << (i & 7))) != 0;
}

void
MyBitArray::setBit(qint64 i, bool val)
{
  if (i < 0 || i > m_size)
    {
      //QMessageBox::information(0, "", QString("Out of bounds : %1 <%2>").arg(i).arg(m_size));
      return;
    }

  if (val)
    setBit(i);
  else
    clearBit(i);
}

void
MyBitArray::setBit(qint64 i)
{
  if (i < 0 || i > m_size)
    {
      //QMessageBox::information(0, "", QString("Out of bounds : %1 <%2>").arg(i).arg(m_size));
      return;
    }

  *(m_bits + (i>>3)) |= uchar(1 << (i & 7));
}

void
MyBitArray::clearBit(qint64 i)
{
  if (i < 0 || i > m_size)
    {
      //QMessageBox::information(0, "", QString("Out of bounds : %1 <%2>").arg(i).arg(m_size));
      return;
    }

  *(m_bits + (i>>3)) &= ~uchar(1 << (i & 7));
}

void
MyBitArray::fill(bool val)
{
  qint64 size = (1 + (m_size+7)/8);
  memset(m_bits, val ? 0xff : 0, size);
}

void
MyBitArray::invert()
{
  qint64 size = (1 + (m_size+7)/8);
  for(int i=0; i<size;i++)
    m_bits[i] = ~m_bits[i];
}

MyBitArray&
MyBitArray::operator=(const MyBitArray& mba)
{
  m_size = mba.m_size;
  if (m_bits) delete [] m_bits;

  qint64 size = (1 + (m_size+7)/8); // allocate 1 extra byte
  m_bits = new uchar[size];
  memcpy(m_bits, mba.m_bits, size);

  return *this;
}

MyBitArray&
MyBitArray::operator&=(const MyBitArray& mba)
{  
  qint64 size0 = (1 + (m_size+7)/8); // allocate 1 extra byte
  qint64 size1 = (1 + (mba.m_size+7)/8); // allocate 1 extra byte

  if (size0 > size1)
    {
      for(int i=0; i<size1; i++)
	m_bits[i] &= mba.m_bits[i];
      for(int i=size1; i<size0; i++) // set rest of the bits to 0
	m_bits[i] = 0;
    }
  else
    {
      for(int i=0; i<size0; i++)
	m_bits[i] &= mba.m_bits[i];
    }

  return *this;
}

MyBitArray&
MyBitArray::operator|=(const MyBitArray& mba)
{  
  qint64 size0 = (1 + (m_size+7)/8); // allocate 1 extra byte
  qint64 size1 = (1 + (mba.m_size+7)/8); // allocate 1 extra byte

  if (size0 > size1)
    {
      for(int i=0; i<size1; i++)
	m_bits[i] |= mba.m_bits[i];
      for(int i=size1; i<size0; i++) // set rest of the bits to 0
	m_bits[i] = 0;
    }
  else
    {
      for(int i=0; i<size0; i++)
	m_bits[i] |= mba.m_bits[i];
    }

  return *this;
}

MyBitArray
operator&(const MyBitArray& a1, const MyBitArray& a2)
{  
  MyBitArray tmp = a1;
  tmp &= a2;
  return tmp;
}

MyBitArray
operator|(const MyBitArray& a1, const MyBitArray& a2)
{  
  MyBitArray tmp = a1;
  tmp |= a2;
  return tmp;
}

MyBitArray&
MyBitArray::operator~()
{  
  qint64 size0 = (1 + (m_size+7)/8); // allocate 1 extra byte
  for(int i=0; i<size0; i++)
    m_bits[i] = ~m_bits[i];

  
  return *this;
}

void
MyBitArray::save(fstream &fout)
{
  qint64 nbytes = (1 + (m_size+7)/8); // allocate 1 extra byte
  fout.write((char*)&m_size, 8);
  //fout.write((char*)m_bits, nbytes);
  
  // compress and save
  int nthreads, pnthreads;
  nthreads = 4;
  blosc_init();
  // use nthreads for compression
  // previous numofthreads returned in pnthreads
  pnthreads = blosc_set_nthreads(nthreads);
  
  int mb100 = 100*1024*1024;
  uchar *vBuf = new uchar[mb100];
  int nblocks = nbytes/mb100;
  if (nblocks * mb100 < nbytes) nblocks++;
  fout.write((char*)&nblocks, 4);
  fout.write((char*)&mb100, 4);
  for(qint64 i=0; i<nblocks; i++)
    {
      int bsz = mb100;
      if ((i+1)*mb100 > nbytes)
	bsz = nbytes-(i*mb100);
      int bufsize = blosc_compress(9, // compression level
				   BLOSC_SHUFFLE, // bit/byte-wise shuffle
				   8, // typesize
				   bsz, // input size
				   m_bits + i*mb100,
				   vBuf,
				   mb100); // destination size
      
      if (bufsize < 0)
	{
	  QMessageBox::information(0, "", "Error in compression : .roi file corrupted");
	  delete [] vBuf;
	  blosc_destroy();
	  return;
	}
      fout.write((char*)&bufsize, 4);
      fout.write((char*)vBuf, bufsize);
    }
  blosc_destroy();
  
  delete [] vBuf;
}

void
MyBitArray::load(fstream &fin)
{
  clear();
  fin.read((char*)&m_size, 8);
  qint64 nbytes = (1 + (m_size+7)/8); // allocate 1 extra byte
  m_bits = new uchar[nbytes];
  //fin.read((char*)m_bits, nbytes);

  // decompress and load
  int mb100, nblocks;
  fin.read((char*)&nblocks, 4);
  fin.read((char*)&mb100, 4);
  uchar *vBuf = new uchar[mb100];
  for(qint64 i=0; i<nblocks; i++)
    {
      int vbsize;
      fin.read((char*)&vbsize, 4);
      if (vbsize < 0)
	{
	  QMessageBox::information(0, "", "Error in decompression : .roi file may be corrupted");
	  delete [] vBuf;
	  return;
	}
      fin.read((char*)vBuf, vbsize);
      int bufsize = blosc_decompress(vBuf, m_bits+i*mb100, mb100);
      if (bufsize < 0)
	{
	  QMessageBox::information(0, "", "Error in decompression : .roi file may be corrupted");
	  delete [] vBuf;
	  return;
	}	
    }
  
  delete [] vBuf;    
}
