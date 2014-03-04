#ifndef IMAGECAPTIONS_H
#define IMAGECAPTIONS_H

#include "imagecaptiongrabber.h"

class ImageCaptions : public QObject
{
 Q_OBJECT

 public :
  ImageCaptions();
  ~ImageCaptions();

  void setActive(bool);
  bool isActive();

  bool grabsMouse();
  void addInMouseGrabberPool();

  int count() { return m_imageCaptions.count(); }

  QList<ImageCaptionObject> imageCaptions();

  void clear();
  void add(ImageCaptionObject);
  void add(Vec);
  void setImageCaptions(QList<ImageCaptionObject>);

  void draw(QGLViewer*, bool);

  bool keyPressEvent(QKeyEvent*);

 public slots :
  void activated();
 
 private :
  QList<ImageCaptionGrabber*> m_imageCaptions;

  void updateConnections();
};

#endif
