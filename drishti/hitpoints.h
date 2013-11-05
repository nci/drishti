#ifndef HITPOINTS_H
#define HITPOINTS_H

#include "hitpointgrabber.h"

class HitPoints : public QObject
{
 Q_OBJECT

 public :
  HitPoints();
  ~HitPoints();

  bool grabsMouse();

  void addInMouseGrabberPool();
  void addInMouseGrabberPool(int i);

  void removeFromMouseGrabberPool();
  void removeFromMouseGrabberPool(int i);

  QList<Vec> points();
  QList<Vec> activePoints();

  QList<Vec> barePoints();

  void ignore(bool);

  int count();
  int bareCount();
  int activeCount();
  bool point(int, Vec&);
  bool activePoint(int, Vec&);

  void clear();
  void removeActive();
  void resetActive();

  void draw(QGLViewer*, bool);
  void postdraw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

  int pointSize();
  Vec pointColor();

  QList<Vec> renewValues();
  void setRawTagValues(QList<QVariant>, QList<QVariant>);

  HitPointGrabber* checkIfGrabsMouse(int, int, Camera*);
  void mousePressEvent(QMouseEvent*, Camera*);
  void mouseMoveEvent(QMouseEvent*, Camera*);
  void mouseReleaseEvent(QMouseEvent*, Camera*);

 signals :
  void sculpt(int, QList<Vec>, float, float, int);

 public slots :
  void add(Vec);
  void setPoints(QList<Vec>);
  void setBarePoints(QList<Vec>);
  void addPoints(QString);
  void addBarePoints(QString);
  void savePoints(QString);
  void removeBarePoints();
  void setPointSize(int);
  void setPointColor(Vec);
  void setMouseGrab(bool);
  void toggleMouseGrab();

 private slots :
  void updatePoint();

 private :
  bool m_ignore;
  QList<HitPointGrabber*> m_points;
  QList<QVariant> m_rawValues;
  QList<QVariant> m_tagValues;
  bool m_showPoints;
  bool m_showRawValues;
  bool m_showTagValues;
  bool m_showCoordinates;

  Vec m_pointColor;
  int m_pointSize;

  bool m_grab;

  QList<Vec> m_barePoints;

  void drawArrows(Vec, int);

  void updatePointDialog();
  void makePointConnections();
  void processCommand(int, QString);
};

#endif
