#include "xmlheaderfunctions.h"
#include <QMessageBox>
#include <QDomDocument>
#include "../common/src/pvlmanifest.h"

void
XmlHeaderFunctions::replaceInHeader(QString pvlFilename,
				 QString attname, QString value)
{
  QDomDocument doc;
  QFile f(pvlFilename);
  if (f.open(QIODevice::ReadOnly))
    {
      doc.setContent(&f);
      f.close();
    }

  int replace = -1;
  QDomElement topElement = doc.documentElement();
  QDomNodeList dlist = topElement.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == attname)
	{
	  replace = i;
	  break;
	}
    }

  if (replace > -1)
    {
      QDomElement de = doc.createElement(attname);
      QDomText tn = doc.createTextNode(value);
      de.appendChild(tn);
      topElement.replaceChild(de, dlist.at(replace));
    }
  else
    {
      QDomElement de = doc.createElement(attname);
      QDomText tn = doc.createTextNode(value);
      de.appendChild(tn);
      topElement.appendChild(de);
    }

  QFile fout(pvlFilename);
  if (fout.open(QIODevice::WriteOnly))
    {
      QTextStream out(&fout);
      doc.save(out, 2);
      fout.close();
    }
  else
    {
      QMessageBox::information(0, "", QString("Cannot write to "+pvlFilename));
    }
}

void
XmlHeaderFunctions::getDimensionsFromHeader(QString pvlFilename,
					 int &d, int &w, int &h)
{
  PvlManifest manifest;
  if (PvlManifestParser::parse(pvlFilename, manifest, false))
    {
      d = manifest.depth;
      w = manifest.width;
      h = manifest.height;
      return;
    }

  d = w = h = 0;
}

int
XmlHeaderFunctions::getSlabsizeFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  if (PvlManifestParser::parse(pvlFilename, manifest, false))
    return manifest.slabSize;

  return 0;
}

int
XmlHeaderFunctions::getPvlVoxelTypeFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  if (PvlManifestParser::parse(pvlFilename, manifest, false))
    return manifest.voxelType;

  return -1;
}

int
XmlHeaderFunctions::getPvlHeadersizeFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  if (PvlManifestParser::parse(pvlFilename, manifest, false))
    return manifest.headerSize;

  return -1;
}

int
XmlHeaderFunctions::getRawHeadersizeFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  if (PvlManifestParser::parse(pvlFilename, manifest, false))
    return manifest.rawHeaderSize;
  return -1;
}

QStringList
XmlHeaderFunctions::getPvlNamesFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  if (PvlManifestParser::parse(pvlFilename, manifest, false))
    return manifest.pvlNames;

  return QStringList();
}

QStringList
XmlHeaderFunctions::getRawNamesFromHeader(QString pvlFilename)
{
  PvlManifest manifest;
  if (PvlManifestParser::parse(pvlFilename, manifest, false))
    return manifest.rawNames;

  return QStringList();
}
