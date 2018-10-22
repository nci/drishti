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
  void setFile(QString, bool);
  void setGridSize(int, int, int, int);

  void exportMask();
  void checkPoint();
  bool loadCheckPoint();
  bool loadCheckPoint(QString);
  bool deleteCheckPoint();
  
  void offLoadMemFile();
  void loadMemFile();

  void setSaveFrequency(int t) { m_maskFileManager.setSaveFrequency(t); }

  void saveIntermediateResults(bool forceSave=false);
  void saveMaskBlock(int, int, int, int);
  void saveMaskBlock(QList< QList<int> >);

  uchar* getMaskDepthSliceImage(int);
  uchar* getMaskWidthSliceImage(int);
  uchar* getMaskHeightSliceImage(int);

  void setMaskDepthSlice(int, uchar*);

  uchar maskValue(int, int, int);

  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);

  uchar* memMaskDataPtr() {return m_maskFileManager.memVolDataPtr();};

 private:
  VolumeFileManager m_maskFileManager;
  QString m_maskfile;
  int m_depth, m_width, m_height;

  uchar* m_maskslice;

  void checkMaskFile();
};

#endif
