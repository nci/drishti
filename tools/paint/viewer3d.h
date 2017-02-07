#ifndef VIEWER3D_H
#define VIEWER3D_H

#include "viewer.h"

#include <QPushButton>

class Viewer3D : public QFrame
{
  Q_OBJECT

 public :
  Viewer3D(QWidget *);
  
  Viewer* viewer() {return m_viewer;};

  void setLarge(bool);
  bool enlarged() { return m_maximized; }

 signals :
  void changeLayout();

 private :
  QCheckBox *m_raycast;
  Viewer *m_viewer;

  QPushButton *m_changeLayout;

  bool m_maximized;
};

#endif
