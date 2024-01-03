#ifndef CAPTIONWIDGET_H
#define CAPTIONWIDGET_H

#include <GL/glew.h>

#include <QMatrix4x4>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class CaptionWidget : public QObject
{
  Q_OBJECT
  
 public :
  CaptionWidget();
  ~CaptionWidget();

  void generateCaption(QString, QString);
  void setText(QString);
  
  void draw(QMatrix4x4, QMatrix4x4);

  void blink(int);
  void blinkAndHide(int);
  
  QString name() { return m_name; }
  QString text() { return m_caption; }
  
  static void setText(QString, QString);
  static void blink(QString, int);
  static void blinkAndHide(QString, int);
  static QList<CaptionWidget*> widgets;  
  static CaptionWidget* getWidget(QString);
  
 private :
  GLuint m_glVA;
  GLuint m_glVB;
  GLuint m_glIB;

  GLuint m_glTexture;
  int m_texWd, m_texHt;
  QMatrix4x4 m_captionMat;
  
  QString m_name;
  QString m_caption;
  int m_fontSize;
  Vec m_color;

  int m_blink;
  bool m_hideAfterBlink;
};

#endif
