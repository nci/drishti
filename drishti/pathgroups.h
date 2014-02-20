#ifndef PATHGROUPS_H
#define PATHGROUPS_H

#include "pathgroupgrabber.h"
#include "cropobject.h"

class PathGroups : public QObject
{
 Q_OBJECT

 public :
  PathGroups();
  ~PathGroups();

  bool isInMouseGrabberPool(int);
  void addInMouseGrabberPool();
  void addInMouseGrabberPool(int);
  void removeFromMouseGrabberPool();
  void removeFromMouseGrabberPool(int);

  bool grabsMouse();

  void clear();

  QList<PathGroupObject> paths();
  void setPaths(QList<PathGroupObject>);

  bool checkForMultiplePaths(QString);

  int count();
  void addPath(QList<Vec>);
  void addPath(PathGroupObject);
  void addPath(QString);
  void addVector(QString);

  void predraw(QGLViewer*, bool,
	       QList<Vec>,
	       QList<Vec>,
	       QList<CropObject>);
  void draw(QGLViewer*, bool, Vec, bool);
  void postdraw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

  void updateScaling();

 signals :
  void showMessage(QString, bool);
  void updateGL();
  void showProfile(int, int, QList<Vec>);

 private slots :
  void selectForEditing(int, int);
  void deselectForEditing();

 private :
  QList<PathGroupGrabber*> m_paths;
  bool m_sameForAll;

  void makePathConnections();
  void processCommand(int, QString);

  void addIndexedPath(QString);
};


#endif
