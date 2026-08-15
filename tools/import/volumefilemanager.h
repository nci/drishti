#ifndef VOLUMEFILEMANAGER_H
#define VOLUMEFILEMANAGER_H

#include "commonqtclasses.h"
#include <QFile>
#include <QProgressDialog>
#include <QStringList>

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

  QString fileName();
  bool exists();

  void setBaseFilename(QString);
  void setHeaderSize(int);
  void setSlabSize(int);
  void setVoxelType(int);

  void setDepth(int);
  void setWidth(int);
  void setHeight(int);
  bool createFile(bool);

  bool commitFileCreation(bool releaseBackup=true);
  bool finalizeFileCreation();
  bool rollbackFileCreation();
  QString lastError() const;
  
  void removeFile();

  uchar* getSlice(int);
  bool setSlice(int, uchar*);

  void setSliceZeroAtTop(bool);

 private :
  QString m_baseFilename;
  qint64 m_header;
  int m_slabSize;
  int m_depth, m_width, m_height;
  int m_voxelType;
  qint64 m_bytesPerVoxel;
  uchar *m_slice;
  std::size_t m_sliceCapacity;

  QFile m_qfile;
  QString m_filename;
  int m_slabno, m_prevslabno;  

  bool m_slice0AtTop;
  QString m_lastError;
  QStringList m_transactionFinals;
  QStringList m_transactionBackups;
  QList<bool> m_transactionHadOriginal;

  void reset();
  static bool checkedMultiply(qint64, qint64, qint64&);
  static bool checkedAdd(qint64, qint64, qint64&);
  bool setError(const QString&);
  void clearError();
  bool validateGeometry(const QString&);
  bool sliceByteCount(qint64&, const QString&);
  bool volumeByteCount(qint64&, const QString&);
  bool slabFileSize(int, qint64, qint64&, const QString&);
  bool ensureSliceCapacity(qint64, const QString&);
  QString slabFilename(int) const;
  bool seekFile(QFile&, qint64, const QString&);
  bool readExact(QFile&, uchar*, qint64, const QString&);
  bool writeExact(QFile&, const uchar*, qint64, const QString&);
  bool flushAndCheckSize(QFile&, qint64, const QString&);
  void cleanupFiles(const QStringList&);
  void clearTransaction();
};

#endif
