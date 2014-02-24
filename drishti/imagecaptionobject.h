#ifndef IMAGECAPTIONOBJECT_H
#define IMAGECAPTIONOBJECT_H

#include <fstream>
using namespace std;

#include "commonqtclasses.h"

class ImageCaptionObject
{
 public :
  ImageCaptionObject();
  ~ImageCaptionObject();
  
  void load(fstream&);
  void save(fstream&);

  void clear();

  bool hasCaption(QStringList);

  void set(Vec, QString);
  void setImageCaption(ImageCaptionObject);
  void setPosition(Vec);
  void setImageFileName(QString);
  void setActive(bool b) { m_active = b; }
  void toggleActive() { m_active = !m_active; }
  
  QImage image() { return m_image; }
  QString imageFile() { return m_imageFile; }
  Vec position() { return m_pos; }
  int height() { return m_height; }
  int width() { return m_width; }
  bool active() { return m_active; }

 private :
  QImage m_image;
  Vec m_pos;
  QString m_imageFile;
  int m_height, m_width;
  bool m_active;

  void loadImage();
};

#endif
