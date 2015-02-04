#ifndef LANDMARKS_H
#define LANDMARKS_H

#include <QGLViewer/manipulatedCameraFrame.h>
#include <QGLViewer/mouseGrabber.h>

#include "landmarkinformation.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QSpinBox>
#include <QLineEdit>

class Landmarks : public QObject, public MouseGrabber
{
 Q_OBJECT

 public :
  Landmarks();
  ~Landmarks();
  
    enum MoveAxis
  {
    MoveX,
    MoveY,
    MoveZ,
    MoveAll
  };

  void setLandmarkInfo(LandmarkInformation);
  LandmarkInformation getLandmarkInfo();

  void setPoints(QList<Vec>);
  void addPoints(QList<Vec>);
  void setPoint(int, Vec);

  Vec point(int);
  QList<Vec> points();

  void setPointSize(int);
  int pointSize();

  void setPointColor(Vec);
  Vec pointColor();

  void setTextColor(Vec);
  Vec textColor();

  void setTextSize(int);
  int textSize();

  QList< QList<int> > distances();
  void setDistances(QList< QList<int> >);
  void setDistances(QString);

  QList< QList<int> > angles();
  void setAngles(QList< QList<int> >);
  void setAngles(QString);

  QList< QList<float> > projectOnLine();
  void setProjectOnLine(QList< QList<float> >);
  void setProjectOnLine(QString);

  QList<float> projectOnPlane();
  void setProjectOnPlane(QList<float>);
  void setProjectOnPlane(QString);

  int count();
  void clear();

  int moveAxis();
  void setMoveAxis(int);

  bool grabsMouse();
  
  void checkIfGrabsMouse(int, int, const Camera* const);
  void mousePressEvent(QMouseEvent* const, Camera* const);
  void mouseMoveEvent(QMouseEvent* const, Camera* const);
  void mouseReleaseEvent(QMouseEvent* const, Camera* const);

  bool keyPressEvent(QKeyEvent*);

  void draw(QGLViewer*, bool);
  void postdraw(QGLViewer*);

  void setMouseGrab(bool);
  void toggleMouseGrab();

 public slots :
  void reorderLandmarks(); 
  void updateLandmarks(int, int); 
  void saveLandmarks();
  void loadLandmarks();
  void loadLandmarks(QString);
  void clearAllLandmarks();
  void changePointColor();
  void changePointSize(int);
  void changeTextColor();
  void changeTextSize(int);
  void updateDistances();
  void updateAngles();
  void updateProjectOnLine();
  void updateProjectOnPlane();

 signals :
  void updateGL();

 private :
  int m_moveAxis;
  int m_grabbed;
  int m_pressed;
  QPoint m_prevPos;

  bool m_grab;

  Vec m_textColor;
  int m_textSize;

  Vec m_pointColor;
  int m_pointSize;

  QList<Vec> m_points;
  QList<QString> m_text;

  bool m_showCoordinates;
  bool m_showText;
  bool m_showNumber;

  QWidget *m_table;
  QTableWidget *m_tableWidget;

  QSpinBox *m_pspinB;
  QSpinBox *m_tspinB;

  QLineEdit *m_distEdit;
  QLineEdit *m_angleEdit;
  QLineEdit *m_projOnLineEdit;
  QLineEdit *m_projOnPlaneEdit;

  QList< QList<int> > m_distances;
  QList< QList<int> > m_angles;

  QList< QList<float> > m_projectLineDistances;
  QList<int> m_projectLinePoints;

  QList<float> m_projectPlanePoints;
  
  void updateTable();
  void postdrawLength(QGLViewer*);

  void drawProjectOnLine(QGLViewer*);
  void postdrawLineProjection(QGLViewer*);
  void postdrawProjectedLineLength(QGLViewer*);

  void drawProjectOnPlane(QGLViewer*);
  void postdrawPlaneProjection(QGLViewer*);
  void postdrawProjectedPlaneLength(QGLViewer*);
};




#endif
