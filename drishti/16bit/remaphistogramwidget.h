#ifndef REMAPHISTOGRAMWIDGET_H
#define REMAPHISTOGRAMWIDGET_H

#include "remaphistogramline.h"
#include <QWidget>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>

class RemapHistogramWidget : public QWidget
{
  Q_OBJECT

 public :
  RemapHistogramWidget(QWidget *parent=NULL);

  void setPvlMapMax(int);

  void setMapping(float, float);

  void setRawMinMax(float, float, int, int);
  void setHistogram(QList<uint>);

  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);

  void keyPressEvent(QKeyEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void wheelEvent(QWheelEvent*);

  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);

  QList<float> rawMap();
  QList<int> pvlMap();

 public slots :
  void setGradientStops(QGradientStops);

 signals :
  void getHistogram();
  void newMapping(float, float);
  void newMinMax(float, float);

 private slots :
  void addTick(int);
  void removeTick();
  void updateScene(int);

 private :
  QWidget *m_parent;

  RemapHistogramLine *m_Line;
  QList<uint> m_histogram;
  QList<uint> m_histogramScaled;

  QGradientStops m_gradientStops;

  int m_activeTick;
  float m_scale;
  int m_base, m_width, m_shift;
  int m_lineOrigin, m_lineWidth, m_lineHeight;

  int m_scaleHistogram;
  float m_rawMin, m_rawMax;
  uint m_histMax;
  float m_orawMin, m_orawMax;
  bool m_validRawMinMax;
  
  int m_button, m_prevX, m_prevY;
  bool m_moveLine;  

  int m_pvlMapMax;
  QList<float> m_rawMap;
  QList<int> m_pvlMap;
  
  QList<QGraphicsEllipseItem*> m_tickMarks;
  QList<QGraphicsLineItem*> m_lineMarks;

  QList<QPoint> tickMarkers();
  QList<QPoint> tickMarkersOriginal();

  void rescaleHistogram();

  void newMapping(QList<float>,
		  QList<int>);

  void resizeLine();

  void drawMainLine(QPainter*);

  void drawTickMarkers(QPainter*,
		       QList<QPoint>,
		       QColor, QColor, QColor);

  void drawConnectionLines(QPainter*,
			   QList<QPoint>,
			   QList<QPoint>);

  void drawColorMap(QPainter*);

  void drawHistogram(QPainter*);

  void defineMapping();
  void drawRawValues(QPainter*);
  
  void processCommands(QString);
};


#endif
