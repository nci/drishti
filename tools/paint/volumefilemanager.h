#ifndef VOLUMEFILEMANAGER_H
#define VOLUMEFILEMANAGER_H

#include "commonqtclasses.h"

#include <QProgressDialog>
#include <QStringList>
#include <QFile>
#include <QThread>

#include "filehandler.h"


typedef QList<int> IntList;

class VolumeFileManager : public QObject
{
  Q_OBJECT
  
 public :
  VolumeFileManager();
  ~VolumeFileManager();
  
  enum VoxelType
  {
    _UChar = 0,
    _Char,
    _UShort,
    _Short,
    _Int,
    _Float
  };

  void reset();

  QString fileName();
  bool exists();

  void checkPoint();
  bool loadCheckPoint();
  bool loadCheckPoint(QString);
  
  void setMemMapped(bool);
  bool isMemMapped();

  void setSaveFrequency(int o) { m_saveFreq = o; };

  void setMemChanged(bool);

  void setFilenameList(QStringList);
  void setBaseFilename(QString);
  void setHeaderSize(int);
  void setSlabSize(int);
  void setVoxelType(int);

  void setDepth(int);
  void setWidth(int);
  void setHeight(int);
  void createFile(bool, bool writeData=false);

  QStringList filenameList();
  QString baseFilename();
  int headerSize();
  int slabSize();

  int depth();
  int width();
  int height();

  int voxelType();
  int readVoxelType();
  int bytesPerVoxel();

  void removeFile();

  uchar* rawValue(int, int, int);
  uchar* interpolatedRawValue(float, float, float);

  void startBlockInterpolation();
  void endBlockInterpolation();
  uchar* blockInterpolatedRawValue(float, float, float);

  void loadMemFile();
  void saveMemFile();

  uchar* getSlice(int);
  void setSlice(int, uchar*);

  void setDepthSliceMem(int, uchar*);
  void setWidthSliceMem(int, uchar*);
  void setHeightSliceMem(int, uchar*);

  uchar* getDepthSliceMem(int);
  uchar* getWidthSliceMem(int);
  uchar* getHeightSliceMem(int);

  bool setValueMem(int, int, int, int);
  uchar* rawValueMem(int, int, int);

  uchar* memVolDataPtr() { return m_volData; }

  void saveBlock(int, int, int, int, int, int);

  void saveSlicesToFile();

  void startFileHandlerThread();
  
 signals :
    void saveDepthSlices(IntList);
    void saveWidthSlices(IntList);
    void saveHeightSlices(IntList);
    void saveDataBlock(int,int,int,int,int,int);
    
 private :
  bool m_memmapped;
  bool m_memChanged;
  int m_saveFreq, m_mcTimes;
  QString m_baseFilename;
  QStringList m_filenames;
  qint64 m_header, m_slabSize;
  int m_depth, m_width, m_height;
  int m_voxelType;
  qint64 m_bytesPerVoxel;
  uchar *m_slice;
  uchar *m_block;
  int m_blockSlices, m_startBlock, m_endBlock;

  QFile m_qfile;
  QString m_filename;
  int m_slabno, m_prevslabno;  

  uchar *m_volData;

  QList<int> m_saveDSlices;
  QList<int> m_saveWSlices;
  QList<int> m_saveHSlices;
    
  void readBlocks(int);

  void createMemFile();

  uchar* getWidthSlice(int);
  uchar* getHeightSlice(int);

  void setWidthSlice(int, uchar*);
  void setHeightSlice(int, uchar*);

  QThread* m_thread;
  FileHandler *m_handler;
};

#endif
