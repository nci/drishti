#ifndef CURVESWIDGET_H
#define CURVESWIDGET_H

#include <QWidget>
#include "commonqtclasses.h"
#include <QStatusBar>
#include <QScrollBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QVector3D>

#include "livewire.h"
#include "curvegroup.h"
#include <QScrollArea>

class CurvesWidget : public QWidget
{
  Q_OBJECT

 public :
  CurvesWidget(QWidget*, QStatusBar *);

  enum SliceType {
    DSlice = 0,
    WSlice,
    HSlice
  };

  static CurveGroup *m_dCurves;
  static CurveGroup *m_wCurves;
  static CurveGroup *m_hCurves;
  
  void setInFocus();
  bool inFocus() { return m_hasFocus; }
  
  void saveImage();

  void setScrollArea(QScrollArea *sa) { m_scrollArea = sa; }
  
  void setGridSize(int, int, int);
  void setSliceType(int);
  void resetSliceType();
  int sliceType() { return m_sliceType; }
  void setImage(uchar*, uchar*);
  void setMaskImage(uchar*);

  void setRawValue(QList<int>);

  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);

  void mouseDoubleClickEvent(QMouseEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);
  void focusInEvent(QFocusEvent*);
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);
  
  void loadLookupTable(QImage);

  void depthUserRange(int&, int&);
  void widthUserRange(int&, int&);
  void heightUserRange(int&, int&);

  void setScrollBars(QScrollBar*, QScrollBar*);
  void getBox(int&, int&, int&, int&, int&, int&);
  void setBox(int, int, int, int, int, int);

  QVector3D pickedPoint() { return QVector3D(m_lastPickDepth, m_lastPickWidth, m_lastPickHeight); };

  QMultiMap<int, Curve*>* multiMapCurves() { return m_Curves.multiMapCurves(); };
  QList< QMap<int, Curve> >* morphedCurves() { return m_Curves.morphedCurves(); };
  QList< QMultiMap<int, Curve*> >* shrinkwrapCurves() { return m_Curves.shrinkwrapCurves(); };


  bool seedMoveMode() { return m_livewire.seedMoveMode(); };
  void deselectAll();
  void propagateCurves(bool);

  bool curvesPresent() { return m_Curves.curvesPresent(); };

  void resetCurves();

  void setMinGrad(float);
  void setMaxGrad(float);
  void setGradThresholdType(int);
  
  void setVolPtr(uchar *vp) {m_volPtr = vp;}

  void setShowPosition(bool);
  int currentSliceNumber() { return m_currSlice; }
  
 public slots :
  void setHLine(int);
  void setVLine(int);
  void updateTagColors();
  void sliceChanged(int);
  void userRangeChanged(int, int);
  void keyPressEvent(QKeyEvent*);
  void setLivewire(bool);
  void setCurve(bool);
  void saveCurves(QString);
  void saveCurves();
  void loadCurves();
  void loadCurves(QString);
  void setSmoothType(int);
  void setGradType(int);
  void freezeLivewire(bool);
  void newCurve(bool);
  void endCurve();
  void morphCurves();
  void morphSlices();
  void deleteAllCurves();
  void zoom0Clicked();
  void zoom9Clicked();
  void zoomUpClicked();
  void zoomDownClicked();
  void setPointSize(int);
  void setMinCurveLength(int);
  void setSliceLOD(int);

  void paintUsingCurves(int, int, int, int, uchar*);

  void setLambda(float);
  void setSegmentLength(int);

  void releaseFocus();

 signals :
  void xPos(int);
  void yPos(int);
  void gotFocus();
  void saveWork();
  void viewerUpdate();
  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);
  void showEndCurve();
  void hideEndCurve();
  void getSlice(int);
  void getRawValue(int, int, int);

  void polygonLevels(QList<int>);
  void updateViewerBox(int, int, int, int, int, int);

  void setPropagation(bool);
  void saveMask();
  
 private :
  bool m_hasFocus;
  
  QScrollArea *m_scrollArea;

  QStatusBar *m_statusBar;
  QScrollBar *m_hbar, *m_vbar;

  int m_maxSlice;
  int m_currSlice;

  bool m_livewireMode;
  LiveWire m_livewire;

  bool m_addingCurvePoints;
  bool m_curveMode;

  CurveGroup m_Curves;


  uchar *m_tagColors;

  int m_sliceType;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;
  int m_Depth, m_Width, m_Height;

  float m_zoom;

  int m_imgHeight, m_imgWidth;
  int m_simgHeight, m_simgWidth;
  int m_simgX, m_simgY;

  bool m_showPosition;
  int m_hline;
  int m_vline;

  QImage m_image;
  QImage m_imageScaled;

  QImage m_maskimage;
  QImage m_maskimageScaled;

  QImage m_gradImageScaled;

  uchar *m_slice;
  uchar *m_sliceImage;

  ushort *m_tags;
  ushort *m_maskslice;

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

  bool m_applyRecursive;
  bool m_extraPressed;
  int m_cslc, m_maxslc;
  int m_key;
  bool m_forward;

  int m_pointSize;

  uchar *m_volPtr;
  float m_minGrad, m_maxGrad;
  int m_gradType;
  

  void resizeImage();
  void recolorImage();
  
  void drawSizeText(QPainter*, int, int);
  void drawRawValue(QPainter*);
  void processCommands(QString);
  void drawRubberBand(QPainter*);
  int  drawCurves(QPainter*);
  int  drawMorphedCurves(QPainter*);
  void drawOtherCurvePoints(QPainter*);
  void drawLivewire(QPainter*);

  bool checkRubberBand(int, int);

  void updateRubberBand(int, int);
  bool validPickPoint(int, int);
  
  bool withinBounds(int, int);

  void setZoom(float);
  void preselect();

  void updateMaskImage();

  void tagUntaggedRegion();

  void getSliceLimits(int&, int&, int&, int&, int&, int&);

  void applyRecursive(int);
  void checkRecursive();
  
  void paintUsingCurves(CurveGroup*, int, int, int, uchar*);
  void paintUsingCurves(uchar*);


  void saveCurves(QFile*, CurveGroup*);
  void loadCurves(QFile*, CurveGroup*);

  void saveMorphedCurves(QFile*, CurveGroup*);
  void loadMorphedCurves(QFile*, CurveGroup*);

  void saveShrinkwrapCurves(QFile*, CurveGroup*);
  void loadShrinkwrapCurves(QFile*, CurveGroup*);

  void saveCurveData(QFile*, int, Curve*);
  QPair<int, Curve> loadCurveData(QFile*);

  void curveModeKeyPressEvent(QKeyEvent*);
  void curveMousePressEvent(QMouseEvent*);
  void curveMouseMoveEvent(QMouseEvent*);

  QList<QPointF> trimPointList(QList<QPointF>, bool);


  void drawSeedPoints(QPainter*, QVector<QPointF>, QColor);
  void drawPoints(QPainter*, QVector<QPointF>, QColor, int, int);
  void drawPoints(QPainter*, QVector<QVector4D>, QColor, int);

  void newPolygon(bool, bool);
  void newEllipse();

  void startShrinkwrap();
  void endShrinkwrap();
  void shrinkwrapCurve();

  void doAnother(int);

  void applyClipping();
  void applyGradLimits();

  void onlyImageScaled();
};


#endif
