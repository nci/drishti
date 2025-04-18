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
			      int&, int&,
			      int, float, float);

  static void smoothConnectedRegion(int, int, int,
				    Vec, Vec,
				    int,
				    int&, int&,
				    int&, int&,
				    int&, int&,
				    int, float, float,
				    int);

  static void smoothAllRegion(Vec, Vec,
			      int,
			      int&, int&,
			      int&, int&,
			      int&, int&,
			      int, float, float,
			      int);
  
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
			 int&, int&,
			 int, float, float);

  static void tagTubes(Vec, Vec, int,
		       bool,
		       int, int, int, int,
		       int&, int&,
		       int&, int&,
		       int&, int&,
		       int, float, float);

  static void setVisible(Vec, Vec,
			 int, bool,
			 int&, int&,
			 int&, int&,
			 int&, int&,
			 int, float, float);

  static void stepTags(Vec, Vec,
		       int, int,
		       int&, int&,
		       int&, int&,
		       int&, int&);
  
  static void mergeTags(Vec bmin, Vec bmax,
			int tag1, int tag2, bool useTF,
			int&, int&,
			int&, int&,
			int&, int&);

  static void erodeAll(Vec, Vec, int,
		       int,
		       int&, int&,
		       int&, int&,
		       int&, int&,
		       int, float, float);  
  static void erodeConnected(int, int, int,
			     Vec, Vec, int,
			     int,
			     int&, int&,
			     int&, int&,
			     int&, int&,
			     int, float, float);

  static void dilateAll(Vec, Vec, int,
			int,
			int&, int&,
			int&, int&,
			int&, int&,
			bool,
			int, float, float);
  static void dilateConnected(int, int, int,
			      Vec, Vec, int,
			      int,
			      int&, int&,
			      int&, int&,
			      int&, int&,
			      bool,
			      int, float, float);

  static void modifyOriginalVolume(Vec, Vec, int,
				   int&, int&,
				   int&, int&,
				   int&, int&);


  static void bakeCurves(uchar*,
			 int, int,
			 int, int,
			 int, int,
			 int,
			 int, float, float);
			 

  static float calcGrad(int, qint64, qint64, qint64,
			int, int, int,
			uchar*, ushort*);

  static bool checkClipped(Vec);

  static void genVisibilityMap(int, float, float);
  static MyBitArray* getVisibilityMap() { return &m_visibilityMap; }
  
 private :
  static int m_depth, m_width, m_height;
  static uchar *m_volData;
  static ushort *m_volDataUS;
  static uchar *m_maskData;

  static MyBitArray m_visibilityMap;
  
  static QList<Vec> m_cPos;
  static QList<Vec> m_cNorm;

  static void getConnectedRegion(int, int, int,
				 int, int, int,
				 int, int, int,
				 int, bool,
				 MyBitArray&,
				 int, float, float);

  static void shrinkwrapSlice(uchar*, int, int);

  static void dilateBitmask(int, bool,
			    qint64, qint64, qint64,
			    MyBitArray&);
  static void dilateBitmaskUsingVDB(int, bool,
				    qint64, qint64, qint64,
				    MyBitArray&);

  static void getVisibleRegion(int, int, int,
			       int, int, int,
			       int, bool,
			       int, float, float,
			       MyBitArray&);  
  static void parVisibleRegionGeneration(QList<QVariant>);

  
  static void getTransparentRegion(int, int, int,
				   int, int, int,
				   MyBitArray&,
				   int, float, float);
  static void parTransparentRegionGeneration(QList<QVariant>);  

  
  static void bakeC(int, int, int,
		    int, int, int,
		    int,
		    int, float, float,
		    uchar*);
  static void parBakeCurves(QList<QVariant>);


  static void resetT(int, int, int,
		     int, int, int,
		     int);
  static void parResetTag(QList<QVariant>);

  
  static void writeToMask(int, int, int,
			  int, int, int,
			  int,
			  int, float, float,
			  bool,
			  MyBitArray&);
  static void parWriteToMask(QList<QVariant>);

  
  static void convertToVDBandSmooth(int, int, int,
				    int, int, int,
				    MyBitArray&,
				    int);
  
};

#endif
