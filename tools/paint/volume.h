#ifndef VOLUME_H
#define VOLUME_H

#include "volumemask.h"
#include "volumefilemanager.h"

class Volume : public QObject
{
  Q_OBJECT

 public :
  Volume();
  ~Volume();

  bool isValid();
  void reset();

  void undo() { m_mask.undo(); }

  void reloadMask() { m_mask.loadMemFile(); }

  bool setFile(QString);
  QString fileName() { return m_fileName; }

  void exportMask();
  void checkPoint();
  bool loadCheckPoint();
  bool loadCheckPoint(QString);
  bool deleteCheckPoint();

  void offLoadMemFile();
  void loadMemFile();

  void setSaveFrequency(int t) { m_mask.setSaveFrequency(t); }

  void gridSize(int&, int&, int&);
  QImage histogramImage1D()  { return m_histogramImage1D; }
  QImage histogramImage2D()  { return m_histogramImage2D; }
  int* histogram2D() { return m_2dHistogram; }
  int* histogram1D() { return m_1dHistogram; }

  uchar* getDepthSliceImage(int);
  uchar* getWidthSliceImage(int);
  uchar* getHeightSliceImage(int);

  uchar* getMaskDepthSliceImage(int);
  uchar* getMaskWidthSliceImage(int);
  uchar* getMaskHeightSliceImage(int);

  void setMaskDepthSlice(int, uchar*);

  QList<int> rawValue(int, int, int);

  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);
  
  uchar* memVolDataPtr() {return m_pvlFileManager.memVolDataPtr();};
  uchar* memMaskDataPtr() {return m_mask.memMaskDataPtr();};

  void saveIntermediateResults(bool forceSave=false);
  
  void saveMaskBlock(int, int, int, int);
  void saveMaskBlock(QList< QList<int> >);

  void genHistogram(bool);
  void generateHistogramImage();

  void saveModifiedOriginalVolume();

  void findStartEndForTag(int,
			  int&, int&,
			  int&, int&,
			  int&, int&);

 signals :
  void progressChanged(int);
  void progressReset();

 private :
  bool m_valid;

  VolumeFileManager m_pvlFileManager;

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
};

#endif
