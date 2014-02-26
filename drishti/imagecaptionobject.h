#ifndef IMAGECAPTIONOBJECT_H
#define IMAGECAPTIONOBJECT_H

#include <fstream>
using namespace std;

#include "commonqtclasses.h"

#include <QTextEdit>

#include "videoplayer.h"

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
  void set(Vec, QString, int, int);
  void setImageCaption(ImageCaptionObject);
  void setPosition(Vec);
  void setImageFileName(QString);
  void setActive(bool);
  bool labelPresent() { return (m_label != 0); }
  
  void setCaptionPosition(int, int);
  void saveSize();

  QString imageFile() { return m_imageFile; }
  Vec position() { return m_pos; }
  int height();
  int width();
  bool active() { return m_active; }

 private :
  Vec m_pos;
  QString m_imageFile;
  int m_height, m_width;
  bool m_active;
  bool m_redo;

  QTextEdit *m_label;
  VideoPlayer *m_videoPlayer;
  
  void loadCaption();
};

#endif
