#ifndef MESHGENERATOR_H
#define MESHGENERATOR_H

#include <QtGui>
#include "volumefilemanager.h"
#include "ply.h"
#include "dcolordialog.h"
#include "propertyeditor.h"
#include "cropobject.h"
#include "pathobject.h"

#include <QGLViewer/vec.h>
using namespace qglviewer;

typedef struct
{
  unsigned char nverts;    /* number of Vertex indices in list */
  int *verts;              /* Vertex index list */
} Face;

typedef struct
{
  float  x,  y,  z ;  /**< Vertex coordinates */
  float nx, ny, nz ;  /**< Vertex normal */
  uchar r, g, b;
} Vertex ;

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

  int m_nverts,m_nfaces;
  Vertex **m_vlist;
  Face **m_flist;
  float *vcolor;

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

  void generateMesh(int,
		    QStringList,
		    QString,
		    int,
		    QGradientStops,
		    int, bool, bool,
		    Vec,
		    QList<Vec>,
		    QList<Vec>,
		    QList<CropObject>,
		    QList<PathObject>,
		    uchar*,
		    int, bool);

  void savePLY(QString);
  bool loadPLY(QString);

  bool getValues(int&, int&,
		 bool&, bool&,
		 QGradientStops&,
		 bool, int&, bool&);

  QColor getVRLutColor(uchar*,	  
		       int, int, int,
		       QVector3D,
		       QVector3D,
		       uchar*,
		       bool,
		       QVector3D);

  QGradientStops resampleGradientStops(QGradientStops);

  void applyTear(int, int, int,
		 uchar*, uchar*, bool);

  bool checkPathCrop(Vec);
  bool checkPathBlend(Vec, ushort, uchar*);
  bool checkCrop(Vec);
  bool checkBlend(Vec, ushort, uchar*);
};

#endif MESHGENERATOR_H
