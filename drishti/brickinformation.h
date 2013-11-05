#ifndef BRICKINFORMATION_H
#define BRICKINFORMATION_H


#include <GL/glew.h>
#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

class DrawBrickInformation
{
 public :
  DrawBrickInformation();
  ~DrawBrickInformation();
  
  void reset();
  int numBricks();
  int subvolSize();
  Vec subvol(int);

  bool get(int, int&,
	   Vec*, Vec*,
	   Vec&, Vec&,
	   QList<bool>&,
	   Vec&, Vec&);
  void append(int,
	      Vec*, Vec*,
	      Vec, Vec,
	      QList<bool>,
	      Vec, Vec);

 private :
  int m_numBricks;
  QList<int> m_tfSet;
  QList<Vec> m_subvol;
  QList<Vec> m_subcorner;
  QList<Vec> m_subdim;
  QList<Vec> m_scalepivot;
  QList<Vec> m_scale;
  QList<Vec> m_texture;

  QList< QList<bool> > m_clips;
};

class BrickBounds
{
 public :
  BrickBounds();
  BrickBounds(Vec, Vec);
  BrickBounds& operator=(const BrickBounds&);
  Vec brickMin, brickMax;
};

class BrickInformation
{
 public :
  BrickInformation();
  ~BrickInformation();
  BrickInformation(const BrickInformation&);
  BrickInformation& operator=(const BrickInformation&);

  Vec getCorrectedPivot();
  Vec getCorrectedScalePivot();

  static BrickInformation interpolate(const BrickInformation,
				      const BrickInformation,
				      float);

  static QList<BrickInformation> interpolate(const QList<BrickInformation>,
					     const QList<BrickInformation>,
					     float);

  void load(fstream&);
  void save(fstream&);

  int tfSet;
  int linkBrick;
  Vec brickMin, brickMax;
  Vec position;
  Vec pivot;
  Vec axis;
  float angle;
  Vec scalepivot;
  Vec scale;
  QList<bool> clippers;

 private :
  void reset();
};

#endif
