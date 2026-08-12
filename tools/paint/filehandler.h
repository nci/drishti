#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "commonqtclasses.h"

#include <QAtomicInt>
#include <QFile>
#include <QMutex>
#include <QStringList>

class QIODevice;

class FileHandler : public QObject
{
  Q_OBJECT

 public :
  FileHandler();
  ~FileHandler();

  bool savingFile() const { return m_savingFile.loadAcquire() != 0; }
  QString lastError() const { return m_lastError; }
  void requestCancel() { m_cancelRequested.storeRelease(1); }

 signals :
  void doneFileSave(quint64, bool, QString);
  void fileSaveProgress(quint64);

 public slots :
  void setFilenameList(QStringList);
  void setBaseFilename(QString);
  void setHeaderSize(int);
  void setSlabSize(int);
  void setVoxelType(int);
  void setDepth(int);
  void setWidth(int);
  void setHeight(int);
  void setVolData(uchar*, qint64);

  bool saveMemFile(quint64 generation = 0);
  void saveSnapshotFile(QString, quint64);
  bool loadMemFile();

  bool genUndo();
  bool undo();
  void saveDataBlock();

 private:
  QAtomicInt m_savingFile;
  QAtomicInt m_cancelRequested;
  QFile m_qfile;
  QString m_filename;
  QString m_baseFilename;
  QStringList m_filenames;
  qint64 m_header, m_slabSize;
  int m_depth, m_width, m_height;
  int m_voxelType;
  qint64 m_bytesPerVoxel;

  uchar *m_volData;
  qint64 m_volDataCapacity;

  QMutex m_mutex;
  QString m_tempFile;
  QString m_lastError;

  void reset();
  bool loadMemFile(QString);
  bool validateConfiguration(const QString&, qint64&, qint64&);
  bool setError(const QString&);
  static bool checkedMultiply(qint64, qint64, qint64&);
  static bool checkedAdd(qint64, qint64, qint64&);
  bool readExact(QIODevice&, uchar*, qint64, const QString&);
  bool writeExact(QIODevice&, const uchar*, qint64, const QString&);
  bool copyFileTransactional(const QString&, const QString&);
  bool canceled(const QString&);
};

#endif
