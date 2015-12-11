#ifndef MYBITARRAY
#define MYBITARRAY

#include "commonqtclasses.h"

class MyBitArray
{
 public :
  MyBitArray();
  ~MyBitArray();

  void clear();
  qint64 resize(qint64);
  bool testBit(qint64);
  void setBit(qint64, bool);
  void setBit(qint64);
  void clearBit(qint64);
  void fill(bool);
  void invert();
  
  qint64 size() { return m_size; }
  qint64 count() { return m_size; }

 private :
  uchar *m_bits;
  qint64 m_size;
};

#endif
