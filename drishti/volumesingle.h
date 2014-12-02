#ifndef VOLUMESINGLE_H
#define VOLUMESINGLE_H

#include <QMutex>

#include "volumeinformation.h"
#include "volumebase.h"
#include "volumefilemanager.h"
#include "cropobject.h"
#include "pathobject.h"

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class VolumeSingle : public VolumeBase
{
 public :
  VolumeSingle();
  ~VolumeSingle();

  void closePvlFileManager();
  VolumeFileManager* pvlFileManager();
  VolumeFileManager* gradFileManager();
  VolumeFileManager* lodFileManager();

  void forMultipleVolumes(int,
			  Vec, int, int,
			  int, int,
			  int, int,
			  int, int, int);

  int timestepNumber(int);

  void startHistogramCalculation();
  void endHistogramCalculation();

  void getColumnsAndRows(int&, int&);
  void getSliceTextureSize(int&, int&);
  Vec getDragTextureInfo();
  void getDragTextureSize(int&, int&);

  uchar* getDragTexture();

  QList<Vec> getSliceTextureSizeSlabs();
  uchar* getSliceTextureSlab(int, int);
  void deleteTextureSlab();

  bool loadVolume(QList<QString>, bool);
  bool loadDummyVolume(int, int, int);
  
  void setRepeatType(bool);
  int actualVolumeNumber(int);

  bool setSubvolume(Vec, Vec,
		    int subsamplingLevel = 0,
		    int volnum1 = 0,
		    bool force=false);
  
  QList<QString> volumeFiles();
  void setVolumeFiles(QList<QString>);

  Vec getSubvolumeMin();
  Vec getSubvolumeMax();
  Vec getSubvolumeSize();
  Vec getSubvolumeTextureSize();
  int getSubvolumeSubsamplingLevel();
  int* getSubvolume1dHistogram();
  int* getSubvolume2dHistogram();
  int* getDrag1dHistogram();
  int* getDrag2dHistogram();

  Vec getFullVolumeSize();

  unsigned char* getSubvolumeTexture();

  VolumeInformation volInfo(int vnum=0);

  void getSurfaceArea(unsigned char*,
		      QList<Vec>,
		      QList<Vec>,
		      QList<CropObject>,
		      QList<PathObject>);

  void saveSliceImage(Vec, Vec, Vec, Vec, float, float, int);
  void resliceVolume(Vec, Vec, Vec, Vec, float, float, int, int);

  void saveVolume(unsigned char*,
		  QList<Vec>,
		  QList<Vec>,
		  QList<CropObject>,
		  QList<PathObject>);
  
  void maskRawVolume(unsigned char*,
		     QList<Vec>,
		     QList<Vec>,
		     QList<CropObject>,
		     QList<PathObject>);

  QBitArray getBitmask(unsigned char*,
		       QList<Vec>,
		       QList<Vec>,
		       QList<CropObject>,
		       QList<PathObject>);

  QList<QVariant> rawValues(QList<Vec>);
  QMap<QString, QList<QVariant> > rawValues(int, QList<Vec>);
  QList<float> getThicknessProfile(int, uchar*, QList<Vec>, QList<Vec>);

  QList<Vec> stickToSurface(uchar*, int, QList< QPair<Vec,Vec> >);

  void countIsolatedRegions(unsigned char*,
			    QList<Vec>,
			    QList<Vec>,
			    QList<CropObject>,
			    QList<PathObject>);

 private :
  VolumeFileManager m_pvlFileManager;
  VolumeFileManager m_gradFileManager;
  VolumeFileManager m_lodFileManager;

  float *m_flhist1D, *m_flhist2D;
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
  uchar* m_dragTexture;
  uchar* m_dgTexture;
  uchar* m_sliceTexture;
  uchar* m_sliceTemp;

  int m_offH, m_offW, m_offD;
  int m_maxHeight, m_maxWidth, m_maxDepth;

  qint64 m_nonZeroVoxels;
  QBitArray m_bitmask;

  int m_volnum;
  QList<QString> m_volumeFiles;
  
  bool m_repeatType;

  QMutex m_mutex;

  void createBitmask(int, int,
		     int, int,
		     int, int,
		     unsigned char*,
		     QList<Vec>,
		     QList<Vec>,
		     QList<CropObject>,
		     QList<PathObject>);

  void getSurfaceBitmask(int, int,
			 int, int,
			 int, int);
  void findConnectedRegion(QList<Vec>,
			   int, int,
			   int, int,
			   int, int,
			   int, int,
			   QList<Vec>,
			   QList<Vec>);

  void saveSubsampledVolume();
  void createSubsampledVolume();

  void checkGradients();
  void createGradVolume();

  void calculateGradientsForDragTexture();

  void setBasicInformation(int);
};

#endif
