#ifndef TICK_H
#define TICK_H

#include <GL/glew.h>

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

class Tick
{
 public :

  static void setTickSize(int);
  static int tickSize();

  static void setTickStep(int);
  static int tickStep();

  static void setLabelX(QString);
  static QString labelX();

  static void setLabelY(QString);
  static QString labelY();

  static void setLabelZ(QString);
  static QString labelZ();

  static void draw(Camera*, double*);
  static void drawTick(int,
		       Vec, Vec,
		       float, float, int, float, float,
		       GLdouble*, GLdouble*, GLint*);
  static int findMaxDistance(int[4][2], GLdouble*, GLdouble*);

 private :
  static int m_tickSize;
  static int m_tickStep;
  static QString m_labelX;
  static QString m_labelY;
  static QString m_labelZ;

  static bool drawGlutText(QString,
			   int, int, int, int,
			   float, float,
			   float, float,
			   float, bool, float);
};

#endif
