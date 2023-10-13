#include "xmlheaderfunctions.h"
#include <QMessageBox>
#include <QDomDocument>

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
  QDomDocument document;
  QFile f(pvlFilename.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "gridsize")
	{
	  QStringList str = (dlist.at(i).toElement().text()).split(" ", QString::SkipEmptyParts);
	  d = str[0].toFloat();
	  w = str[1].toFloat();
	  h = str[2].toFloat();
	  return;
	}
    }
}

int
XmlHeaderFunctions::getSlabsizeFromHeader(QString pvlFilename)
{
  QDomDocument document;
  QFile f(pvlFilename.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "slabsize")
	return (dlist.at(i).toElement().text()).toInt();
    }
  return 0;
}

int
XmlHeaderFunctions::getPvlVoxelTypeFromHeader(QString pvlFilename)
{
  QDomDocument document;
  QFile f(pvlFilename.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "pvlvoxeltype")
	{
	  if (dlist.at(i).toElement().text() == "unsigned char") return 0;
	  if (dlist.at(i).toElement().text() == "char") return 1;
	  if (dlist.at(i).toElement().text() == "unsigned short") return 2;
	  if (dlist.at(i).toElement().text() == "short") return 3;
	  if (dlist.at(i).toElement().text() == "int") return 4;
	  if (dlist.at(i).toElement().text() == "float") return 5;
	}
    }
  return 0;
}

int
XmlHeaderFunctions::getPvlHeadersizeFromHeader(QString pvlFilename)
{
  QDomDocument document;
  QFile f(pvlFilename.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "pvlheadersize")
	return (dlist.at(i).toElement().text()).toInt();
    }

  // default is 13 byte header
  return 13;
}

int
XmlHeaderFunctions::getRawHeadersizeFromHeader(QString pvlFilename)
{
  QDomDocument document;
  QFile f(pvlFilename.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "rawheadersize")
	return (dlist.at(i).toElement().text()).toInt();
    }

  // default is 13 byte header
  return 13;
}

QStringList
XmlHeaderFunctions::getPvlNamesFromHeader(QString pvlFilename)
{
  QStringList filenames;

  QDomDocument document;
  QFile f(pvlFilename.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "pvlnames")
	{
	  QString names = (dlist.at(i).toElement().text()).simplified();
	  QStringList flnms = names.split(" ", QString::SkipEmptyParts);

	  QFileInfo fileInfo(pvlFilename);
	  QDir direc = fileInfo.absoluteDir();
	  for(int fi=0; fi<flnms.count(); fi++)
	    {
	      fileInfo.setFile(direc, flnms[fi]);
	      filenames << fileInfo.absoluteFilePath();
	    }

	  return filenames;
	}
    }

  return filenames;
}

QStringList
XmlHeaderFunctions::getRawNamesFromHeader(QString pvlFilename)
{
  QStringList filenames;

  QDomDocument document;
  QFile f(pvlFilename.toUtf8().data());
  if (f.open(QIODevice::ReadOnly))
    {
      document.setContent(&f);
      f.close();
    }

  QDomElement main = document.documentElement();
  QDomNodeList dlist = main.childNodes();
  for(int i=0; i<dlist.count(); i++)
    {
      if (dlist.at(i).nodeName() == "rawnames")
	{
	  QString names = (dlist.at(i).toElement().text()).simplified();
	  QStringList flnms = names.split(" ", QString::SkipEmptyParts);

	  QFileInfo fileInfo(pvlFilename);
	  QDir direc = fileInfo.absoluteDir();
	  for(int fi=0; fi<flnms.count(); fi++)
	    {
	      fileInfo.setFile(direc, flnms[fi]);
	      filenames << fileInfo.absoluteFilePath();
	    }

	  return filenames;
	}
    }

  return filenames;
}
