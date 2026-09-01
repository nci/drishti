#ifndef ZARRHANDLER_H
#define ZARRHANDLER_H

#include <QWidget>
#include <QString>
#include "volumedata.h"

class ZarrWriter
{
 public :
  static void saveZarr(QWidget*,
		       QString,
		       VolumeData*,
		       int, int,
		       int, int,
		       int, int);
};

#endif
