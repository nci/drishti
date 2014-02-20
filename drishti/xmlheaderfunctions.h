#ifndef XMLHEADERFUNCTION_H
#define XMLHEADERFUNCTION_H

#include "commonqtclasses.h"

class XmlHeaderFunctions
{
 public :
  static void replaceInHeader(QString, QString, QString);

  static void getDimensionsFromHeader(QString, int&, int&, int&);
  static int getSlabsizeFromHeader(QString);
  static int getPvlVoxelTypeFromHeader(QString);
  static int getPvlHeadersizeFromHeader(QString);
  static int getRawHeadersizeFromHeader(QString);
  static QStringList getPvlNamesFromHeader(QString);
  static QStringList getRawNamesFromHeader(QString);
};

#endif
