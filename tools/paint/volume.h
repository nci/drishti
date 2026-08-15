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
  bool reset();
  bool exiting();

  bool createUndo();
  bool undo();

  bool reloadMask();
  bool loadRawMask(QString);

  bool setFile(QString);
  QString fileName() { return m_fileName; }
  QString maskPvlFileName() const { return m_mask.pvlFileName(); }

  void setMaskVoxelType(int vt) { m_mask.setVoxelType(vt); }
  
  
  bool saveTagNames(QStringList);
  QStringList loadTagNames();

  bool exportMask();
  void checkPoint();
  bool loadCheckPoint();
  bool loadCheckPoint(QString);
  bool deleteCheckPoint();

  bool offloadMaskFile() { return m_mask.offloadMemFile(); }
  bool offloadMemFile();
  bool loadMemFile();

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

  bool setMaskDepthSlice(int, uchar*);

  QList<int> rawValue(int, int, int);

  bool tagDSlice(int, uchar*);
  bool tagWSlice(int, uchar*);
  bool tagHSlice(int, uchar*);
  
  uchar* memVolDataPtr() {return m_pvlFileManager.memVolDataPtr();};
  ushort* memVolDataPtrUS() {return m_pvlFileManager.memVolDataPtrUS();};

  uchar* memMaskDataPtr() {return m_mask.memMaskDataPtr();};
  ushort* memMaskDataPtrUS() {return m_mask.memMaskDataPtrUS();};

  bool checkFileSave();
  bool saveIntermediateResults(bool forceSave=false);
  
  bool saveMaskBlock(int, int, int, int);
  bool saveMaskBlock(QList< QList<int> >);

  bool genHistogram(bool);
  void generateHistogramImage();

  bool saveModifiedOriginalVolume();

  QString lastError() const;

  void findStartEndForTag(int,
			  int&, int&,
			  int&, int&,
			  int&, int&);

 signals :
  void progressChanged(int);
  void progressReset();

 private :
  bool loadFileInternal(QString);
  bool prepareForStateSwap();
  bool swapState(Volume&);

  bool m_valid;

  VolumeFileManager m_pvlFileManager;

  VolumeMask m_mask;

  QString m_fileName;
  QString m_lastError;

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
