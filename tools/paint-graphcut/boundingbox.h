#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include <QKeyEvent>
#include "mymanipulatedframe.h"

class BoundingBox : public QObject {
 Q_OBJECT

 public :
  BoundingBox();
  ~BoundingBox();
  
  void activateBounds();
  void deactivateBounds();

  void setPositions(Vec, Vec);
  void setBounds(Vec, Vec);
  void bounds(Vec&, Vec&);
  void draw();
  bool keyPressEvent(QKeyEvent*);

 signals :
  void updated();

 private slots :
  void update();

 private :
  MyManipulatedFrame m_bounds[6];
  Vec m_dataMin, m_dataMax;
  Vec boxColor, defaultColor, selectColor;

  void drawX(float, Vec, Vec, Vec);
  void drawY(float, Vec, Vec, Vec);
  void drawZ(float, Vec, Vec, Vec);

  void updateMidPoints();
};

#endif
