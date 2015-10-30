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

 private :
  uchar *m_bits;
  qint64 m_size;
};

#endif
