#ifndef RAW2PVL_H
#define RAW2PVL_H

#include <QtGui>
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

  static void applyFilter(uchar*, uchar*, uchar*,
			  uchar*, int, int, int,
			  int);
  
  static void applyVolumeFilter(QString, QString,
				int, int, int, int, int,
				int);

  static void subsampleVolume(QString, QString,
			      int, int,
			      int, int, int,
			      int, int);

  static void savePvl(VolumeData*,
		      int, int,
		      int, int,
		      int, int,
		      QStringList);

  static void saveIsosurface(VolumeData*,
			     int, int,
			     int, int,
			     int, int,
			     QStringList);

  static void Old2New(QString, QString);

  static void batchProcess(VolumeData*, QStringList);

 private :
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
			      int, bool);
  static void applyMeanFilterToSlice(uchar*, uchar*,
				     int,
				     int, int,
				     int, bool);
  

  static void Old2NewScalar(QString, QString);
  static void Old2NewRGBA(QString, QString);

  static void saveMHD(QString,
		      VolumeData*,
		      int, int,
		      int, int,
		      int, int);
};

#endif
