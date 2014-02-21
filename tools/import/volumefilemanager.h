#ifndef VOLUMEFILEMANAGER_H
#define VOLUMEFILEMANAGER_H

#include "commonqtclasses.h"
#include <QFile>
#include <QProgressDialog>

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
  void createFile(bool);
  
  void removeFile();

  uchar* getSlice(int);
  void setSlice(int, uchar*);

  void setSliceZeroAtTop(bool);

 private :
  QString m_baseFilename;
  int m_header, m_slabSize;
  int m_depth, m_width, m_height;
  int m_voxelType;
  int m_bytesPerVoxel;
  uchar *m_slice;

  QFile m_qfile;
  QString m_filename;
  int m_slabno, m_prevslabno;  

  bool m_slice0AtTop;

  void reset();
};

#endif
