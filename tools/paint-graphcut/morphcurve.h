#include <QtWidgets>
#include "commonqtclasses.h"
#include <QPoint>
#include <QMap>

//#include <QGLViewer/qglviewer.h>
//using namespace qglviewer;

// A tuple containing the editions array (an int[n][3] array)
// and the length of such array.
// This is used only by the findOptimalEditSequence method.
class Editions
{
 public :
  Editions();
  ~Editions();

  Editions(const Editions&);
  Editions& operator=(const Editions&);

  QVector<int> editions;
  int length; //the number of editions.
  double distance; // between the two perimeters
};

// Define a struct to contain the array of x and y values,
// and the z-index of all points as well.
class Perimeter
{
 public :
  Perimeter();
  ~Perimeter();

  Perimeter(const Perimeter&);
  Perimeter& operator=(const Perimeter&);

  QVector<double> x; //the list of x coordinates
  QVector<double> y; //the list of y coordinates
  QVector<double> v_x; //the vectors from one point to the next, once subsampled.
  QVector<double> v_y;
  double delta; // the average point interdistance
  int length; // the number of elements in x, y, v_x and v_y.
  double z; // the z coord of all x,y points. Perimeters are flat in the XY plane.
  bool subsampled; // whether the perimeter has already been subsampled in place. It also guarantes, when '1', that the vectors exists, and when '0', that they don't.
  QList<Editions> editions; // may be null if it was not one of the original perimeters.
};

/** A tuple containing the starting index and the matrix for that index. Used only in the findMinimumEditDistance below, in the divide and conquer approach */
class MinDist
{
 public :
  MinDist();
  ~MinDist();

  MinDist(const MinDist&);
  MinDist& operator=(const MinDist&);

  void set(double**, double**, int, int);

  int ndim, mdim;
  int min_j;
  double min_dist;
  double** matrix;
  double** matrix_e;
};

/** A tuple to be used to return the array of interpolated perimeters and its length.*/
class Result
{
 public :
  Result();
  ~Result();
  
  Result(const Result&);
  Result& operator=(const Result&);

  QList<Perimeter> p;
  int length;
};

/** A tuple containing:
 *  - the verts as a 2D array of their X,Y,Z coords, in the form double[n_verts][3]
 *  - the faces as a 2D array of the vert indices that each face uses, in the form int[n_faces][4]
 *   (faces can be quads. If the fourth vert is -1, then it's a triangle).
 *
 */
class Skin
{
 public :
  Skin();
  ~Skin();

  double** verts;
  int n_verts;
  int** faces;
  int n_faces;
};

class MorphCurve
{
 public :
  MorphCurve();
  ~MorphCurve();

  void setPaths(QStringList);
  void setPaths(QMap<int, QVector<QPoint>>);
  QList<Perimeter> getMorphedPaths();

  enum EditOperation
    {
      DELETION = 1,
      INSERTION,
      MUTATION
    };

 private :
  QList<Perimeter> m_perimeters;
  Result *m_result;
  
  double getAngle(double, double);
  
  double getAndSetAveragePointInterdistance(Perimeter&);
  QVector<double> recalculate(QVector<double>, int, double);
  Perimeter subsample(Perimeter, double);
  double** findEditMatrix(Perimeter, Perimeter, int, double, double**);
  int findStartingPoint(Perimeter, Perimeter);
  MinDist findMinDist(Perimeter, Perimeter, double, int, int, int, MinDist);
  double** findMinimumEditDistance(Perimeter&, Perimeter&, double, int);
  QVector<int> resizeAndFillEditionsCopy(QVector<int>, int, int);
  Editions findOptimalEditSequence(Perimeter, Perimeter, double, int);
  Perimeter getMorphedPerimeter(Perimeter, Perimeter, QVector<int>, int, double);
  Result* getMorphedPerimeters(Perimeter, Perimeter, int, double, int);
  Result* getAllPerimeters(QList<Perimeter>, int, int, double, int);

//  int** resizeFacesArray(int**, int, int);
//  Skin* makeSkinByOrthogonalMatch(Result*, double, double, int);
//  Skin* makeSkinByStringMatch(Result*, double, double, int);
//  Perimeter* makePerimeterFromBezierData(double*, double*, double*, double*, double*, double*, double, int, int);

};
