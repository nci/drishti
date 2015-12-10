#include "mybitarray.h"
#include <QMessageBox>

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
      QMessageBox::information(0, "", QString("Out of bounds : %1 <%2>").arg(i).arg(m_size));
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
      QMessageBox::information(0, "", QString("Out of bounds : %1 <%2>").arg(i).arg(m_size));
      return;
    }

  *(m_bits + (i>>3)) |= uchar(1 << (i & 7));
}

void
MyBitArray::clearBit(qint64 i)
{
  if (i < 0 || i > m_size)
    {
      QMessageBox::information(0, "", QString("Out of bounds : %1 <%2>").arg(i).arg(m_size));
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
