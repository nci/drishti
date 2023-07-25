#ifndef RAW2PVL_H
#define RAW2PVL_H

#include "vdbvolume.h"
#include "volumedata.h"

class Raw2Pvl
{
 public :
  enum VoxelType
  {
    _UChar = 0,
    _Char,
    _UShort,
    _Short,
    _Int,
    _Float
  };
  
  enum VoxelUnit {
    _Nounit = 0,
    _Angstrom,
    _Nanometer,
    _Micron,
    _Millimeter,
    _Centimeter,
    _Meter,
    _Kilometer,
    _Parsec,
    _Kiloparsec
  };

  enum VolumeFilters {
    _NoFilter = 0,
    _MeanFilter,
    _MedianFilter,
    _BilateralFilter
  };
  
  static void applyMapping(uchar*, int, QList<float>,
			   uchar*, int, QList<int>,
			   int, int);

  static void savePvl(VolumeData*,
		      int, int,
		      int, int,
		      int, int,
		      QStringList);

  static void mergeVolumes(VolumeData*,
			   int, int,
			   int, int,
			   int, int,
			   QStringList);

  static void quickRaw(VolumeData*,
		       QStringList);

  static void saveIsosurface(VolumeData*,
			     int, int,
			     int, int,
			     int, int,
			     QStringList);

  static void batchProcess(VolumeData*, QStringList);

 private :
  static int m_vdb_bType;
  static float m_vdb_bValue1, m_vdb_bValue2, m_vdb_resample;
  
  static void createPvlNcFile(QString,
			      bool, QString,
			      int, int,
			      int, int, int,
			      float, float, float,
			      QList<float>, QList<int>,
			      QString,
			      int);

  static void savePvlHeader(QString,
			    bool, QString,
			    int, int, int,
			    int, int, int,
			    float, float, float,
			    QList<float>, QList<int>,
			    QString,
			    int);

  static void applyMeanFilter(uchar**, uchar*,
			      int,
			      int, int,
			      int, bool,
			      float*);
  static void applyMeanFilterToSlice(uchar*, uchar*,
				     int,
				     int, int,
				     int, bool,
				     float*);
  
  static void saveMHD(QString,
		      VolumeData*,
		      int, int,
		      int, int,
		      int, int);

  static bool saveVDB(int,
		      QString,
		      VolumeData*);


  
  static bool getValues(int&, float&,
			int&, float&, float&,
			float&, float&,
			int&, int&,
			int&, int&,
			QColor&, bool&);

  static void getBackgroundValues(int&, float&, float&);


  static void saveIsosurfaceRange(VolumeData*,
				  int, int,
				  int, int,
				  int, int,
				  QStringList,
				  QString,
				  bool,
				  float, float,
				  float, float,
				  int, int,
				  int, int,
				  QColor,
				  bool);

  static void populateVDB(VdbVolume&,
			  VolumeData*,
			  uchar, int, int,
			  int, int,
			  int, int,
			  int, int,
			  uchar*, float*,
			  bool,
			  int,
			  QProgressDialog&);
  
  static void applyMorpho(VdbVolume&,
			  int, float,
			  QProgressDialog&);
  
  
  static void levelSetMeshAndSave(VdbVolume&,
				  VolumeData*,
				  QString,
				  float,
				  bool,
				  bool,
				  QColor,
				  QProgressDialog&);


  static void parIsoGen(VolumeData*,
			uchar,
			int, int, int,
			int, int,
			int, int,
			int, int,
			bool,
			int,
			QString,
			float,
			bool,
			int, int,
			int, float,
			float);

  static void mapIsoGen(QList<QVariant>);
};

#endif
