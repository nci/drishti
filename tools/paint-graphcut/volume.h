#ifndef VOLUME_H
#define VOLUME_H

#include <QtGui>

#include "volumemask.h"
#include "volumefilemanager.h"
#include "bitmapthread.h"

class Volume : public QObject
{
  Q_OBJECT

 public :
  Volume();
  ~Volume();

  bool isValid();
  void reset();

  void setBitmapThread(BitmapThread*);

  bool setFile(QString);
  QString fileName() { return m_fileName; }

  void gridSize(int&, int&, int&);
  QImage histogramImage1D()  { return m_histogramImage1D; }
  QImage histogramImage2D()  { return m_histogramImage2D; }

  uchar* getDepthSliceImage(int);
  uchar* getWidthSliceImage(int);
  uchar* getHeightSliceImage(int);

  uchar* getMaskDepthSliceImage(int);
  uchar* getMaskWidthSliceImage(int);
  uchar* getMaskHeightSliceImage(int);

  void setMaskDepthSlice(int, uchar*);

  QList<uchar> rawValue(int, int, int);

  void createBitmask();

  void tagDSlice(int, uchar*, bool);
  void tagWSlice(int, uchar*, bool);
  void tagHSlice(int, uchar*, bool);

  void fillVolume(int, int,
		  int, int,
		  int, int,
		  QList<int>,
		  bool);

  void tagAllVisible(int, int,
		     int, int,
		     int, int);

  void dilateVolume(int, int,
		    int, int,
		    int, int);
  void dilateVolume();

  void erodeVolume();
  void erodeVolume(int, int,
		   int, int,
		   int, int);
  
 signals :
  void progressChanged(int);
  void progressReset();

 private :
  BitmapThread *thread;

  bool m_valid;

  VolumeFileManager m_pvlFileManager;
  //VolumeFileManager m_gradFileManager;

  VolumeMask m_mask;

  QString m_fileName;

  int m_depth, m_width, m_height;
  uchar *m_slice;

  int *m_1dHistogram;
  int *m_2dHistogram;
  uchar *m_histImageData1D;
  uchar *m_histImageData2D;
  QImage m_histogramImage1D;
  QImage m_histogramImage2D;


  int m_nonZeroVoxels;
  int m_bitsize;
  QBitArray m_bitmask;
  QBitArray m_connectedbitmask;


  void genHistogram();
  void generateHistogramImage();

  void findConnectedRegion(int, int,
			   int, int,
			   int, int,
			   QList<int>,
			   bool);

  void markVisibleRegion(int, int,
			 int, int,
			 int, int);

  //void createGradVolume();
  //void saveSmoothedVolume();
};

#endif
