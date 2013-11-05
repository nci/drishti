#ifndef VOLUMERGB_H
#define VOLUMERGB_H

#include <QMutex>

#include "volumeinformation.h"
#include "volumergbbase.h"
#include "volumefilemanager.h"
#include "cropobject.h"

class VolumeRGB : public VolumeRGBBase
{
 public :
  VolumeRGB();
  ~VolumeRGB();


  void startHistogramCalculation();
  void endHistogramCalculation();

  void getColumnsAndRows(int&, int&);
  void getSliceTextureSize(int&, int&);

  uchar* getDragTexture();
  void deleteDragTexture();

  Vec getDragTextureInfo();
  void getDragTextureSize(int&, int&);

  QList<Vec> getSliceTextureSizeSlabs();
  uchar* getSliceTextureSlab(int, int);
  void deleteTextureSlab();

  bool setSubvolume(Vec, Vec,
		    int volnum = 0,
		    bool force=false);

  bool loadVolume(const char*, bool);

  QList<QString> volumeFiles();
  void setVolumeFiles(QList<QString>);

  Vec getSubvolumeMin();
  Vec getSubvolumeMax();
  Vec getSubvolumeSize();
  Vec getSubvolumeTextureSize();
  int getSubvolumeSubsamplingLevel();

  int* getSubvolume1dHistogram(int);
  int* getSubvolume2dHistogram(int);
  int* getDrag1dHistogram(int);
  int* getDrag2dHistogram(int);

  unsigned char* getSubvolumeTexture();

  VolumeInformation volInfo(int vnum=0);

  void maskRawVolume(unsigned char*,
		     QList<Vec>,
		     QList<Vec>,
		     QList<CropObject>);
  void saveOpacityVolume(unsigned char*,
			 QList<Vec>,
			 QList<Vec>,
			 QList<CropObject>);

 private :
  VolumeFileManager m_rgbaFileManager[4];

  float *m_flhist1DR, *m_flhist2DR;
  float *m_flhist1DG, *m_flhist2DG;
  float *m_flhist1DB, *m_flhist2DB;
  float *m_flhist1DA, *m_flhist2DA;
  int *m_subvolume1dHistogram, *m_subvolume2dHistogram;
  int *m_drag1dHistogram, *m_drag2dHistogram;

  Vec m_dataMin, m_dataMax;
  Vec m_subvolumeSize, m_subvolumeTextureSize;
  int m_subvolumeSubsamplingLevel;
  unsigned char* m_subvolumeTexture;

  int m_texColumns, m_texRows;
  int m_texWidth, m_texHeight;
  Vec m_dragTextureInfo;
  int m_dragTexWidth, m_dragTexHeight;
  unsigned char* m_dragTexture;
  unsigned char* m_sliceTexture;
  unsigned char* m_sliceTemp;

  int *m_subvolume1dHistogramR, *m_subvolume2dHistogramR;
  int *m_subvolume1dHistogramG, *m_subvolume2dHistogramG;
  int *m_subvolume1dHistogramB, *m_subvolume2dHistogramB;
  int *m_subvolume1dHistogramA, *m_subvolume2dHistogramA;

  int *m_drag1dHistogramR, *m_drag2dHistogramR;
  int *m_drag1dHistogramG, *m_drag2dHistogramG;
  int *m_drag1dHistogramB, *m_drag2dHistogramB;
  int *m_drag1dHistogramA, *m_drag2dHistogramA;

  int m_volnum;
  QList<QString> m_volumeFiles;
  
  QMutex m_mutex;
};

#endif
