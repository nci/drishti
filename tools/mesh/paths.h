#ifndef PATHS_H
#define PATHS_H

#include "pathgrabber.h"

class Paths : public QObject
{
 Q_OBJECT

 public :
  Paths();
  ~Paths();

  bool isInMouseGrabberPool(int);
  void addInMouseGrabberPool();
  void addInMouseGrabberPool(int);
  void removeFromMouseGrabberPool();
  void removeFromMouseGrabberPool(int);

  bool grabsMouse();

  void clear();

  QList<PathGrabber*> pathsPointer() { return m_paths; }
  QList<PathObject> paths();
  void setPaths(QList<PathObject>);

  int count();
  void addPath(QList<Vec>);
  void addPath(QList<Vec>, Vec);
  void addPath(PathObject);
  void addPath(QString);

  void addFibers(QString);

  void draw(QGLViewer*,
	    Vec, float, float,
	    bool, Vec);
  void postdraw(QGLViewer*);

  void postdrawInViewport(QGLViewer*,
			  QVector4D, Vec, Vec, int);

  bool keyPressEvent(QKeyEvent*);

  void updateScaling();

  PathGrabber* checkIfGrabsMouse(int, int, Camera*);
  void mousePressEvent(QMouseEvent*, Camera*);
  void mouseMoveEvent(QMouseEvent*, Camera*);
  void mouseReleaseEvent(QMouseEvent*, Camera*);

  bool continuousAdd();
  void addPoint(Vec);

  int inViewport(int, int, int, int);
  bool viewportsVisible();
  int viewportGrabbed();
  void resetViewportGrabbed();
  void setViewportGrabbed(int, bool);
  void drawViewportBorders(QGLViewer*);
  bool viewportKeypressEvent(int, QKeyEvent*,
			     QPoint, Vec, int, int);
  void modThickness(bool,
		    int, int,
		    QPoint, Vec, int, int);
  void translate(int, int, int,
		 QPoint, Vec, int, int);
  void rotate(int, int,
		 QPoint, Vec, int, int);

 signals :
  void showMessage(QString, bool);
  void updateGL();

  void addToCameraPath(QList<Vec>,QList<Vec>,QList<Vec>,QList<Vec>);

 private slots :
  void selectForEditing(int, int);
  void deselectForEditing();

 private :
  QList<PathGrabber*> m_paths;
  bool m_sameForAll;

  void makePathConnections();
  void processCommand(int, QString);

  void addIndexedPath(QString);

  void loadCaption(int);

  bool openPropertyEditor(int);

  void getRequiredParameters(int,
			     Vec,
			     int, int,
			     int&, int&, int&, int&,
			     int&, int&,
			     int&, float&);
  int getPointPressed(int, Vec,
		      QList<Vec>,
		      QPoint,
		      int, int, int, float);

};


#endif
