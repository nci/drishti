#ifndef IMAGECAPTIONS_H
#define IMAGECAPTIONS_H

#include "imagecaptiongrabber.h"

class ImageCaptions
{
 public :
  ImageCaptions();
  ~ImageCaptions();

  bool grabsMouse();
  void addInMouseGrabberPool();

  int count() { return m_imageCaptions.count(); }

  QList<ImageCaptionObject> imageCaptions();

  void clear();
  void add(ImageCaptionObject);
  void add(Vec);
  void setImageCaptions(QList<ImageCaptionObject>);

  void draw(QGLViewer*, bool);
  void postdraw(QGLViewer*);

  bool keyPressEvent(QKeyEvent*);

 private :
  QList<ImageCaptionGrabber*> m_imageCaptions;
  GLuint m_imageTex;
};

#endif
