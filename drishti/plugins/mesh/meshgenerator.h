#ifndef MESHGENERATOR_H
#define MESHGENERATOR_H

#include "volumefilemanager.h"
#include "ply.h"
#include "propertyeditor.h"
#include "cropobject.h"
#include "pathobject.h"

#include <QGLViewer/vec.h>
using namespace qglviewer;



enum ColorType
  {
    _FixedColor = 0,
    _LutColor,
    _NormalColor,
    _PositionColor,
    _OcclusionColor,
    _VRLutColor
  };


class MeshGenerator
{
 public :
  MeshGenerator();
  ~MeshGenerator();

  QList<char *> plyStrings;

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
		QVector<uchar>,
		bool);

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

  bool m_batchMode;
  
  
  void generateMesh(int, int,
		    QString,
		    int, int,
		    QGradientStops,
		    int,
		    Vec,
		    QList<Vec>,
		    QList<Vec>,
		    QList<CropObject>,
		    QList<PathObject>,
		    int,
		    uchar*,
		    int, bool, float,
		    bool);

  bool getValues(int&, float&,
		 int&, int&, int&,
		 int&,
		 QGradientStops&,
		 int&, bool&,
		 float&,
		 bool);

  QColor getLutColor(uchar*,	  
		     int, int, int,
		     uchar,
		     QVector3D,
		     QVector3D,
		     uchar*,
		     QVector3D,
		     bool);

  QGradientStops resampleGradientStops(QGradientStops);


  void applyTear(int, int,
		 uchar*, uchar*, bool);

  bool checkPathCrop(Vec);
  bool checkPathBlend(Vec, ushort, uchar*);
  bool checkCrop(Vec);
  bool checkBlend(Vec, ushort, uchar*);
  void applyOpacity(int, uchar*, uchar*, uchar*,
		    uchar*, uchar*, uchar*);
};

#endif MESHGENERATOR_H
