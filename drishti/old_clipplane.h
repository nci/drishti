#ifndef CLIPPLANES_H
#define CLIPPLANES_H

#include "clipper.h"
#include "clipinformation.h"

class ClipPlanes : public QObject
{
  Q_OBJECT

 public :
  ClipPlanes();
  ~ClipPlanes();

  ClipInformation clipInfo();
  void reset();
  bool updateFlag();
  void resetUpdateFlag();

  int count() { return m_clipList.count(); }

  bool isInMouseGrabberPool(int);
  void addInMouseGrabberPool();
  void addInMouseGrabberPool(int);
  void removeFromMouseGrabberPool();
  void removeFromMouseGrabberPool(int);

  void activate();
  void deactivate();

 signals :
  void showMessage(QString, bool);
  void addClipper();
  void removeClipper(int);

 public slots :
  void update();
  void setBounds(Vec, Vec);
  void predraw();
  void draw();
  void postdraw(QGLViewer*);
  bool keyPressEvent(QKeyEvent*);
  void addClipplane();
  void set(ClipInformation);

 private :
  bool m_updateFlag;
  Vec m_dataMin, m_dataMax;
  QList<Clipper*> m_clipList;
};

#endif
