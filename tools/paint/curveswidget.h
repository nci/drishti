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
#include "fibergroup.h"
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

  void saveImage();

  void setScrollArea(QScrollArea *sa) { m_scrollArea = sa; }
  
  void setGridSize(int, int, int);
  void setSliceType(int);
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
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);

  void loadLookupTable(QImage);

  void depthUserRange(int&, int&);
  void widthUserRange(int&, int&);
  void heightUserRange(int&, int&);

  void setScrollBars(QScrollBar*, QScrollBar*);
  void getBox(int&, int&, int&, int&, int&, int&);
  void setBox(int, int, int, int, int, int);

  void processPrevSliceTags();

  QVector3D pickedPoint() { return QVector3D(m_lastPickDepth, m_lastPickWidth, m_lastPickHeight); };

  QMultiMap<int, Curve*>* multiMapCurvesD() { return m_dCurves.multiMapCurves(); };
  QMultiMap<int, Curve*>* multiMapCurvesW() { return m_wCurves.multiMapCurves(); };
  QMultiMap<int, Curve*>* multiMapCurvesH() { return m_hCurves.multiMapCurves(); };

  QList< QMap<int, Curve> >* morphedCurvesD() { return m_dCurves.morphedCurves(); };
  QList< QMap<int, Curve> >* morphedCurvesW() { return m_wCurves.morphedCurves(); };
  QList< QMap<int, Curve> >* morphedCurvesH() { return m_hCurves.morphedCurves(); };

  QList< QMultiMap<int, Curve*> >* shrinkwrapCurvesD() { return m_dCurves.shrinkwrapCurves(); };
  QList< QMultiMap<int, Curve*> >* shrinkwrapCurvesW() { return m_wCurves.shrinkwrapCurves(); };
  QList< QMultiMap<int, Curve*> >* shrinkwrapCurvesH() { return m_hCurves.shrinkwrapCurves(); };

  QList<Fiber*>* fibers() { return m_fibers.fibers(); };

  bool seedMoveMode() { return m_livewire.seedMoveMode(); };
  void deselectAll();
  void propagateCurves(bool);

  bool dCurvesPresent() { return m_dCurves.curvesPresent(); };
  bool wCurvesPresent() { return m_wCurves.curvesPresent(); };
  bool hCurvesPresent() { return m_hCurves.curvesPresent(); };
  bool fibersPresent() { return m_fibers.fibersPresent(); };

  void resetCurves();

  void setMinGrad(float);
  void setMaxGrad(float);
  void setGradThresholdType(int);
  
  void setVolPtr(uchar *vp) {m_volPtr = vp;}
  
 public slots :
  void updateTagColors();
  void sliceChanged(int);
  void userRangeChanged(int, int);
  void keyPressEvent(QKeyEvent*);
  void setLivewire(bool);
  void setFiberMode(bool);
  void setCurve(bool);
  void saveCurves(QString);
  void saveCurves();
  void loadCurves();
  void loadCurves(QString);
  void saveFibers();
  void loadFibers();
  void loadFibers(QString);
  void setSmoothType(int);
  void setGradType(int);
  void freezeLivewire(bool);
  void newCurve(bool);
  void endCurve();
  void newFiber();
  void endFiber();
  void morphCurves();
  void morphSlices();
  void deleteAllCurves();
  void zoom0();
  void zoom9();
  void zoomUp();
  void zoomDown();
  void setPointSize(int);
  void setMinCurveLength(int);
  void setSliceLOD(int);

  void paintUsingCurves(int, int, int, int, uchar*, QList<int>);

  void modifyUsingLivewire();
  void freezeModifyUsingLivewire();

  void setLambda(float);
  void setSegmentLength(int);
  void showTags(QList<int>);

 signals :
  void saveWork();
  void viewerUpdate();
  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);
  void showEndCurve();
  void hideEndCurve();
  void showEndFiber();
  void hideEndFiber();
  void getSlice(int);
  void getRawValue(int, int, int);
  void newMinMax(float, float);

  void polygonLevels(QList<int>);
  void updateViewerBox(int, int, int, int, int, int);

  void setPropagation(bool);
  void saveMask();
  
 private :
  QScrollArea *m_scrollArea;

  QStatusBar *m_statusBar;
  QScrollBar *m_hbar, *m_vbar;

  int m_currSlice;

  bool m_livewireMode;
  LiveWire m_livewire;

  bool m_addingCurvePoints;
  bool m_curveMode;
  CurveGroup m_dCurves;
  CurveGroup m_wCurves;
  CurveGroup m_hCurves;

  bool m_fiberMode;
  FiberGroup m_fibers;

  QVector<QRgb> m_tagColors;
  QVector<QRgb> m_prevslicetagColors;

  int m_sliceType;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;
  int m_Depth, m_Width, m_Height;

  float m_zoom;

  int m_imgHeight, m_imgWidth;
  int m_simgHeight, m_simgWidth;
  int m_simgX, m_simgY;

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

  bool m_applyRecursive;
  bool m_extraPressed;
  int m_cslc, m_maxslc;
  int m_key;
  bool m_forward;

  int m_pointSize;

  uchar *m_volPtr;
  float m_minGrad, m_maxGrad;
  int m_gradType;
  
  QList<int> m_showTags;

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
  void drawFibers(QPainter*);

  bool checkRubberBand(int, int);

  void updateRubberBand(int, int);
  bool validPickPoint(int, int);
  
  bool withinBounds(int, int);
  void dotImage(int, int, bool);
  void removeDotImage(int, int);

  void setZoom(float);
  void preselect();

  void updateMaskImage();

  void tagUntaggedRegion();

  void getSliceLimits(int&, int&, int&, int&, int&, int&);

  void applyRecursive(int);
  void checkRecursive();
  
  void paintUsingCurves(CurveGroup*, int, int, int, uchar*, QList<int>);
  void paintUsingCurves(uchar*);

  void saveFibers(QFile*);
  void loadFibers(QFile*);

  void saveCurves(QFile*, CurveGroup*);
  void loadCurves(QFile*, CurveGroup*);

  void saveMorphedCurves(QFile*, CurveGroup*);
  void loadMorphedCurves(QFile*, CurveGroup*);

  void saveShrinkwrapCurves(QFile*, CurveGroup*);
  void loadShrinkwrapCurves(QFile*, CurveGroup*);

  void saveCurveData(QFile*, int, Curve*);
  QPair<int, Curve> loadCurveData(QFile*);

  void curveModeKeyPressEvent(QKeyEvent*);
  bool fiberModeKeyPressEvent(QKeyEvent*);

  void propagateLivewire();

  QList<QPointF> trimPointList(QList<QPointF>, bool);

  void startLivewirePropagation();
  void endLivewirePropagation();

  void modifyUsingLivewire(int, int);

  void curveMousePressEvent(QMouseEvent*);
  void fiberMousePressEvent(QMouseEvent*);
  void curveMouseMoveEvent(QMouseEvent*);

  CurveGroup* getCg();

  void drawSeedPoints(QPainter*, QVector<QPointF>, QColor);
  void drawPoints(QPainter*, QVector<QPointF>, QColor, int, int);
  void drawPoints(QPainter*, QVector<QVector4D>, QColor, int);

  void newPolygon(bool, bool);
  void newEllipse();

  void startShrinkwrap();
  void endShrinkwrap();
  void shrinkwrapCurve();

  void doAnother(int);

  void applyPaint(bool);

  void applyGradLimits();

  void onlyImageScaled();
};


#endif
