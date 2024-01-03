#ifndef VRMENU_H
#define VRMENU_H

#include "menu01.h"

#include <QImage>
#include <QMatrix4x4>
#include <QVector3D>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;


class VrMenu : public QObject
{
  Q_OBJECT

 public :
  VrMenu();
  ~VrMenu();

  void generateHUD(QMatrix4x4);
  void reGenerateHUD();

  void draw(QMatrix4x4, QMatrix4x4, bool);

  void setImage(QImage);

  int checkOptions(QMatrix4x4, QMatrix4x4, int);

  QVector3D pinPoint();

  void setCurrentMenu(QString);

  QStringList menuList() { return m_menus.keys(); }

  bool pointingToMenu();

  void setValue(QString, float);
  float value(QString);
  
 signals :
  void resetModel();
  void updateScale(int);
  void updateSoftShadows(bool);
  void updateEdges(bool);

  void toggle(QString, float);
  
 private :
  QMap<QString, QObject*> m_menus;
  QString m_currMenu;
};

#endif
