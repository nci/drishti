#ifndef VOLUMEFILEMANAGER_H
#define VOLUMEFILEMANAGER_H

#include "commonqtclasses.h"

#include <QProgressDialog>
#include <QStringList>
#include <QFile>

#include <cstddef>

class VolumeFileManager
{
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

  void closeQFile();

  void setMemMapped(bool);
  bool isMemMapped();

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

  QString lastError() const;

  QStringList filenameList();
  QString baseFilename();
  int headerSize();
  int slabSize();

  int depth();
  int width();
  int height();

  int bytesPerVoxel();
  int voxelType();
  int readVoxelType();

  void removeFile();

  uchar* getSlice(int);
  uchar* getWidthSlice(int);
  uchar* getHeightSlice(int);

  bool setSlice(int, uchar*);
  bool setWidthSlice(int, uchar*);
  bool setHeightSlice(int, uchar*);

  uchar* rawValue(int, int, int);
  uchar* interpolatedRawValue(float, float, float);

  void startBlockInterpolation();
  void endBlockInterpolation();
  uchar* blockInterpolatedRawValue(float, float, float);

  bool loadMemFile();
  bool saveMemFile();
  uchar* getSliceMem(int);
  bool setSliceMem(int, uchar*);
  uchar* getWidthSliceMem(int);
  bool setWidthSliceMem(int, uchar*);
  uchar* getHeightSliceMem(int);
  bool setHeightSliceMem(int, uchar*);
  uchar* rawValueMem(int, int, int);

  bool setValueMem(int, int, int, int);

  uchar* memVolDataPtr() { return m_volData; }

  void saveBlock(int, int, int, int, int, int);

  bool changeSliceOrdering();

 private :
  bool m_memmapped;
  bool m_memChanged;
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

  bool readBlocks(int);

  bool createMemFile();

  static bool checkedMultiply(qint64, qint64, qint64&);
  static bool checkedAdd(qint64, qint64, qint64&);
  bool validateGeometry(const QString&);
  bool sliceByteCount(qint64&, const QString&);
  bool volumeByteCount(qint64&, const QString&);
  bool slabFileSize(int, qint64, qint64&, const QString&);
  bool ensureSliceCapacity(qint64, const QString&);
  bool ensureBlockCapacity(qint64, const QString&);
  bool setError(const QString&);
  void clearError();
  QString slabFilename(int) const;
  bool openSlab(int, QIODevice::OpenMode, const QString&);
  bool seekFile(QFile&, qint64, const QString&);
  bool readExact(QFile&, uchar*, qint64, const QString&);
  bool writeExact(QFile&, const uchar*, qint64, const QString&);
  bool flushAndCheckSize(QFile&, qint64, const QString&);
  void cleanupPartialFiles(const QStringList&);
};

#endif
