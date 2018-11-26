#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "commonqtclasses.h"

#include <QProgressDialog>
#include <QFile>
#include <QMutexLocker>

typedef QList<int> IntList;

class FileHandler : public QObject
{
  Q_OBJECT

 public :
  FileHandler();
  ~FileHandler();

  bool savingFile() { return m_savingFile; }

  public slots :
    void setFilenameList(QStringList);
    void setBaseFilename(QString);
    void setHeaderSize(int);
    void setSlabSize(int);
    void setVoxelType(int);
    void setDepth(int);
    void setWidth(int);
    void setHeight(int);

    void setVolData(uchar*);
    
    void saveMemFile();
    void loadMemFile();

    void genUndo();
    void undo();
    
    void saveDataBlock(int,int,
		       int,int,
		       int,int);

    void saveDepthSlices(IntList);
    void saveWidthSlices(IntList);
    void saveHeightSlices(IntList);
    
 private:
    bool m_savingFile;
    QFile m_qfile;
    QString m_filename;
    QString m_baseFilename;
    QStringList m_filenames;
    qint64 m_header, m_slabSize;
    int m_depth, m_width, m_height;
    int m_voxelType;
    qint64 m_bytesPerVoxel;

    uchar *m_volData;    

    QMutex m_mutex;

    QString m_tempFile;
    
    void reset();

    void loadMemFile(QString);
};

#endif
