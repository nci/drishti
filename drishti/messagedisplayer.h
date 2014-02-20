#ifndef MESSAGEDISPLAYER_H
#define MESSAGEDISPLAYER_H

#include <QObject>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class Viewer;

class MessageDisplayer : public QObject
{
 Q_OBJECT

 public :
  MessageDisplayer(Viewer*);
  ~MessageDisplayer();

  bool renderScene();
  bool showingMessage();

 signals :
  void updateGL();

 public slots :
  void refreshGL();
  void holdMessage(QString, bool);
  void renderMessage(QSize);
  void turnOffMessage();
  void drawMessage(QSize);

 private :
  Viewer *m_Viewer;
  QString m_message;
  bool m_mesgShowing;
  bool m_firstTime;
  bool m_warning;
  int m_mesgShift;
  GLubyte *m_mesgImage;

};

#endif
