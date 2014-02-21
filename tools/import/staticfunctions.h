#ifndef STATICFUNCTIONS_H
#define STATICFUNCTIONS_H

#include "commonqtclasses.h"
#include <QProgressDialog>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>

class StaticFunctions 
{
 public :
  static void initQColorDialog();

  static bool checkExtension(QString, const char*);
  static bool checkURLs(QList<QUrl>, const char*);
  static bool checkRegExp(QString, QString);
  static bool checkURLsRegExp(QList<QUrl>, QString);

  static QGradientStops resampleGradientStops(QGradientStops);

  static int getScaledown(int, int);

  static void swapbytes(uchar*, int);
  static void swapbytes(uchar*, int, int);
};


#endif
