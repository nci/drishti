#include "volumeinformation.h"

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
  bool xmlheader = false;

  QFile qfl(volfile);
  if (!qfl.open(QIODevice::ReadOnly | QIODevice::Text))
    {
      QMessageBox::information(0, "Cannot open", volfile);
      return false;
    }
  QString line = qfl.readLine();
  // interested in second line
  line = qfl.readLine();
  line = line.trimmed();
  //QMessageBox::information(0, volfile, "["+line+"]");
  if (line == "<PvlDotNcFileHeader>")
    xmlheader = true;
  qfl.close();

  return xmlheader;
}

bool
VolumeInformation::checkRGB(QString volfile)
{
  if (!xmlHeaderFile(volfile))
    {
//      QMessageBox::information(0, "Error",
//      QString("%1 is not a valid preprocessed volume file").arg(volfile));
      return false;
    }

  QDomDocument document;
  QFile f(volfile.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "voxeltype")
	{
	  QString pvalue = dlist.at(i).toElement().text();
	  if (pvalue == "RGB" ||
	      pvalue == "RGBA")
	    return true;
	}
    }
  return false;
}

bool
VolumeInformation::checkRGBA(QString volfile)
{
  if (!xmlHeaderFile(volfile))
    {
      QMessageBox::information(0, "Error",
      QString("%1 is not a valid preprocessed volume file").arg(volfile));
      return false;
    }

  QDomDocument document;
  QFile f(volfile.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "voxeltype")
	{
	  QString pvalue = dlist.at(i).toElement().text();
	  if (pvalue == "RGBA")
	    return true;
	}
    }
  return false;
}

bool
VolumeInformation::volInfo(QString volfile,
			   VolumeInformation& pvlInfo)
{  
  if (!xmlHeaderFile(volfile))
    {
      QMessageBox::information(0, "Error",
	QString("%1 is not a valid preprocessed volume file").arg(volfile));
      return false;
    }
      
  bool rgba = checkRGB(volfile) || checkRGBA(volfile);

  pvlInfo.pvlFile = volfile;
  pvlInfo.relativeVoxelScaling = Vec(1,1,1);


  std::vector<float> pvlmap;
  std::vector<float> rawmap;

  QDomDocument document;
  QFile f(volfile.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }
  
  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "rawfile")
	{
	  pvlInfo.rawFile = dlist.at(i).toElement().text();
	}
      else if (dlist.at(i).nodeName() == "description")
	{
	  pvlInfo.description = dlist.at(i).toElement().text();
	}
      else if (dlist.at(i).nodeName() == "voxeltype")
	{
	  QString pvalue = dlist.at(i).toElement().text();
	  if (pvalue == "unsigned char")
	    pvlInfo.voxelType = VolumeInformation::_UChar;
	  else if (pvalue == "char")
	    pvlInfo.voxelType = VolumeInformation::_Char;
	  else if (pvalue == "unsigned short")
	    pvlInfo.voxelType = VolumeInformation::_UShort;
	  else if (pvalue == "short")
	    pvlInfo.voxelType = VolumeInformation::_Short;
	  else if (pvalue == "int")
	    pvlInfo.voxelType = VolumeInformation::_Int;
	  else if (pvalue == "float")
	    pvlInfo.voxelType = VolumeInformation::_Float;
	}
      else if (dlist.at(i).nodeName() == "voxelunit")
	{
	  QString pvalue = dlist.at(i).toElement().text();
	  pvlInfo.voxelUnit = VolumeInformation::Nounit;
	  if (pvalue == "angstrom")
	    pvlInfo.voxelUnit = VolumeInformation::Angstrom;
	  else if (pvalue == "nanometer")
	    pvlInfo.voxelUnit = VolumeInformation::Nanometer;
	  else if (pvalue == "micron")
	    pvlInfo.voxelUnit = VolumeInformation::Micron;
	  else if (pvalue == "millimeter")
	    pvlInfo.voxelUnit = VolumeInformation::Millimeter;
	  else if (pvalue == "centimeter")
	    pvlInfo.voxelUnit = VolumeInformation::Centimeter;
	  else if (pvalue == "meter")
	    pvlInfo.voxelUnit = VolumeInformation::Meter;
	  else if (pvalue == "kilometer")
	    pvlInfo.voxelUnit = VolumeInformation::Kilometer;
	  else if (pvalue == "parsec")
	    pvlInfo.voxelUnit = VolumeInformation::Parsec;
	  else if (pvalue == "kiloparsec")
	    pvlInfo.voxelUnit = VolumeInformation::Kiloparsec;
	}
      else if (dlist.at(i).nodeName() == "voxelsize")
	{
	  QStringList str = (dlist.at(i).toElement().text()).split(" ", QString::SkipEmptyParts);
	  float vx = str[0].toFloat();
	  float vy = str[1].toFloat();
	  float vz = str[2].toFloat();
	  
	  if (vx <=0 || vy <= 0 || vz <= 0)
	    {
	      QMessageBox::critical(0, "Voxel Size Error",
				    QString("Voxel size <= 0 not allowednDefaulting to 1 1 1"),
				    QString("%1 %2 %3").arg(vx).arg(vy).arg(vz));
	      vx = vy = vz = 1;
	    }
	  	  
	  pvlInfo.voxelSize = Vec(vx, vy, vz);
	  float minval = qMin(vx, qMin(vy, vz));
	  if (minval > 0.00000001)
	    pvlInfo.relativeVoxelScaling = pvlInfo.voxelSize/minval;
	}
      else if (dlist.at(i).nodeName() == "gridsize")
	{
	  QStringList str = (dlist.at(i).toElement().text()).split(" ", QString::SkipEmptyParts);
	  int d = str[0].toInt();
	  int w = str[1].toInt();
	  int h = str[2].toInt();
	  pvlInfo.dimensions = Vec(d,w,h);
	}
      else if (dlist.at(i).nodeName() == "slabsize")
	{
	  pvlInfo.slabSize = (dlist.at(i).toElement().text()).toInt();
	}
      else if (dlist.at(i).nodeName() == "rawmap")
	{
	  QStringList str = (dlist.at(i).toElement().text()).split(" ", QString::SkipEmptyParts);
	  for(int im=0; im<str.count(); im++)
	    rawmap.push_back(str[im].toFloat());
	}
      else if (dlist.at(i).nodeName() == "pvlmap")
	{
	  QStringList str = (dlist.at(i).toElement().text()).split(" ", QString::SkipEmptyParts);
	  for(int im=0; im<str.count(); im++)
	    pvlmap.push_back(str[im].toInt());
	}
    }

  for(int i=0; i<(int)qMin(rawmap.size(), pvlmap.size()); i++)
    pvlInfo.mapping << QPointF(rawmap[i], pvlmap[i]);
  
  rawmap.clear();
  pvlmap.clear();

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
