#ifndef VOLUME_H
#define VOLUME_H

#include "volumesingle.h"
#include "volumergb.h"

class Volume : public QObject
{


 public :
  Volume();
  ~Volume();

  bool valid();

  void clearVolumes();

  int timestepNumber(int, int);

  void startHistogramCalculation();
  void endHistogramCalculation();
  
  void forceCreateLowresVolume();

  int pvlVoxelType(int);

  VolumeFileManager* pvlFileManager(int);
  VolumeFileManager* gradFileManager(int);
  VolumeFileManager* lodFileManager(int);

  bool loadVolumeRGB(const char*, bool);
  bool loadDummyVolume(int, int, int);
  bool loadVolume(QList<QString>, bool);
  bool loadVolume(QList<QString>,
		  QList<QString>,
		  bool);
  bool loadVolume(QList<QString>,
		  QList<QString>,		  
		  QList<QString>,		  
		  bool);
  bool loadVolume(QList<QString>,
		  QList<QString>,
		  QList<QString>,
		  QList<QString>,		  
		  bool);

  QList<QString> volumeFiles(int vol=0);

  bool setSubvolume(Vec, Vec,
		    int volnum = 0,
		    bool force=false);
  bool setSubvolume(Vec, Vec,
		    int volnum0 = 0, int volnum1 = 0,
		    bool force=false);
  bool setSubvolume(Vec, Vec,
		    int volnum0 = 0, int volnum1 = 0,
		    int volnum2 = 0,
		    bool force=false);
  bool setSubvolume(Vec, Vec,
		    int volnum0 = 0, int volnum1 = 0,
		    int volnum2 = 0, int volnum3 = 0,
		    bool force=false);

  void setRepeatType(int, bool);
  void setRepeatType(QList<bool>);

  Vec getSubvolumeSize();
  Vec getSubvolumeTextureSize();
  int getSubvolumeSubsamplingLevel();

  int* getLowres1dHistogram(int vol=0);
  int* getLowres2dHistogram(int vol=0);

  int* getSubvolume1dHistogram(int vol=0);
  int* getSubvolume2dHistogram(int vol=0);

  int* getDrag1dHistogram(int vol=0);
  int* getDrag2dHistogram(int vol=0);

  unsigned char* getSubvolumeTexture();

  VolumeInformation volInfo(int vnum=0, int vol=0);

  Vec getFullVolumeSize();
  Vec getLowresVolumeSize();
  Vec getLowresTextureVolumeSize();
  int* getLowres1dHistogram();
  int* getLowres2dHistogram();
  unsigned char* getLowresTextureVolume();

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


  void getColumnsAndRows(int&, int&);
  void getSliceTextureSize(int&, int&);

  uchar* getSlabTexture(int);

  QList<Vec> getSliceTextureSizeSlabs();
  uchar* getSliceTextureSlab(int, int);
  void deleteTextureSlab();

  uchar* getDragTexture();
  Vec getDragTextureInfo();
  void getDragTextureSize(int&, int&);

  QList<QVariant> rawValues(QList<Vec>);
  QMap<QString, QList<QVariant> > rawValues(int, QList<Vec>);
  QList<float> getThicknessProfile(int, uchar*, QList<Vec>, QList<Vec>);

  QList<Vec> stickToSurface(uchar*, int, QList< QPair<Vec,Vec> >);

  void countIsolatedRegions(uchar*,
			    QList<Vec>,
			    QList<Vec>,
			    QList<CropObject>,
			    QList<PathObject>);

 private :
  QList<VolumeSingle*> m_volume;

  VolumeRGB* m_volumeRGB;

  uchar* m_subvolumeTexture;
  uchar* m_dragTexture;
  uchar* m_lowresTexture;
};

#endif
