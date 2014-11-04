#ifndef VOLUMEMASK_H
#define VOLUMEMASK_H

#include "volumefilemanager.h"

class VolumeMask : public QObject
{
  Q_OBJECT

 public :
  VolumeMask();
  ~VolumeMask();

  void reset();
  void setFile(QString);
  void setGridSize(int, int, int, int);

  void saveIntermediateResults();

  uchar* getMaskDepthSliceImage(int);
  uchar* getMaskWidthSliceImage(int);
  uchar* getMaskHeightSliceImage(int);

  void setMaskDepthSlice(int, uchar*);

  uchar maskValue(int, int, int);

  void tagDSlice(int, QBitArray, uchar*);
  void tagWSlice(int, QBitArray, uchar*);
  void tagHSlice(int, QBitArray, uchar*);

  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);

  void tagUsingBitmask(QList<int>,
		       QBitArray);

  void dilate(QBitArray);
  void dilate(int, int,
	      int, int,
	      int, int,
	      QBitArray);

  void erode(QBitArray);
  void erode(int, int,
	     int, int,
	     int, int,
	     QBitArray);

 signals :
  void progressChanged(int);
  void progressReset();

 private:
  VolumeFileManager m_maskFileManager;
  QString m_maskfile;
  int m_depth, m_width, m_height;

  uchar* m_maskslice;
  QBitArray m_bitmask;

  void checkMaskFile();
  void createBitmask();
  void dilateBitmask();
  void dilateBitmask(int, int,
		     int, int,
		     int, int);
  void erodeBitmask();
  void erodeBitmask(int, int,
		    int, int,
		    int, int);
  void findConnectedRegion(QList<int>);
};

#endif
