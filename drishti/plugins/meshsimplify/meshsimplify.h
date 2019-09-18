#ifndef MESHSIMPLIFY_H
#define MESHSIMPLIFY_H

#include "commonqtclasses.h"
#include <QProgressBar>
#include <QTextEdit>
#include <QInputDialog>
#include <QFileDialog>

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
} PlyFace;

typedef struct
{
  float  x,  y,  z ;  /**< Vertex coordinates */
  float nx, ny, nz ;  /**< Vertex normal */
  uchar r, g, b;
} PlyVertex ;

class MeshSimplify
{
 public :
  MeshSimplify();
  ~MeshSimplify();

  QString start(QString);

 private :
  QTextEdit *m_meshLog;
  QProgressBar *m_meshProgress;

  int m_nverts,m_nfaces;
  PlyVertex **m_vlist;
  PlyFace **m_flist;
  float *vcolor;

  float m_scaleModel;

  QList<char*> plyStrings;

  void simplifyMesh(QString, QString,
		    float, int);

  void savePLY(QString);
  bool loadPLY(QString);

  bool getValues(float&, int&);
};

#endif MESHSIMPLIFY_H
