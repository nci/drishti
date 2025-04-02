#ifndef CROPS_H
#define CROPS_H

#include "cropgrabber.h"

class Crops : public QObject
{
 Q_OBJECT

 public :
  Crops();
  ~Crops();

  bool checkCrop(Vec);
  
  bool show(int);
  void setShow(int, bool);
  void show();
  void hide();

  bool isInMouseGrabberPool(int);
  void addInMouseGrabberPool();
  void addInMouseGrabberPool(int);
  void removeFromMouseGrabberPool();
  void removeFromMouseGrabberPool(int);

  bool grabsMouse();

  void clear();

  QList<CropObject> crops();
  void setCrops(QList<CropObject>);

  bool tearPresent();
  bool viewPresent();
  bool cropPresent();
  bool glowPresent();

  int count();
  void addCrop(QList<Vec>);
  void addTear(QList<Vec>);
  void addView(QList<Vec>);
  void addDisplace(QList<Vec>);
  void addGlow(QList<Vec>);

  void addCrop(CropObject);
  void addCrop(QString);

  void draw(QGLViewer*, bool);
  void postdraw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

  void updateScaling();
  bool updated();
  
 signals :
  void showMessage(QString, bool);
  void mopCrop(int);
  void updateGL();
  void updateShaders();

 private slots :
  void selectForEditing(int, int);
  void deselectForEditing();

 private :
  QList<CropGrabber*> m_crops;
  bool m_sameForAll;

  void makeCropConnections();
  void processCommand(int, QString);
};


#endif
