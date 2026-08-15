#ifndef PVLMANIFEST_H
#define PVLMANIFEST_H

#include <QString>
#include <QStringList>
#include <QList>

struct PvlManifest
{
  PvlManifest();

  bool valid;
  QString error;
  QString headerFile;
  int depth;
  int width;
  int height;
  int slabSize;
  int headerSize;
  int rawHeaderSize;
  int voxelType;
  int rawVoxelType;
  QString rawFile;
  QString voxelUnit;
  QString description;
  float voxelSizeX;
  float voxelSizeY;
  float voxelSizeZ;
  QList<float> rawMap;
  QList<int> pvlMap;
  QStringList pvlNames;
  QStringList rawNames;
  QStringList sourceOrder;
  // RGB/RGBA manifests use one base filename per channel.  Scalar readers
  // must never interpret these names as scalar PVL slabs.
  QStringList channelNames;
  bool isColor;
};

class PvlManifestParser
{
 public:
  enum VoxelType
  {
    UnsignedChar = 0,
    Char = 1,
    UnsignedShort = 2,
    Short = 3,
    Int = 4,
    Float = 5,
    RGB = 6,
    RGBA = 7
  };

  static bool parse(const QString&, PvlManifest&, bool validateFiles = true);
  static QStringList deriveNames(const QString&, int depth, int slabSize);
  static int bytesPerVoxel(int voxelType);

 private:
  static bool readDocument(const QString&, class QDomDocument&, QString&);
  static bool readRequiredInt(const class QDomElement&, const QString&, int&, QString&);
  static bool readTriple(const class QDomElement&, const QString&, int&, int&, int&, QString&);
  static bool readVoxelType(const class QDomElement&, const QString&, int&, QString&);
  static QStringList readNames(const class QDomElement&, const QString&, QString&, int expectedCount = -1);
  static bool validateSlab(const PvlManifest&, int, const QString&, bool);
  static bool validateColorChannel(const PvlManifest&, const QString&);
  static bool fail(PvlManifest&, const QString&);
};

#endif
