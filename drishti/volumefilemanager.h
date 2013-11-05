#ifndef VOLUMEFILEMANAGER_H
#define VOLUMEFILEMANAGER_H

#include <QStringList>
#include <QFile>

#include <fstream>
using namespace std;

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

  void setFilenameList(QStringList);
  void setBaseFilename(QString);
  void setHeaderSize(int);
  void setSlabSize(int);
  void setVoxelType(int);

  void setDepth(int);
  void setWidth(int);
  void setHeight(int);
  void createFile(bool);

  QStringList filenameList();
  QString baseFilename();
  int headerSize();
  int slabSize();

  int depth();
  int width();
  int height();

  int voxelType();
  int readVoxelType();

  void removeFile();

  uchar* getSlice(int);
  void setSlice(int, uchar*);

  uchar* rawValue(int, int, int);
  uchar* interpolatedRawValue(float, float, float);

  void startBlockInterpolation();
  void endBlockInterpolation();
  uchar* blockInterpolatedRawValue(float, float, float);

  void save(fstream&);
  void load(fstream&);

 private :
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

  void reset();
  void readBlocks(int);
};

#endif
