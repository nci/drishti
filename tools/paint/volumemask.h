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
  
  bool createUndo() { return m_maskFileManager.createUndo(); }
  bool undo() { return m_maskFileManager.undo(); }

  bool exiting();
  bool reset();
  bool setFile(QString, bool);
  bool setGridSize(int, int, int, int);
  void setVoxelType(int vt) { m_maskFileManager.setVoxelType(vt); }
  
  bool exportMask();
  void checkPoint();
  bool loadCheckPoint();
  bool loadCheckPoint(QString);
  bool deleteCheckPoint();
  
  bool offloadMemFile();
  bool loadMemFile();
  bool loadRawFile(QString);
  
  void setSaveFrequency(int t) { m_maskFileManager.setSaveFrequency(t); }

  bool checkFileSave();
  bool saveIntermediateResults(bool forceSave=false);
  bool saveMaskBlock(int, int, int, int);
  bool saveMaskBlock(QList< QList<int> >);

  uchar* getMaskDepthSliceImage(int);
  uchar* getMaskWidthSliceImage(int);
  uchar* getMaskHeightSliceImage(int);

  bool setMaskDepthSlice(int, uchar*);

  ushort maskValue(int, int, int);

  bool tagDSlice(int, uchar*);
  bool tagWSlice(int, uchar*);
  bool tagHSlice(int, uchar*);

  QString lastError() const
  {
    const QString fileError = m_maskFileManager.lastError();
    return fileError.isEmpty() ? m_lastError : fileError;
  }

  uchar* memMaskDataPtr() {return m_maskFileManager.memVolDataPtr();};
  ushort* memMaskDataPtrUS() {return m_maskFileManager.memVolDataPtrUS();};

 private:
  VolumeFileManager m_maskFileManager;
  QString m_maskfile;
  QString m_lastError;
  int m_depth, m_width, m_height;

  uchar* m_maskslice;

  bool checkMaskFile();
  bool createPvlNc(QString, QString=QString());
};

#endif
