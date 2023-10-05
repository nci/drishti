#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QWidget>
#include "commonqtclasses.h"
#include <QStatusBar>
#include <QScrollBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVector3D>
#include <QScrollArea>

#include <QGLViewer/vec.h>
using namespace qglviewer;

class ImageWidget : public QWidget
{
  Q_OBJECT

 public :
  ImageWidget(QWidget*);

  enum SliceType {
    DSlice = 0,
    WSlice,
    HSlice
  };

  void saveImage();
  void saveImageSequence();

  void setScrollArea(QScrollArea *sa) { m_scrollArea = sa; }

  void setVolPtr(uchar *vp) {m_volPtr = vp;}
  void setMaskPtr(uchar *mp) {m_maskPtr = mp;}
  
  void setGridSize(int, int, int);
  void setSliceType(int);
  int sliceType() { return m_sliceType; }
  void resetSliceType();
  void setMaskImage(uchar*);

  void setRawValue(QList<int>);

  void setMinGrad(float);
  void setMaxGrad(float);
  void setGradType(int);
  
  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);

  void mouseDoubleClickEvent(QMouseEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);

  void loadLookupTable(QImage);

  void depthUserRange(int&, int&);
  void widthUserRange(int&, int&);
  void heightUserRange(int&, int&);

  void getBox(int&, int&, int&, int&, int&, int&);
  void setBox(int, int, int, int, int, int);
  
  void processPrevSliceTags();

  QVector3D pickedPoint() { return QVector3D(m_lastPickDepth, m_lastPickWidth, m_lastPickHeight); };
  
  void setShowPosition(bool);
  int currentSliceNumber() { return m_currSlice; }
			  
 public slots :
  void setHLine(int);
  void setVLine(int);

  //void bbupdated(Vec, Vec);
  void updateTagColors();
  void userRangeChanged(int, int);
  void smooth(int, bool, bool);
  void keyPressEvent(QKeyEvent*);
  void setSlice(int);

  void zoom0Clicked();
  void zoom9Clicked();
  void zoomUpClicked();
  void zoomDownClicked();

  void shrinkwrapPaintedRegion();
  void shrinkwrapVisibleRegion();

  void setModeType(int);

  void applyFilters();

  void multiSliceOperation();
  void restartRecursive();
  
 signals :
  void xPos(int);
  void yPos(int);
  void sliceChanged(int);
  void setSliceNumber(int);

  void updateSliderLimits();
  void resetSliderLimits();
  
  void disconnectSlider();
  void reconnectSlider();
  
  void saveWork();
  void viewerUpdate();
  void getRawValue(int, int, int);
  void newMinMax(float, float);
  void tagDSlice(int, QImage);
  void tagWSlice(int, QImage);
  void tagHSlice(int, QImage);
  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);
  void fillVolume(int, int,
		  int, int,
		  int, int,
		  QList<int>,
		  bool);
  void tagAllVisible(int, int,
		     int, int,
		     int, int);

  void applyMaskOperation(int, int, int);
  void updateViewerBox(int, int, int, int, int, int);

  void setPropagation(bool);

  void shrinkwrap(Vec, Vec,
		  int, bool, int,
		  bool,
		  int, int, int,
		  int);
  void connectedRegion(int, int, int,
		       Vec, Vec,
		       int, int);
  
 private :
  QScrollArea *m_scrollArea;

  uchar *m_volPtr;
  uchar *m_maskPtr;

  //modeType 
  // 0 - graphcut
  // 1 - superpixel
  int m_modeType;

  int m_maxSlice;
  int m_currSlice;

  float m_minGrad, m_maxGrad;
  int m_gradType;
  
  QVector<QRgb> m_tagColors;
  QVector<QRgb> m_prevslicetagColors;

  int m_sliceType;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;
  int m_Depth, m_Width, m_Height;
  int m_bytesPerVoxel;

  float m_zoom;

  int m_imgHeight, m_imgWidth;
  int m_simgHeight, m_simgWidth;

  bool m_showPosition;
  int m_hline;
  int m_vline;

  QImage m_image;
  QImage m_imageScaled;

  QImage m_maskimage;
  QImage m_maskimageScaled;

  QImage m_userimage;
  QImage m_userimageScaled;

  QImage m_prevslicetagimage;
  QImage m_prevslicetagimageScaled;

  QImage m_gradImageScaled;

  uchar *m_slice;
  uchar *m_sliceFiltered;
  uchar *m_sliceImage;

  uchar *m_maskslice;

  uchar *m_lut;

  int m_button;
  bool m_pickPoint;
  int m_pickDepth, m_pickWidth, m_pickHeight;
  int m_lastPickDepth, m_lastPickWidth, m_lastPickHeight;

  QList<int> m_vgt;
  QPoint m_cursorPos;

  QRectF m_rubberBand;
  bool m_rubberBandActive;
  bool m_rubberXmin;
  bool m_rubberYmin;
  bool m_rubberXmax;
  bool m_rubberYmax;
  bool m_rubberNew;

  uchar *m_prevtags;
  uchar *m_usertags;
  uchar *m_prevslicetags;
  uchar *m_tags;
  uchar *m_tmptags;

  bool m_applyRecursive;
  bool m_extraPressed;
  int m_cslc, m_maxslc;
  int m_slcBlock;
  int m_key;
  bool m_forward;

  int m_pointSize;

  QList<int> m_showTags;
  
  
  void resizeImage();
  void recolorImage();
  
  void drawSizeText(QPainter*, int, int);
  void drawRawValue(QPainter*);
  void processCommands(QString);

  void drawBoxes2D(QPainter*);

  void drawRubberBand(QPainter*);
  bool checkRubberBand(int, int);

  void updateRubberBand(int, int);
  bool validPickPoint(int, int);
  
  bool withinBounds(int, int);
  void dotImage(int, int, bool);
  void removeDotImage(int, int);

  void applyGraphCut();
  void applyPaint(bool);
  void applyReset();
  
  void setZoom(float);
  void preselect();

  void updateMaskImage();

  void tagUntaggedRegion();

  void getSliceLimits(int&, int&, int&, int&, int&, int&);

  void applyRecursive(int);
  void checkRecursive();

  void graphcutModeKeyPressEvent(QKeyEvent*);
  void graphcutMousePressEvent(QMouseEvent*);
  void graphcutMouseMoveEvent(QMouseEvent*);

  void doAnother(int);

  void getSlice();

  void applyGradLimits();

  void onlyImageScaled();

  void update3DBox(bool);
  void update2DBox(bool);
};


#endif
