#ifndef VOLUMEFILEMANAGER_H
#define VOLUMEFILEMANAGER_H

#include "commonqtclasses.h"

#include <QProgressDialog>
#include <QStringList>
#include <QFile>
#include <QTimer>
#include <QThread>

#include <cstddef>

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

  bool reset();

  QString fileName();
  bool exists();

  bool exiting();
  
  QString exportMask();
  void checkPoint();
  bool loadCheckPoint();
  bool loadCheckPoint(QString);
  bool deleteCheckPoint();
  
  bool setMemMapped(bool);
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
  bool createFile(bool, bool writeData=false);

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

  bool loadMemFile();
  bool saveMemFile();
  bool requestSave();
  bool flushPendingChanges();
  bool loadRawFile(QString);

  uchar* getSlice(int);
  bool setSlice(int, uchar*);

  bool setDepthSliceMem(int, uchar*);
  bool setWidthSliceMem(int, uchar*);
  bool setHeightSliceMem(int, uchar*);

  uchar* getDepthSliceMem(int);
  uchar* getWidthSliceMem(int);
  uchar* getHeightSliceMem(int);

  bool setValueMem(int, int, int, int);
  uchar* rawValueMem(int, int, int);

  uchar* memVolDataPtr() { return m_volData; }
  ushort* memVolDataPtrUS() { return (ushort*)m_volData; }

  bool saveBlock();

  bool saveSlicesToFile();
  bool checkFileSave();
  
  bool startFileHandlerThread();

  bool undo();
  bool createUndo();

  QString lastError() const;

 signals :
  void saveSnapshot(QString, quint64);
  void saveCycleFinished();
  void saveCycleProgressed();
  void saveDepthSlices(IntList);
  void saveWidthSlices(IntList);
  void saveHeightSlices(IntList);
  void saveDataBlock(int,int,int,int,int,int);
					     
 public slots :
  void doneFileSave(quint64, bool, QString);
  void beginBackgroundSave();
  void fileSaveProgress(quint64);
  void fileHandlerThreadFinished();
  
 private :
  bool m_fileHandlerBusy;
  bool m_waitingOnFileHandler;
  bool m_saveRequested;
  bool m_backgroundSaveFailed;
  
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
  size_t m_sliceCapacity;
  uchar *m_block;
  size_t m_blockCapacity;
  int m_blockSlices, m_startBlock, m_endBlock;

  QFile m_qfile;
  QString m_filename;
  int m_slabno, m_prevslabno;  

  uchar *m_volData;
  size_t m_volDataCapacity;
  QString m_lastError;
  quint64 m_changeGeneration;
  quint64 m_saveGeneration;
  QString m_snapshotPath;

  QList<int> m_saveDSlices;
  QList<int> m_saveWSlices;
  QList<int> m_saveHSlices;
    
  bool readBlocks(int);

  bool createMemFile();

  uchar* getWidthSlice(int);
  uchar* getHeightSlice(int);

  bool setWidthSlice(int, uchar*);
  bool setHeightSlice(int, uchar*);

  static bool checkedMultiply(qint64, qint64, qint64&);
  static bool checkedAdd(qint64, qint64, qint64&);
  bool validateGeometry(const QString&, bool requireFilename=true);
  bool sliceByteCount(qint64&, const QString&);
  bool volumeByteCount(qint64&, const QString&);
  bool planeByteCount(int, int, qint64&, const QString&);
  bool ensureSliceCapacity(qint64, const QString&);
  bool ensureBlockCapacity(qint64, const QString&);
  bool setError(const QString&);
  void clearError();
  QString slabFilename(int) const;
  bool openSlab(int, QIODevice::OpenMode, const QString&);
  bool seekFile(QFile&, qint64, const QString&);
  bool readExact(QFile&, uchar*, qint64, const QString&);
  bool writeExact(QFile&, const uchar*, qint64, const QString&);
  bool flushFile(QFile&, const QString&);
  bool expectedSlabSize(int, qint64, qint64&, const QString&);
  void cleanupPartialFiles(const QStringList&);
  void configureFileHandler(FileHandler&);
  bool loadCompressedMask(uchar*, qint64);
  bool saveCompressedMask(uchar*, qint64);
  bool stopFileHandlerThread(bool);
  bool queueFileSave();
  bool createSaveSnapshot(QString&, quint64&);
  bool waitForBackgroundSave();
  void markChanged();

  QThread* m_thread;
  FileHandler *m_handler;
  QTimer *m_saveDebounceTimer;
};

#endif
