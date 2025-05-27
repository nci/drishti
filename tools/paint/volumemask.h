#ifndef VOLUMEMASK_H
#define VOLUMEMASK_H

#include "volumefilemanager.h"

class VolumeMask : public QObject
{
  Q_OBJECT

 public :
  VolumeMask();
  ~VolumeMask();

  void saveTagNames(QStringList);
  QStringList loadTagNames();  
  
  void undo() { m_maskFileManager.undo(); }

  void exiting();
  void reset();
  void setFile(QString, bool);
  void setGridSize(int, int, int, int);
  void setVoxelType(int vt) { m_maskFileManager.setVoxelType(vt); }
  
  void exportMask();
  void checkPoint();
  bool loadCheckPoint();
  bool loadCheckPoint(QString);
  bool deleteCheckPoint();
  
  void offloadMemFile();
  void loadMemFile();
  void loadRawFile(QString);
  
  void setSaveFrequency(int t) { m_maskFileManager.setSaveFrequency(t); }

  void checkFileSave();
  void saveIntermediateResults(bool forceSave=false);
  void saveMaskBlock(int, int, int, int);
  void saveMaskBlock(QList< QList<int> >);

  uchar* getMaskDepthSliceImage(int);
  uchar* getMaskWidthSliceImage(int);
  uchar* getMaskHeightSliceImage(int);

  void setMaskDepthSlice(int, uchar*);

  ushort maskValue(int, int, int);

  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);

  uchar* memMaskDataPtr() {return m_maskFileManager.memVolDataPtr();};
  ushort* memMaskDataPtrUS() {return m_maskFileManager.memVolDataPtrUS();};

 private:
  VolumeFileManager m_maskFileManager;
  QString m_maskfile;
  int m_depth, m_width, m_height;

  uchar* m_maskslice;

  void checkMaskFile();
  void createPvlNc(QString);
};

#endif
