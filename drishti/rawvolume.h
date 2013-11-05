#ifndef RAWVOLUME_H
#define RAWVOLUME_H

#include <QtGui>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

#include "volumefilemanager.h"

class RawVolume
{
 public :

  static void setDepth(int);
  static void setWidth(int);
  static void setHeight(int);
  static void setSlabSize(int);
  static void setVoxelType(int);

  static QVariant rawValue(Vec);
  static QVariant rawValue(int, int, int);
  static QList<QVariant> rawValues(QList<Vec>);
  static QMap<QString, QList<QVariant> > rawValues(int, QList<Vec>);

  static QVariant tagValue(Vec);
  static QVariant tagValue(int, int, int);
  static QList<QVariant> tagValues(QList<Vec>);
  
  static void reset();

  static void maskRawVolume(int, int,
			    int, int,
			    int, int,
			    QBitArray);

  static void extractPath(QList<Vec>,
			  QList<Vec>, QList<float>,
			  int, int, bool,
			  Vec, Vec, QBitArray);

  static void extractPatch(QList<Vec>,
			   QList<Vec>, QList<float>,
			   int, int, bool,
			   bool, int,
			   Vec, Vec, QBitArray);

 private :
  static QString m_rawFileName;
  static VolumeFileManager m_rawFileManager;

  static void setRawFileName();

  static int m_depth, m_width, m_height;
  static int m_slabSize, m_voxelType;
};

#endif
