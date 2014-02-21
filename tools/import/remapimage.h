#ifndef REMAPIMAGE_H
#define REMAPIMAGE_H

#include <QWidget>
#include "commonqtclasses.h"
#include <QStatusBar>
#include <QScrollBar>
#include <QFileDialog>
#include <QInputDialog>
#include <QTextEdit>
#include <QVBoxLayout>

class RemapImage : public QWidget
{
  Q_OBJECT

 public :
  RemapImage(QWidget *parent=NULL);

  enum SliceType {
    DSlice,
    WSlice,
    HSlice
  };
  

  void loadLimits();
  void saveLimits();
  void saveImage();

  void setGridSize(int, int, int);
  void setSliceType(int);
  void setImage(QImage);
  void setRawValue(QPair<QVariant, QVariant>);

  QVector<QRgb> colorMap();

  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);

  void keyPressEvent(QKeyEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);
  
  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);

  void emitSaveTrimmed();
  void emitSaveIsosurface();
  void emitSaveTrimmedImages();

  void depthUserRange(int&, int&);
  void widthUserRange(int&, int&);
  void heightUserRange(int&, int&);

 public slots :
  void setGradientStops(QGradientStops);
  void userRangeChanged(int, int);
  void sliceChanged(int);

 signals :
  void getSlice(int);
  void getRawValue(int, int, int);
  void newMinMax(float, float);
  void saveTrimmed(int, int,
		   int, int,
		   int, int);
  void saveIsosurface(int, int,
		      int, int,
		      int, int);
  void saveTrimmedImages(int, int,
			 int, int,
			 int, int);
  void extractRawVolume();

 private :
  int m_currSlice;

  int m_sliceType;
  int m_minDSlice, m_maxDSlice;
  int m_minWSlice, m_maxWSlice;
  int m_minHSlice, m_maxHSlice;
  int m_Depth, m_Width, m_Height;

  int m_imgHeight, m_imgWidth;
  int m_simgHeight, m_simgWidth;
  int m_simgX, m_simgY;
  QImage m_image;
  QImage m_imageScaled;

  float m_zoom;

  int m_button;
  bool m_pickPoint;
  int m_pickDepth, m_pickWidth, m_pickHeight;

  QVariant m_rawValue;
  QVariant m_pvlValue;
  QPoint m_cursorPos;

  QRectF m_rubberBand;
  bool m_rubberBandActive;
  bool m_rubberXmin;
  bool m_rubberYmin;
  bool m_rubberXmax;
  bool m_rubberYmax;
  bool m_rubberNew;
  
  QGradientStops m_gradientStops;
  QVector<QRgb> m_colorMap;

  void resizeImage();
  void drawRawValue(QPainter*);
  void generateColorTable();
  void drawRubberBand(QPainter*);

  bool checkRubberBand(int, int, bool);
  bool checkSlider(int, int);

  void updateRubberBand(int, int);
  bool validPickPoint(int, int);

  void setZoom(float);

  void updateStatusText();
};


#endif
