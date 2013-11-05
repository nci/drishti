#ifndef MESHGENERATOR_H
#define MESHGENERATOR_H

#include <QtGui>
#include "marchingcubes.h"
#include "volumefilemanager.h"
#include "ply.h"
#include "dcolordialog.h"
#include "propertyeditor.h"
#include "cropobject.h"
#include "pathobject.h"

#include <QGLViewer/vec.h>
using namespace qglviewer;

enum ColorType
  {
    _FixedColor = 0,
    _NormalColor,
    _PositionColor,
    _OcclusionColor,
    _LutColor,
    _VRLutColor
  };


class MeshGenerator
{
 public :
  MeshGenerator();
  ~MeshGenerator();

  QString start(VolumeFileManager*,
		int, int, int,
		Vec, Vec,
		QString,
		Vec, int,
		QList<Vec>,
		QList<Vec>,
		QList<CropObject>,
		QList<PathObject>,
		uchar*,
		int, int, int, int,
		QVector<uchar>,
		QVector<uchar>);

 private :
  QTextEdit *m_meshLog;
  QProgressBar *m_meshProgress;

  VolumeFileManager *m_vfm;
  int m_voxelType;
  int m_nX, m_nY, m_nZ;
  int m_depth, m_width, m_height;
  Vec m_dataMin, m_dataMax, m_dataSize;
  int m_samplingLevel;

  bool m_blendPresent;
  bool m_tearPresent;
  bool m_cropPresent;
  bool m_pathBlendPresent;
  bool m_pathCropPresent;
  QList<CropObject> m_crops;
  QList<PathObject> m_paths;

  int m_pruneX, m_pruneY, m_pruneZ;
  float m_pruneLod;
  QVector<uchar> m_pruneData;

  bool m_useTagColors;
  QVector<uchar> m_tagColors;

  float m_scaleModel;

  void generateMesh(int, int,
		    QString,
		    int, int,
		    QGradientStops,
		    int, int,
		    bool, bool,
		    Vec,
		    QList<Vec>,
		    QList<Vec>,
		    QList<CropObject>,
		    QList<PathObject>,
		    bool, bool, uchar*,
		    int, bool);

  void saveMesh(QString, int, int, int, bool);

  bool getValues(int&, float&,
		 int&, int&, int&, int&,
		 bool&, bool&, bool&, bool&,
		 QGradientStops&,
		 bool, int&, bool&);

  float getOcclusionFraction(uchar*,
			     int, int, int,
			     uchar,
			     QVector3D,
			     QVector3D,
			     bool);

  float getOcclusionFractionSAT(int*,
				int, int, int,
				QVector3D,
				QVector3D,
				bool);

  QColor getOcclusionColor(int*,
			   int, int, int,
			   uchar,
			   QVector3D,
			   QVector3D,
			   QGradientStops,
			   bool);

  QColor getLutColor(uchar*,	  
		     int*,
		     int, int, int, int,
		     uchar,
		     QVector3D,
		     QVector3D,
		     QGradientStops,
		     bool, bool);

  QColor getLutColor(uchar*,	  
		     int*,
		     int, int, int, int,
		     uchar,
		     QVector3D,
		     QVector3D,
		     uchar*,
		     bool,
		     QVector3D,
		     bool);

  QGradientStops resampleGradientStops(QGradientStops);

  void smoothData(uchar*,
		  int, int, int, int);
  void genSAT(int*,
	      int, int, int, int);

  void applyTear(int, int, int,
		 uchar*, uchar*, bool);

  bool checkPathCrop(Vec);
  bool checkPathBlend(Vec, ushort, uchar*);
  bool checkCrop(Vec);
  bool checkBlend(Vec, ushort, uchar*);
  void applyOpacity(int, uchar*, uchar*, uchar*);
};

#endif MESHGENERATOR_H
