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
  void mouseReleaseEvent(QMouseEvent*);

  bool keyPressEvent(QKeyEvent*);

  QImage gradientImage();
  QVector<QPointF> poly();
  QVector<QPointF> polyA();
  QVector<QPointF> polyB();
  QVector<int> seedpos();
  QVector<QPointF> livewire();
  QVector<QPointF> seeds();
  bool closed();
  uchar type();

  void setSmoothType(int s) { m_smoothType = s; };
  void setGradType(int g) { m_gradType = g; };

  void freeze();

  void setWeightG(float);
  void setWeightN(float);
  void setUseDynamicTraining(bool);

  void livewireFromSeeds(QVector<QPointF>);

  void moveShape(int, int);
  bool seedMoveMode();
  void setSeedMoveMode(bool);
  void setPolygonToUpdate(QVector<QPointF>,
			  QVector<int>,
			  bool, uchar,
			  QVector<QPointF>);
  
  bool propagateLivewire();
  void setPropagateLivewire(bool);
  void setGuessCurve(QVector<QPointF>, QVector<QPointF>);
  void renewGuessCurve();

  void setLod(int);
  int lod() { return m_lod; }

 private :
  QVector<QPointF> m_poly;
  QVector<QPointF> m_polyA;
  QVector<QPointF> m_polyB;
  QVector<QPointF> m_livewire;
  QVector<int> m_seedpos;
  QVector<QPointF> m_seeds;

  int m_activeSeed;
  bool m_seedMoveMode;
  bool m_closed;
  uchar m_type;

  bool m_propagateLivewire;
  QVector<QPointF> m_guessCurve;
  QVector<QPointF> m_guessSeeds;

  int m_Owidth, m_Oheight;
  int m_width, m_height;
  uchar *m_Oimage;
  uchar *m_image;
  float *m_grad;
  float *m_normal;
  uchar *m_tmp;

  float *m_gradCost;
  bool m_useDynamicTraining;

  float m_wtG, m_wtN;

  int m_gradType;
  int m_smoothType;

  int m_lod;

  QVector<float> m_edgeWeight;
  QVector<float> m_cost;
  QVector<QPointF> m_prev;

  QImage m_gradImage;

  void calculateEdgeWeights();
  void calculateGradients();
  void calculateCost(int, int, int boxSize=100);
  void calculateCost1(int, int, int boxSize=100);
  void calculateLivewire(int, int);

  void applySmoothing(uchar*, int, int, int);
  void gaussBlur_4(uchar*, uchar*, int, int, int);
  QVector<int> boxesForGauss(float, int);
  void boxBlur_4(uchar*, uchar*, int, int, int);
  void boxBlurH_4(uchar*, uchar*, int, int, int);
  void boxBlurT_4(uchar*, uchar*, int, int, int);

  void updateLivewireFromSeeds(int, int);
  void splitPolygon(int);
  int  getActiveSeed(int, int);
  int  insertSeed(int, int);
  int  removeSeed(int, int);

  int getActiveSeedFromShape(int, int);
  void updateShapeFromSeeds(int, int);
  void updatePolygonFromSeeds();
  void updateEllipseFromSeeds();
};

#endif
