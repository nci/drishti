#ifndef CAPTIONS_H
#define CAPTIONS_H

#include "captiongrabber.h"

class Captions
{
 public :
  Captions();
  ~Captions();

  bool grabsMouse();

  QList<CaptionObject> captions();

  void clear();
  void add(CaptionObject);
  void setCaptions(QList<CaptionObject>);

  void draw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

 private :
  QList<CaptionGrabber*> m_captions;
  GLuint m_imageTex;
};

#endif
