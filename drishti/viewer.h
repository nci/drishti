#include "glewinitialisation.h"

#include <QObject>
#include <QUdpSocket>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <QInputDialog>
#include <QSpinBox>
#include <QDockWidget>

#include "drawhiresvolume.h"
#include "drawlowresvolume.h"
#include "keyframe.h"
#include "brickswidget.h"

#ifdef USE_GLMEDIA
#include "glmedia.h"
#endif // USE_GLMEDIA
#include "messagedisplayer.h"
#include "volume.h"
#include "popupslider.h"

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
  void setVolume(Volume*);
  void setHiresVolume(DrawHiresVolume*);
  void setLowresVolume(DrawLowresVolume*);
  void setKeyFrame(KeyFrame*);
  void setBricksWidget(BricksWidget*);
  
  QImage histogramImage1D();
  QImage histogramImage2D();

  unsigned char* lookupTable();

  QGLFramebufferObject *imageBuffer() { return m_imageBuffer; }
  QGLFramebufferObject *lowresBuffer() { return m_lowresBuffer; }
  bool savingImages() { return (m_saveSnapshots || m_saveMovie); }
  bool drawToFBO();
  void setImageSize(int, int);
  void imageSize(int&, int&);

  void setUseFBO(bool);

  void setVolDataPtr(VolumeFileManager*);

  QMap<QString, QMap<QString, MenuViewerFncPtr> > registerMenuFunctions();

  bool paintMode() { return m_paintMode; }
  float paintRadius() { return m_paintRad; }
  Vec paintColor() { return m_paintColor; }
  void setPaintStyle(int p) { m_paintStyle = p; }
  int paintStyle() { return m_paintStyle; }
  float paintAlpha() { return m_paintAlpha; }
			 
 public slots :
   void setTag(int);
   void setDOF(int, float);
   void setCarveRadius(int);
   void updateLightBuffers();
   void updatePruneBuffer(bool);
   void displayMessage(QString, bool);
   void showFullScene();
   void updateStereoSettings(float, float, float);
   void updateScaling();
   virtual void resizeGL(int, int);
   
   virtual void keyPressEvent(QKeyEvent*);
   
   virtual void enterEvent(QEvent*);
   virtual void leaveEvent(QEvent*);
   virtual void wheelEvent(QWheelEvent*);
   virtual void mousePressEvent(QMouseEvent*);
   virtual void mouseReleaseEvent(QMouseEvent*);
   virtual void mouseMoveEvent(QMouseEvent*);
   virtual void closeEvent(QCloseEvent*);

   void brickAngleFromMouse(bool);
  
   void updateLookFrom(Vec, Quaternion, float, float);
   
   void resetLookupTable();
   void loadLookupTable(QList<QImage>);
   void updateLookupTable(unsigned char*);
   void updateLookupTable();
   
   void checkPointSelected(const QMouseEvent*);
   void reloadData();
   void switchToHires();
   void switchDrawVolume();
   void enableTextureUnits();
   void disableTextureUnits();
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
   
   void currentView();
   void updateTagColors();
   
   void grabScreenShot();
   void processMorphologicalOperations();
   
   void readSocket();
   
   void setPaintColor();
   void setPaintRadius(double r) { m_paintRad = r; }
   void setPaintAlpha(double b) { m_paintAlpha = b; }
   void setPaintOctave(int b) { m_paintOctave = b; }

  //------------
   //menu viewer functions
   void reslice();
   void rescale();
   void image2volume();
   void changeSliceOrdering();
   void colorBar();
   void scaleBar();
   void caption();
   void setFloatPrecision();
   void getSurfaceArea();
   void getVolume();
   void addPoint();
   void removePoints();
   void mix();
   void mop();
   void interpolateVolumes();
   void path();
   void clip();
   void crop();
   void blend();
   void disect();
   void glow();
   void displace();
   void opmod();
   void showGiLightDialog();
   void setPaintMode(bool);
  //------------

 signals:
  void show16BitEditor(bool);
  void resetFlipImage();
  void quitDrishti();
  void showMessage(QString, bool);
  void stereoSettings(float, float, float);
  void focusSetting(float);
  void histogramUpdated(QImage, QImage);
  void histogramUpdated(int*);
  void setHiresMode(bool);
  void changeStill(int);
  void changeDrag(int); 
  void setView(Vec, Quaternion,
	       QImage, float);
  void setKeyFrame(Vec, Quaternion,
		   int, float, float,
		   unsigned char*,
		   QImage);
  void replaceKeyFrameImage(int, QImage);
  void switchBB();
  void switchAxis();
  void saveVolume();
  void maskRawVolume();
  void countIsolatedRegions();

  void searchCaption(QStringList);
  void addRotationAnimation(int, float, int);
  void generateMesh(bool);
  void processMops();
  void moveToKeyframe(int);

  void addDockFrame(QString, QFrame*);
  
 protected :  
  virtual void draw();
  virtual void fastDraw();
  virtual void init();
  virtual QString helpString() const;

 private :
  QWidget *m_parent;
  BricksWidget *m_bricksWidget;


  ViewerUndo m_undo;

  QString m_commandString;

  int m_drawVolumeType;
  DrawHiresVolume *m_hiresVolume;
  DrawLowresVolume *m_lowresVolume;

  Volume *m_Volume;
  KeyFrame *m_keyFrame;

  QImage m_lutImage;
  unsigned char *m_lut;
  unsigned char *m_prevLut;
  
  GLuint m_lutTex;
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
  QGLFramebufferObject *m_lowresBuffer;

  GLhandleARB m_blurShader;
  GLint m_blurParm[5];
  GLhandleARB m_copyShader;
  GLint m_copyParm[5];

  bool m_mouseDrag;
  bool m_updatePruneBuffer;
  QPoint m_mousePressPos;
  QPoint m_mousePrevPos;

  uchar *m_backBufferImage;
  int m_backBufferWidth, m_backBufferHeight;

  float m_focusDistance;

  QTimer m_autoUpdateTimer;

  QUdpSocket *m_listeningSocket;
  int m_socketPort;

  bool m_disableRotationInViewport;
  QSpinBox *m_tagSpinBox;
  QSpinBox *m_radSpinBox;

  QWidget *m_paintMenuWidget;

  bool m_brickAngleFromMouse;
  
  bool m_paintMode;
  Vec m_paintColor;
  float m_paintRad;
  int m_paintStyle;
  float m_paintAlpha;
  int m_paintOctave;
  float m_unitPaintRad;


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

  void loadLookupTable(unsigned char*);

  void processCommand(QString);
  void processLight(QStringList);

  QImage getSnapshot();
  void saveSnapshot(QString);
  void fboToMovieFrame();
  void screenToMovieFrame();

  void setWidgetSizeToImageSize();
  void restoreOriginalWidgetSize();

  void splashScreen();
  bool bindFBOs(int);
  void releaseFBOs(int);
  void drawInfoString(int, float);

  void createBlurShader();
  void createCopyShader();

  void grabBackBufferImage();
  void showBackBufferImage();

  void undoParameters();
  void commandEditor();
  void showHelp();

  void handleMorphologicalOperations(QStringList);
  void drawCarveCircle();
  void drawCarveCircleInViewport(int);

  Vec checkPointSelectedInViewport(int, QPoint, bool&);

  Vec setViewportCamera(int, Camera&);
  bool mousePressEventInViewport(int, QMouseEvent*);
  bool mouseMoveEventInViewport(int, QMouseEvent*);

  bool mouseMoveEventInPathViewport(int, QMouseEvent*);  


  void showMenuFunctionHelp(QString);

  void createPaintMenu();
};
