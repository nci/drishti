#ifndef LIVEWIRE_H
#define LIVEWIRE_H


#include <QtGui>

class LiveWire
{
 public :
  LiveWire();
  ~LiveWire();

  void resetPoly();
  void reset();
  void setImageData(int, int, uchar*);

  bool mousePressEvent(int, int, QMouseEvent*);
  bool mouseMoveEvent(int, int, QMouseEvent*);
  bool mousePressEvent(QMouseEvent*);
  bool mouseMoveEvent(QMouseEvent*);
  void mouseReleaseEvent(QMouseEvent*);

  bool keyPressEvent(QKeyEvent*);

  QImage gradientImage();
  QVector<QPoint> poly();
  QVector<QPoint> livewire();


 private :
  QVector<QPoint> m_poly;
  QVector<QPoint> m_livewire;

  int m_width, m_height;
  uchar *m_image;
  float *m_grad;
  uchar *m_tmp;

  QVector<float> m_edgeWeight;
  QVector<float> m_cost;
  QVector<QPoint> m_prev;

  QImage m_gradImage;

  void calculateEdgeWeights();
  void calculateGradients();
  void calculateCost(int, int, int boxSize=100);
  void calculateCost1(int, int, int boxSize=100);
  void calculateLivewire(int, int);

  void applySmoothing(int);
  void gaussBlur_4(uchar*, uchar*, int, int, int);
  QVector<int> boxesForGauss(float, int);
  void boxBlur_4(uchar*, uchar*, int, int, int);
  void boxBlurH_4(uchar*, uchar*, int, int, int);
  void boxBlurT_4(uchar*, uchar*, int, int, int);
};

#endif
