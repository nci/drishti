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
  static void setClip(QList<Vec>, QList<Vec>);

  static void getVolume(Vec, Vec, int);

  static void hatchConnectedRegion(int, int, int,
				   Vec, Vec,
				   int, int,
				   int, int,
				   int&, int&,
				   int&, int&,
				   int&, int&);

  static void connectedRegion(int, int, int,
			      Vec, Vec,
			      int, int,
			      int&, int&,
			      int&, int&,
			      int&, int&);

  static void resetTag(Vec, Vec, int,
		       int&, int&,
		       int&, int&,
		       int&, int&);

  static void shrinkwrap(Vec, Vec, int,
			 bool, int,
			 bool,
			 int, int, int, int,
			 int&, int&,
			 int&, int&,
			 int&, int&);

  static void setVisible(Vec, Vec,
			 int, bool,
			 int&, int&,
			 int&, int&,
			 int&, int&);

  static void mergeTags(Vec bmin, Vec bmax,
			int tag1, int tag2, bool useTF,
			int&, int&,
			int&, int&,
			int&, int&);

  static void erodeConnected(int, int, int,
			     Vec, Vec, int,
			     int,
			     int&, int&,
			     int&, int&,
			     int&, int&);

  static void dilateConnected(int, int, int,
			      Vec, Vec, int,
			      int,
			      int&, int&,
			      int&, int&,
			      int&, int&,
			      bool);

  static void modifyOriginalVolume(Vec, Vec, int,
				   int&, int&,
				   int&, int&,
				   int&, int&);


 private :
  static int m_depth, m_width, m_height;
  static uchar *m_volData;
  static uchar *m_maskData;

  static QList<Vec> m_cPos;
  static QList<Vec> m_cNorm;

  static void getConnectedRegion(int, int, int,
				 int, int, int,
				 int, int, int,
				 int, bool,
				 MyBitArray&);

  static void shrinkwrapSlice(uchar*, int, int);

  static void getTransparentRegion(int, int, int,
				   int, int, int,
				   MyBitArray&);

  static void dilateBitmask(int, bool,
			    qint64, qint64, qint64,
			    MyBitArray&);
};

#endif
