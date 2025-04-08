#ifndef CROPS_H
#define CROPS_H

#include "cropgrabber.h"

class Crops : public QObject
{
 Q_OBJECT

 public :
  Crops();
  ~Crops();

  void collectCropInfoBeforeCheckCropped();
  bool checkCrop(Vec);
  bool checkCropped(Vec, int);
  
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

  Vec cc_pts[10];
  float cc_plen[5];
  float cc_radX[10];
  float cc_radY[10];
  Vec cc_tang[5];
  Vec cc_xaxis[5];
  Vec cc_yaxis[5];
  int cc_cropType[5];
  int cc_keepEnds[5];
  int cc_keepInside[5];

  
  void makeCropConnections();
  void processCommand(int, QString);
};


#endif
