#ifndef VIEWER_H
#define VIEWER_H

#include "glewinitialisation.h"

#include <QObject>
#include <QUdpSocket>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <QDockWidget>
#include <QFrame>

#include "drawhiresvolume.h"
#include "keyframe.h"
#ifdef USE_GLMEDIA
#include "glmedia.h"
#endif // USE_GLMEDIA
#include "messagedisplayer.h"

typedef void (Viewer::*MenuViewerFncPtr)();

class ViewerUndo
{
 public :
  ViewerUndo();
  ~ViewerUndo();
  
  void clear();
  void append(Vec, Quaternion);

  void undo();
  void redo();

  Vec pos();
  Quaternion rot();

 private :
  int m_index;
  QList<Vec> m_pos;
  QList<Quaternion> m_rot;

  void clearTop();
};

class Viewer : public QGLViewer
{
  Q_OBJECT

 public :

  Viewer(QWidget *parent=0);
  ~Viewer();


  void GlewInit();

  void dummydraw();

  void createImageBuffers();

  void setFieldOfView(float);
  void setHiresVolume(DrawHiresVolume*);
  void setKeyFrame(KeyFrame*);

  QGLFramebufferObject *imageBuffer() { return m_imageBuffer; }
  bool savingImages() { return (m_saveSnapshots || m_saveMovie); }
  bool drawToFBO();
  void setImageSize(int, int);
  void imageSize(int&, int&);

  void setUseFBO(bool);

  QMap<QString, QMap<QString, MenuViewerFncPtr> > registerMenuFunctions();

  bool paintMode() { return m_paintMode; }
  float paintRadius() { return m_paintRad; }
  Vec paintColor() { return m_paintColor; }
  int paintStyle() { return m_paintStyle; }
  float paintAlpha() { return m_paintAlpha; }
  int paintOctave() { return m_paintOctave; }
  int paintRoughness() { return m_paintRoughness; }
			 
 public slots :
   void setDOF(int, float);
   void displayMessage(QString, bool);
   void showFullScene();
   void updateScaling();

   virtual void resizeGL(int, int);
   virtual void paintGL();  
   virtual void keyPressEvent(QKeyEvent*);   
   virtual void enterEvent(QEvent*);
   virtual void leaveEvent(QEvent*);
   virtual void wheelEvent(QWheelEvent*);
   virtual void mousePressEvent(QMouseEvent*);
   virtual void mouseReleaseEvent(QMouseEvent*);
   virtual void mouseMoveEvent(QMouseEvent*);
   virtual void closeEvent(QCloseEvent*);
   
   void updateLookFrom(Vec, Quaternion);
   
   void checkPointSelected(const QMouseEvent*);
   void switchToHires();
   void switchDrawVolume();
   void setKeyFrame(int);
   void captureKeyFrameImage(int);
   void setSaveSnapshots(bool);
   void setSaveMovie(bool);
   void setCurrentFrame(int);
   void setImageMode(int);
   void endPlay();
   void setImageFileName(QString);
   
   bool startMovie(QString, int, int, bool);
   bool endMovie();
   
   
   void grabScreenShot();
   
   void readSocket();
   
   //------------
   //menu viewer functions
   void scaleBar();
   void caption();
   void addPoint();
   void removePoints();
   void path();
   void clip();
  //------------



  void setPaintMode(bool);
  void setPaintColor(Vec);
  void setPaintStyle(int p) { m_paintStyle = p; }
  void setPaintRadius(double r) { m_paintRad = r; }
  void setPaintAlpha(double b) { m_paintAlpha = b; }
  void setPaintOctave(int b) { m_paintOctave = b; }
  void setPaintRoughness(QString);
  
  void setPointMode(bool b) { m_pointMode = b; }
  void measureLength();
  
 signals:
  void quitDrishti();
  void allGood(bool);
  void showMessage(QString, bool);
  void setHiresMode(bool);
  void setKeyFrame(Vec, Quaternion, int, QImage);
  void replaceKeyFrameImage(int, QImage);
  void switchBB();
  void switchAxis();

  void addRotationAnimation(int, float, int);
  void moveToKeyframe(int);

  void addDockFrame(QString, QFrame*);


  void loadSurfaceMesh(QString);
  
  
 protected :  
  virtual void draw();
  virtual void fastDraw();
  virtual void init();

 private :
  QWidget *m_parent;

  ViewerUndo m_undo;

  QString m_commandString;

  int m_drawVolumeType;
  DrawHiresVolume *m_hiresVolume;

  KeyFrame *m_keyFrame;

  GLuint m_paintTex;

  int m_currFrame;
  bool m_useFBO;
  bool m_saveSnapshots;
  QString m_imageFileName;

  int m_imageMode;
  bool m_saveMovie;
#ifdef USE_GLMEDIA
  glmedia_movie_writer_t m_movieWriterLeft;
  glmedia_movie_writer_t m_movieWriterRight;
#endif // USE_GLMEDIA
  unsigned char *m_movieFrame;

  MessageDisplayer *m_messageDisplayer;

  bool m_imageSizeFlag;
  int m_origWidth;
  int m_origHeight;
  int m_imageWidth;
  int m_imageHeight;
  QGLFramebufferObject *m_imageBuffer;

  //GLhandleARB m_blurShader;
  //GLint m_blurParm[5];
  //GLhandleARB m_copyShader;
  //GLint m_copyParm[5];

  bool m_mouseDrag;
  bool m_updatePruneBuffer;
  QPoint m_mousePressPos;
  QPoint m_mousePrevPos;

  uchar *m_backBufferImage;
  int m_backBufferWidth, m_backBufferHeight;


  QTimer m_autoUpdateTimer;

  QUdpSocket *m_listeningSocket;
  int m_socketPort;


  bool m_paintMode;
  Vec m_paintColor;
  float m_paintRad;
  int m_paintStyle;
  float m_paintAlpha;
  int m_paintOctave;
  int m_paintRoughness;
  float m_unitPaintRad;

  GLuint m_glVA;
  GLuint m_glAB;
  GLuint m_glEAB;

  QList<int> m_zeroEventKeys;

  bool m_pointMode;
  bool m_lengthMode;
  bool m_gotPoint0;
  Vec m_point0;
  Vec m_point1;


  QRect m_selectionWindow;
  
  void initSocket();
  void processSocketData(QString);

  void renderVolume(int);
  void drawInHires(int);
  void drawImageOnScreen();
  void saveMovie();
  void saveImage();
  void saveMonoImage(QString, QChar, int);
  void saveStereoImage(QString, QChar, int);
  void saveCubicImage(QString, QChar, int);
  void save360Image(QString, QChar, int);

  void processCommand(QString);

  QImage getSnapshot();
  void saveSnapshot(QString);
  void fboToMovieFrame();
  void screenToMovieFrame();

  void setWidgetSizeToImageSize();
  void restoreOriginalWidgetSize();

  void splashScreen();
  bool bindFBOs(int);
  void releaseFBOs(int);

  //void createBlurShader();
  //void createCopyShader();

  void grabBackBufferImage();
  void showBackBufferImage();

  void undoParameters();
  void showHelp();


  void showMenuFunctionHelp(QString);

  void drawSelectionWindow();
};

#endif
