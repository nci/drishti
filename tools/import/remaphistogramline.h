#ifndef REMAPHISTOGRAMLINE_H
#define REMAPHISTOGRAMLINE_H

#include <QtGui>

class RemapHistogramLine : public QObject
{
 Q_OBJECT

 public :
  RemapHistogramLine();

  bool grabsMouse();

  void setLine(int, int);
  QPoint line();

  void mousePress(int, int);
  void mouseMove(int);
  void mouseRelease();

  int activeTickNumber();
  QList<float> ticks();
  QList<float> ticksOriginal();

  void setKeepEndsFixed(bool);

  void addTick(float);
  void moveTick(int, float);

 signals :
  void addTick(int);
  void removeTick();
  void updateScene(int);

 private :
  bool m_keepGrabbing;
  int m_start, m_width;
  bool m_keepEndsFixed;
  uint m_tickMaxKey;
  uint m_tickMinKey;
  QMap<uint, uint> m_ticks;
  QMap<uint, uint> m_ticksOriginal;
  int m_activeTickNumber;
  int m_activeTick;
  int m_activeB1, m_activeB2;
};

#endif
