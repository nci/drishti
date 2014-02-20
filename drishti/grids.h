#ifndef GRIDS_H
#define GRIDS_H

#include "gridgrabber.h"

class Grids : public QObject
{
 Q_OBJECT

 public :
  Grids();
  ~Grids();

  bool isInMouseGrabberPool(int);
  void addInMouseGrabberPool();
  void addInMouseGrabberPool(int);
  void removeFromMouseGrabberPool();
  void removeFromMouseGrabberPool(int);

  bool grabsMouse();

  void clear();

  QList<GridObject> grids();
  void setGrids(QList<GridObject>);

  int count();
  void addGrid(QList<Vec>, int, int);
  void addGrid(GridObject);
  void addGrid(QString);

  void draw(QGLViewer*, bool, Vec);
  void postdraw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

  void updateScaling();
  void setPoints(int, QList<Vec>);

 signals :
  void showMessage(QString, bool);
  void updateGL();
  void gridStickToSurface(int, int, QList< QPair<Vec, Vec> >);

 private slots :
  void selectForEditing(int, int);
  void deselectForEditing();

 private :
  QList<GridGrabber*> m_grids;
  bool m_sameForAll;

  void makeGridConnections();
  void processCommand(int, QString);
};


#endif
