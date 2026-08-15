#include "volumeinformation.h"
#include "../../common/src/pvlmanifest.h"

#include <QMessageBox>

VolumeInformation VolumeInformation::m_volInfo[4];
void
VolumeInformation::setVolumeInformation(VolumeInformation volInfo, int vol)
{
  if (vol<4)
    m_volInfo[vol] = volInfo;
}
VolumeInformation
VolumeInformation::volumeInformation(int vol)
{
  if (vol<4)
    return m_volInfo[vol];
  else
    return m_volInfo[0];
}

QString
VolumeInformation::voxelTypeString()
{
  if (voxelType >= 0 &&
      voxelType < m_voxelTypeStrings.size())
    return m_voxelTypeStrings[voxelType];
  else
    return m_voxelTypeStrings[0];
}

QString
VolumeInformation::voxelUnitString()
{
  if (voxelUnit >= 0 &&
      voxelUnit < m_voxelUnitStrings.size())
    return m_voxelUnitStrings[voxelUnit];
  else
    return m_voxelUnitStrings[0];
}

QString
VolumeInformation::voxelUnitStringShort()
{
  if (voxelUnit >= 0 &&
      voxelUnit < m_voxelUnitStringsShort.size())
    return m_voxelUnitStringsShort[voxelUnit];
  else
    return m_voxelUnitStringsShort[0];
}

VolumeInformation::VolumeInformation()
{
  pvlFile = "";
  rawFile = "";
  description = "No description available";
  dimensions = Vec(0,0,0);
  voxelType = _UChar;
  voxelUnit = Nounit;
  voxelSize = Vec(1,1,1);
  relativeVoxelScaling = Vec(1,1,1);
  skipheaderbytes = 0;
  mapping.clear();
  slabSize = 0;
  repeatType = CycleType;

  m_voxelTypeStrings.clear();
  m_voxelTypeStrings << "unsigned char";
  m_voxelTypeStrings << "char";
  m_voxelTypeStrings << "unsigned short";
  m_voxelTypeStrings << "short";
  m_voxelTypeStrings << "int";
  m_voxelTypeStrings << "float";

  m_voxelUnitStrings.clear();
  m_voxelUnitStrings << "Nounit";
  m_voxelUnitStrings << "Angstrom";
  m_voxelUnitStrings << "Nanometer";
  m_voxelUnitStrings << "Micron";
  m_voxelUnitStrings << "Millimeter";
  m_voxelUnitStrings << "Centimeter";
  m_voxelUnitStrings << "Metre";
  m_voxelUnitStrings << "Kilometer";
  m_voxelUnitStrings << "Parsec";
  m_voxelUnitStrings << "Kiloparsec";

  m_voxelUnitStringsShort.clear();
  m_voxelUnitStringsShort << "";
  m_voxelUnitStringsShort << "A";
  m_voxelUnitStringsShort << "nm";
  //m_voxelUnitStringsShort << QString("%1m").arg(QChar(0xB5));
  m_voxelUnitStringsShort << "um";
  m_voxelUnitStringsShort << "mm";
  m_voxelUnitStringsShort << "cm";
  m_voxelUnitStringsShort << "m";
  m_voxelUnitStringsShort << "km";
  m_voxelUnitStringsShort << "p";
  m_voxelUnitStringsShort << "kp";
}

VolumeInformation::~VolumeInformation()
{
  pvlFile = "";
  rawFile = "";
  description = "No description available";
  dimensions = Vec(0,0,0);
  voxelType = _UChar;
  voxelUnit = Nounit;
  voxelSize = Vec(1,1,1);
  relativeVoxelScaling = Vec(1,1,1);
  skipheaderbytes = 0;
  mapping.clear();
  slabSize = 0;

  m_voxelTypeStrings.clear();
  m_voxelUnitStrings.clear();
  m_voxelUnitStringsShort.clear();
}

VolumeInformation&
VolumeInformation::operator=(const VolumeInformation& V)
{
  pvlFile = V.pvlFile;
  rawFile = V.rawFile;
  description = V.description;
  dimensions = V.dimensions;
  voxelType = V.voxelType;
  voxelUnit = V.voxelUnit;
  voxelSize = V.voxelSize;
  relativeVoxelScaling = V.relativeVoxelScaling;
  skipheaderbytes = V.skipheaderbytes;
  mapping = V.mapping;
  slabSize = V.slabSize;
  repeatType = V.repeatType;

  return *this;
}


bool
VolumeInformation::xmlHeaderFile(QString volfile)
{
  PvlManifest manifest;
  return PvlManifestParser::parse(volfile, manifest, false);
}

bool
VolumeInformation::checkRGB(QString volfile)
{
  PvlManifest manifest;
  return PvlManifestParser::parse(volfile, manifest, false) && manifest.isColor;
}

bool
VolumeInformation::checkRGBA(QString volfile)
{
  PvlManifest manifest;
  return PvlManifestParser::parse(volfile, manifest, false) &&
         manifest.voxelType == PvlManifestParser::RGBA;
}

bool
VolumeInformation::volInfo(QString volfile,
			   VolumeInformation& pvlInfo)
{  
  PvlManifest manifest;
  if (!PvlManifestParser::parse(volfile, manifest, false))
    {
      QMessageBox::information(0, "Error",
	QString("%1 is not a valid preprocessed volume file").arg(volfile));
      return false;
    }
  pvlInfo.pvlFile = volfile;
  pvlInfo.rawFile = manifest.rawFile;
  pvlInfo.description = manifest.description;
  pvlInfo.dimensions = Vec(manifest.depth, manifest.width, manifest.height);
  pvlInfo.slabSize = manifest.slabSize;
  if (manifest.voxelType <= PvlManifestParser::Float)
    pvlInfo.voxelType = manifest.voxelType;
  const QString unit = manifest.voxelUnit.toLower();
  const QStringList units = QStringList() << "no units" << "angstrom"
    << "nanometer" << "micron" << "millimeter" << "centimeter" << "meter"
    << "kilometer" << "parsec" << "kiloparsec";
  pvlInfo.voxelUnit = units.indexOf(unit);
  if (pvlInfo.voxelUnit < 0) pvlInfo.voxelUnit = Nounit;
  pvlInfo.voxelSize = Vec(manifest.voxelSizeX, manifest.voxelSizeY,
                          manifest.voxelSizeZ);
  const float minval = qMin(manifest.voxelSizeX,
                            qMin(manifest.voxelSizeY, manifest.voxelSizeZ));
  pvlInfo.relativeVoxelScaling = minval > 0.00000001 ?
    pvlInfo.voxelSize/minval : Vec(1, 1, 1);
  for (int i = 0; i < qMin(manifest.rawMap.count(), manifest.pvlMap.count()); ++i)
    pvlInfo.mapping << QPointF(manifest.rawMap.at(i), manifest.pvlMap.at(i));

  return true;
}

bool
VolumeInformation::checkForDoubleVolume(QList<QString> files1,
					QList<QString> files2)
{
  if (files1.count() > 0 &&
      files2.count() > 0)
    {
      // check if the all volumes have same dimensions
      VolumeInformation pvlInfo1;
      VolumeInformation pvlInfo2;
      volInfo(files1[0], pvlInfo1);
      volInfo(files2[0], pvlInfo2);
      if ((pvlInfo1.dimensions-pvlInfo2.dimensions).squaredNorm() < 1)
	{
	  int ok = QMessageBox::question(0, "Double Volume ?",
		   QString("We already have volume loaded with %1 time steps.\nLoad this volume (with %2 time steps) as double volume ?"). \
					 arg(files1.count()).\
					 arg(files2.count()),
					 QMessageBox::Yes | QMessageBox::No);
	  if (ok == QMessageBox::Yes)
	    return true;
	}
    }
  return false;
}

bool
VolumeInformation::checkForTripleVolume(QList<QString> files1,
					QList<QString> files2,
					QList<QString> files3)
{
  if (files1.count() > 0 &&
      files2.count() > 0 &&
      files3.count() > 0)
    {
      // check if the all volumes have same dimension<s
      VolumeInformation pvlInfo1;
      VolumeInformation pvlInfo2;
      VolumeInformation pvlInfo3;
      volInfo(files1[0], pvlInfo1);
      volInfo(files2[0], pvlInfo2);
      volInfo(files3[0], pvlInfo3);
      if ((pvlInfo1.dimensions-pvlInfo2.dimensions).squaredNorm() < 1 &&
	  (pvlInfo1.dimensions-pvlInfo3.dimensions).squaredNorm() < 1)
	{
	  int ok = QMessageBox::question(0, "Triple Volume ?",
		   QString("We already have 2 volumes loaded with %1 and %2 time steps.\nLoad this volume (with %3 time steps) as triple volume ?"). \
					 arg(files1.count()).	\
					 arg(files2.count()).	\
					 arg(files3.count()),
					 QMessageBox::Yes | QMessageBox::No);
	  if (ok == QMessageBox::Yes)
	    return true;
	}
    }
  return false;
}

bool
VolumeInformation::checkForQuadVolume(QList<QString> files1,
				      QList<QString> files2,
				      QList<QString> files3,
				      QList<QString> files4)
{
  if (files1.count() > 0 &&
      files2.count() > 0 &&
      files3.count() > 0 &&
      files4.count() > 0)
    {
      // check if the all volumes have same dimensions
      VolumeInformation pvlInfo1;
      VolumeInformation pvlInfo2;
      VolumeInformation pvlInfo3;
      VolumeInformation pvlInfo4;
      volInfo(files1[0], pvlInfo1);
      volInfo(files2[0], pvlInfo2);
      volInfo(files3[0], pvlInfo3);
      volInfo(files4[0], pvlInfo4);
      if ((pvlInfo1.dimensions-pvlInfo2.dimensions).squaredNorm() < 1 &&
	  (pvlInfo1.dimensions-pvlInfo3.dimensions).squaredNorm() < 1 &&
	  (pvlInfo1.dimensions-pvlInfo4.dimensions).squaredNorm() < 1)
	{
	  int ok = QMessageBox::question(0, "Quad Volume ?",
		   QString("We already have 3 volumes loaded with %1, %2 and %3 time steps.\nLoad this volume (with %4 time steps) as triple volume ?"). \
					 arg(files1.count()).	\
					 arg(files2.count()).	\
					 arg(files3.count()).	\
					 arg(files4.count()),
					 QMessageBox::Yes | QMessageBox::No);
	  if (ok == QMessageBox::Yes)
	    return true;
	}
    }
  return false;
}
