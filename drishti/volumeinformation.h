#ifndef VOLUMEINFORMATION_H
#define VOLUMEINFORMATION_H

#include <GL/glew.h>

#include <QGLViewer/vec.h>
using namespace qglviewer;

class VolumeInformation
{
 public :

  static bool xmlHeaderFile(QString);

  static void setVolumeInformation(VolumeInformation, int vol=0);
  static VolumeInformation volumeInformation(int vol=0);

  static bool volInfo(QString, VolumeInformation&);

  static bool checkRGB(QString);
  static bool checkRGBA(QString);

  static bool checkForDoubleVolume(QList<QString>, QList<QString>);
  static bool checkForTripleVolume(QList<QString>, QList<QString>,
				   QList<QString>);
  static bool checkForQuadVolume(QList<QString>, QList<QString>,
				 QList<QString>, QList<QString>);


  VolumeInformation();
  ~VolumeInformation();
  VolumeInformation& operator=(const VolumeInformation&);

  QString voxelTypeString();
  QString voxelUnitString();
  QString voxelUnitStringShort();  

  enum VoxelType {
    _UChar,
    _Char,
    _UShort,
    _Short,
    _Int,
    _Float
  };
  enum VoxelUnit {
    Nounit = 0,
    Angstrom,
    Nanometer,
    Micron,
    Millimeter,
    Centimeter,
    Meter,
    Kilometer,
    Parsec,
    Kiloparsec
  };

  enum RepeatType {
    CycleType,
    WaveType
  };

  QString pvlFile;
  QString rawFile;
  QString description;
  Vec dimensions;
  int voxelType;
  int voxelUnit;
  Vec voxelSize;
  Vec relativeVoxelScaling;
  int skipheaderbytes;
  QPolygonF mapping;
  int slabSize;
  int repeatType;

  private :
  static VolumeInformation m_volInfo[4];

  QStringList m_voxelTypeStrings;
  QStringList m_voxelUnitStrings;
  QStringList m_voxelUnitStringsShort;
};

#endif
