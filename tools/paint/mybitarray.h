#ifndef MYBITARRAY
#define MYBITARRAY

#include "commonqtclasses.h"

class MyBitArray
{
 public :
  MyBitArray();
  ~MyBitArray();

  MyBitArray& operator=(const MyBitArray&);

  MyBitArray& operator&=(const MyBitArray&);
  MyBitArray& operator|=(const MyBitArray&);
  MyBitArray& operator~();

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

// these operators have to be declared out of the class declaration
MyBitArray operator&(const MyBitArray&, const MyBitArray&);
MyBitArray operator|(const MyBitArray&, const MyBitArray&);

#endif
