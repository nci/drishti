#ifndef VOLUMEOPERATIONS_H
#define VOLUMEOPERATIONS_H

#include "commonqtclasses.h"
#include <QProgressDialog>

#include <QGLViewer/vec.h>
using namespace qglviewer;

#include "mybitarray.h"

class VolumeOperations
{
 public :
  static void setVolData(uchar*);
  static void setMaskData(uchar*);
  static void setGridSize(int, int, int);

  static void getVolume(Vec, Vec, int,
			QList<Vec>, QList<Vec>);

  static void connectedRegion(int, int, int,
			      Vec, Vec,
			      int, int,
			      QList<Vec>, QList<Vec>,
			      int&, int&,
			      int&, int&,
			      int&, int&);
 private :
  static int m_depth, m_width, m_height;
  static uchar *m_volData;
  static uchar *m_maskData;

  static void getConnectedRegion(int, int, int,
				 int, int, int,
				 int, int, int,
				 int, bool,
				 QList<Vec>, QList<Vec>,
				 MyBitArray&);
};

#endif
