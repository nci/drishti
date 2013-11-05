#ifndef CAMERAPATHNODE_H
#define CAMERAPATHNODE_H

#include <QtGui>
#include <QObject>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class CameraPathNode : public QObject
{
 Q_OBJECT

 public :
  CameraPathNode(Vec, Quaternion);
  
  Vec position();
  void setPosition(Vec);

  Quaternion orientation();
  void setOrientation(Quaternion);

  void draw(float);
  int keyPressEvent(QKeyEvent*, bool&);
  bool markedForDeletion();

 public slots :
  void nodeModified();

 signals :
  void modified();

 private :
  ManipulatedFrame *m_mf;
  bool m_markForDelete;
  AxisPlaneConstraint *m_constraints;
};

#endif
