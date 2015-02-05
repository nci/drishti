#include "morphcurve.h"

#include <math.h>

#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QProgressDialog>

#define PI 3.1415926535897931
#define DLARGE 100000000.0

//--------------------------------------------------
Editions::Editions()
{
  editions.clear();
  length = 0;
  distance = 0;
}

Editions::~Editions()
{
  editions.clear();
  length = 0;
  distance = 0;
}

Editions::Editions(const Editions& e)
{
  editions = e.editions;
  length = e.length;
  distance = e.distance;
}

Editions& 
Editions::operator=(const Editions& e)
{
  editions = e.editions;
  length = e.length;
  distance = e.distance;

  return *this;
}
//--------------------------------------------------


//--------------------------------------------------
Perimeter::Perimeter()
{
  x.clear();
  y.clear();
  v_x.clear();
  v_y.clear();
  delta = 0;
  length = 0;
  z = 0;
  subsampled = false;
  editions.clear();
}
Perimeter::~Perimeter()
{
  x.clear();
  y.clear();
  v_x.clear();
  v_y.clear();
  delta = 0;
  length = 0;
  z = 0;
  subsampled = false;
  editions.clear();
}

Perimeter::Perimeter(const Perimeter& p)
{
  x = p.x;
  y = p.y;
  v_x = p.v_x;
  v_y = p.v_y;
  delta = p.delta;
  length = p.length;
  z = p.z;
  subsampled = p.subsampled;
  editions = p.editions;
}
Perimeter&
Perimeter::operator=(const Perimeter& p)
{
  x = p.x;
  y = p.y;
  v_x = p.v_x;
  v_y = p.v_y;
  delta = p.delta;
  length = p.length;
  z = p.z;
  subsampled = p.subsampled;
  editions = p.editions;

  return *this;
}
//--------------------------------------------------



//--------------------------------------------------
MinDist::MinDist()
{
  min_j = 0;
  min_dist = 0;
  matrix = 0;
  matrix_e = 0;
  ndim = 0;
  mdim = 0;
}
MinDist::~MinDist()
{
  min_j = 0;
  min_dist = 0;

  if (ndim > 0)
    {
      for (int i=0; i<ndim; i++)
	delete [] matrix[i];
      for (int i=0; i<ndim; i++)
	delete [] matrix_e[i];

      delete [] matrix;
      delete [] matrix_e;
    }

  matrix = 0;
  matrix_e = 0;
  ndim = 0;
  mdim = 0;
}
MinDist::MinDist(const MinDist& m)
{
  min_j = m.min_j;
  min_dist = m.min_dist;

  ndim = m.ndim;
  mdim = m.mdim;

  matrix = new double*[ndim];
  for(int i=0; i<ndim; i++)
    {
      matrix[i] = new double[mdim];
      memcpy(matrix[i], m.matrix[i], mdim*sizeof(double));
    }

  matrix_e = new double*[ndim];
  for(int i=0; i<ndim; i++)
    {
      matrix_e[i] = new double[mdim];
      memcpy(matrix_e[i], m.matrix_e[i], mdim*sizeof(double));
    }
}
MinDist&
MinDist::operator=(const MinDist& m)
{
  min_j = m.min_j;
  min_dist = m.min_dist;

  if (ndim > 0)
    {
      for (int i=0; i<ndim; i++) delete [] matrix[i];
      for (int i=0; i<ndim; i++) delete [] matrix_e[i];

      delete [] matrix;
      delete [] matrix_e;
    }

  ndim = m.ndim;
  mdim = m.mdim;

  matrix = new double*[ndim];
  for(int i=0; i<ndim; i++)
    {
      matrix[i] = new double[mdim];
      memcpy(matrix[i], m.matrix[i], mdim*sizeof(double));
    }

  matrix_e = new double*[ndim];
  for(int i=0; i<ndim; i++)
    {
      matrix_e[i] = new double[mdim];
      memcpy(matrix_e[i], m.matrix_e[i], mdim*sizeof(double));
    }

  return *this;
}
void
MinDist::set(double** mat, double** mate, int n, int m)
{
  if (ndim > 0)
    {
      for (int i=0; i<ndim; i++) delete [] matrix[i];
      for (int i=0; i<ndim; i++) delete [] matrix_e[i];

      delete [] matrix;
      delete [] matrix_e;
    }

  ndim = n;
  mdim = m;  

  matrix = new double*[ndim];
  for(int i=0; i<ndim; i++)
    {
      matrix[i] = new double[mdim];
      memcpy(matrix[i], mat[i], mdim*sizeof(double));
    }

  matrix_e = new double*[ndim];
  for(int i=0; i<ndim; i++)
    {
      matrix_e[i] = new double[mdim];
      memcpy(matrix_e[i], mate[i], mdim*sizeof(double));
    }
}
//--------------------------------------------------


//--------------------------------------------------
Result::Result()
{
  p.clear();
  length = 0;
}
Result::~Result()
{
  p.clear();  
  length = 0;
}
Result::Result(const Result& r)
{
  p = r.p;
  length = r.length;
}
Result&
Result::operator=(const Result& r)
{
  p = r.p;
  length = r.length;
  return *this;
}
//--------------------------------------------------


MorphCurve::MorphCurve()
{
  m_perimeters.clear();
  m_result = 0;
}

MorphCurve::~MorphCurve()
{
  m_perimeters.clear();
  if (m_result) delete m_result;
  m_result = 0;
}

QList<Perimeter>
MorphCurve::getMorphedPaths()
{
  // define the number of curves to interpolate between
  // any given z-consecutive pair of perimeters:
  int n_interpolates = -1; //from 0 to infinite
  
  double user_delta = 0.0; // 0 means "use average point interdistance as delta"
  m_result = getAllPerimeters(m_perimeters, m_perimeters.count(),
			       n_interpolates, user_delta, 1);
  return m_result->p;  
}

void
MorphCurve::setPaths(QStringList paths)
{
  m_perimeters.clear();

  for(int np=0; np<paths.count(); np++)
    {
      Perimeter p;
      QVector<double> x;
      QVector<double> y;

      QFile fpath(paths[np]);
      fpath.open(QFile::ReadOnly);
      QTextStream fd(&fpath);
      
      QString line = fd.readLine(); // ignore first line
      QStringList list = line.split(" ", QString::SkipEmptyParts);
      float z = 0;
      if (list.count() == 1)
	{
	  int npts = list[0].toInt();
	  for(int i=0; i<npts; i++)
	    {
	      if (fd.atEnd())
		break;
	      else
		{
		  QString line = fd.readLine();
		  QStringList list = line.split(" ", QString::SkipEmptyParts);
		  if (list.count() == 3)
		    {
		      x << list[0].toFloat();
		      y << list[1].toFloat();
		      if (i==0) z = list[2].toFloat();
		    }
		}
	    }
	}

      p.x = x;
      p.y = y;
      p.z = z;
      p.length = x.count();

      m_perimeters << p;
    }
}

void
MorphCurve::setPaths(QMap< int, QVector<QPoint> > paths)
{
  m_perimeters.clear();

  QList<int> keys = paths.keys();

  for(int i=0; i<keys.count(); i++)
    {
      QVector<QPoint> c = paths.value(keys[i]);

      Perimeter p;
      QVector<double> x;
      QVector<double> y;
      for(int j=0; j<c.count(); j++)
	{
	  x << c[j].x();
	  y << c[j].y();
	}

      p.x = x;
      p.y = y;
      p.z = keys[i];
      p.length = x.count();
      m_perimeters << p;
    }
}

double
MorphCurve::getAngle(double x, double y)
{
  // calculate angle
  double a = atan2(x, y);

  // fix too large angles (beats me why are they ever generated)
  if (a > 2.0 * PI)
    a = a - 2.0 * PI;

  // fix atan2 output scheme to match my mental scheme
  if (a > 0.0 && a <= PI/2)
    a = PI / 2.0 - a;
  else if (a < 0.0 && a >= -PI)
    a = PI / 2.0 -a;
  else if (a > PI/2 && a <= PI)
    a = PI + PI + PI/2.0 - a;
  return a;
}

// Returns the average distance between the points specified in the Perimeter.
// Also sets the delta for that given perimeter.
double
MorphCurve::getAndSetAveragePointInterdistance(Perimeter& p)
{
  double delta = 0.0;
  int length = p.length;
  int i;
  for (i=1; i<length; i++) {
    delta += sqrt((p.x[i] - p.x[i-1])*(p.x[i] - p.x[i-1]) +
		  (p.y[i] - p.y[i-1])*(p.y[i] - p.y[i-1]));
  }
  // last point to first point:
  delta += sqrt((p.x[0] - p.x[length-1])*(p.x[0] - p.x[length-1]) +
		(p.y[0] - p.y[length-1])*(p.y[0] - p.y[length-1]));
  // average:
  delta = delta / (length +1);
  // set the delta for the perimeter:
  p.delta = delta;
  // return average:
  return delta;  
}

// Recalculate an array of weights so that its sum is at most 1.0 
// WARNING this function is recursive.
// The possible infinite loop is broken by the usage of an 'error' range of 0.005
QVector<double>
MorphCurve::recalculate(QVector<double> w0, int length, double sum_)
{
  QVector<double> w;
  w.resize(w0.count());
  
  double sum = 0;
  int q;
  for (q=0; q<length; q++)
    {
      w[q] = w0[q] / sum_;
      sum += w[q];
    }

  double error = 1.0 - sum;
  // make it be an absolute value
  if (error < 0.0)
    error = -error;
  if (error < 0.005)
    w[0] += 1.0 - sum;
  else if (sum > 1.0)
    return recalculate(w, length, sum);

  return w;
}

// Make all points in a given Perimeter have the same interdistance 'delta'.
// The points are changed in place, releasing and reallocating memory when needed.
// The perimeter order points are shifted to counter-clock wise order if necessary.
// The same Perimeter is returned.
Perimeter
MorphCurve::subsample(Perimeter p, double delta_orig)
{
  if (p.subsampled)
    return p;

  // find total path length  
  int xcount = p.x.count(); 
  double plen = 0;
  for (int i=1; i<xcount; i++)
    plen += sqrt((p.x[i] - p.x[i-1])*(p.x[i] - p.x[i-1]) +
		 (p.y[i] - p.y[i-1])*(p.y[i] - p.y[i-1]));
  // because curve is closed
  plen += sqrt((p.x[0] - p.x[xcount-1])*(p.x[0] - p.x[xcount-1]) +
	       (p.y[0] - p.y[xcount-1])*(p.y[0] - p.y[xcount-1]));
  
  int npcount = plen/delta_orig;
  double delta = plen/npcount;

  QVector<double> x, y, vx, vy;
  x << p.x[0];
  y << p.y[0];
  vx << 0;
  vy << 0;
  double clen = 0;
  double pclen = 0;
  int j = x.count();
  for (int i=1; i<xcount+1; i++)
    {
      int xa, xb, ya, yb;
      if (i < xcount)
	{
	  xb = p.x[i];
	  yb = p.y[i];
	  xa = p.x[i-1];
	  ya = p.y[i-1];
	}
      else
	{
	  xb = p.x[0];
	  yb = p.y[0];
	  xa = p.x[xcount-1];
	  ya = p.y[xcount-1];
	}

      double dx = xb-xa;
      double dy = yb-ya;
      clen += sqrt(dx*dx + dy*dy);

      while (j*delta <= clen)
	{
	  double frc = (j*delta - pclen)/(clen-pclen);
	  x << xa + frc*dx;
	  y << ya + frc*dy;

	  j = x.count();
	}
      
      pclen = clen;
    }
  
  int newxcount = x.count();
  for (int i=1; i<newxcount; i++)
    {
      int xa, xb, ya, yb;
      if (i < newxcount)
	{
	  xb = x[i];
	  yb = y[i];
	  xa = x[i-1];
	  ya = y[i-1];
	}
      else
	{
	  xb = x[0];
	  yb = y[0];
	  xa = x[newxcount-1];
	  ya = y[newxcount-1];
	}

      double dx = xb-xa;
      double dy = yb-ya;

      vx << dx;
      vy << dy;
    }

  Perimeter pnew;
  pnew.x = x;
  pnew.y = y;
  pnew.v_x = vx;
  pnew.v_y = vy;
  pnew.delta = p.delta;
  pnew.z = p.z;
  pnew.length = x.count();
  pnew.subsampled = true;

  return pnew;
}

/** Finds the edit matrix for any two given Perimeters
 * 0 is the starting point for p1
 * 'first' is the starting point for p2
 */
double**
MorphCurve::findEditMatrix(Perimeter p1, Perimeter p2,
			   int first, double delta,
			   double** matrix)
{
  // iterators
  int i;
  int j;
  int jj;
  int ii;
  // catch array pointers, for readability (and speed, I believe, saving dereferences)
  int n = p1.length;
  int m = p2.length;

  // the reason why we came here down to C: speed up this loop
  double fun1;
  double fun2;
  double fun3;
  double vs_x, vs_y;

  double* mati;
  double* mat1;
  for (i=1; i< n + 1; i++)
    {
      mati = matrix[i];
      mat1 = matrix[i-1];
      for (jj=1; jj< m + 1; jj++)
	{
	  // offset j to first:
	  j = first + jj -1; //-1 so it starts at 'first'
	  if (j >= m)
	    j = j - m;
	  
	  ii = i;
	  if (ii == n) ii--; // PATCH ! For valgrind's complain. Of course one can't read a vector one element after the end of the string!
	  // TODO this can be indenting the curves sometimes. FIX
	  // cost deletion:
	  fun1 =  mat1[jj] + delta; 
	  // cost insertion:
	  fun2 = mati[jj-1] + delta;
	  // cost mutation:
	  if (i == n || j == m)
	    fun3 = mat1[jj-1];
	  else
	    {
	      vs_x = p1.v_x[ii] - p2.v_x[j];
	      vs_y = p1.v_y[ii] - p2.v_y[j];
	      fun3 = mat1[jj-1] + sqrt(vs_x*vs_x + vs_y*vs_y);
	    }
	  // put the lowest value:
	  // since most are mutations, start with fun3:
	  if (fun3 <= fun1 && fun3 <= fun2)
	    mati[jj] = fun3;
	else if (fun1 <= fun2 && fun1 <= fun3)
	  mati[jj] = fun1;
	else
	  mati[jj] = fun2;
	}
    }

  return matrix;
}

/** Find the minimum sum of the edges of all triangular faces drawn between two perimeters, by applying a crawling test starting at point 0, 1, 2 .. etc of the second perimeter. Always starts at point 0 of the first perimeter.
 * Returns the starting point in the second perimeter.
 *
 * Result: I can see that this algorithm is much faster than the one based on vector string distance. The accuracy is not so high, though, as several artifacts may be introduced around the first and last points.
 * OTOH, I can't use the quad-finding algorithm as in making the skin, because it may introduce artifacts as well -unless quads are counted as three edges, i.e. the two sides and the smallest diagonal.
 * 
 */
int
MorphCurve::findStartingPoint(Perimeter p1, Perimeter p2)
{
  // iterators
  int i, j, jj, k, s;
  
  // catch variables
  int length1 = p1.length;
  int length2 = p2.length;
  
  // 'j' is index over p2 (i.e. x2, y2)
  j = 0;
  // 'jj' is the counter, from 0 to length2
  jj = 0;
  // 'i' is index over p1 (i.e. x1, y1)
  i = 0;
  // 's' is the starting point, which moves one position in every iteration
  s = 0;
  // The sum of all edges
  double sum = 0;
  
  double dist_i1_j0, dist_i0_j1;
  
  // the lowest sum so far
  double min_sum = DLARGE;
  // ... and the corresponding starting point:
  int start = 0;
  
  
  // iterate over every possible starting point of the p2
  for (s=0; s<length2; s++)
    {
      // reset for each iteration
      i = 0;
      jj = 0;
      j = s;
      
      // initial values of the distances, unsquared
      dist_i1_j0 = (p2.x[j] - p1.x[i+1])*(p2.x[j] - p1.x[i+1]) +
	           (p2.y[j] - p1.y[i+1])*(p2.y[j] - p1.y[i+1]);
      if (j + 1 == length2)
	dist_i0_j1 = (p2.x[0] - p1.x[i])*(p2.x[0] - p1.x[i]) +
	             (p2.y[0] - p1.y[i])*(p2.y[0] - p1.y[i]);
      else
	dist_i0_j1 = (p2.x[j+1] - p1.x[i])*(p2.x[j+1] - p1.x[i]) +
	             (p2.y[j+1] - p1.y[i])*(p2.y[j+1] - p1.y[i]);

      // reset sum and add the distance between starting points
      sum = /*dist_i0_j0 == */ (p2.x[j] - p1.x[i])*(p2.x[j] - p1.x[i]) +
	                       (p2.y[j] - p1.y[i])*(p2.y[j] - p1.y[i]);

      while (i < (length1 -1)  &&  jj < (length2 -1))
	{
	  if (dist_i1_j0 < dist_i0_j1)
	    {
	      sum += dist_i1_j0;	      
	      // j doesn't change
	      i += 1;	      
	    }
	  else
	    {
	      sum += dist_i0_j1;
	      // i doesn't change
	      j += 1; // the index
	      jj += 1; // the counter (from 0 to length2)
	    }

	  // adjust j to fall always within array range:
	  if (j >= length2)
	    j -= length2;

	  // recompute distances
	  dist_i1_j0 = (p2.x[j] - p1.x[i+1])*(p2.x[j] - p1.x[i+1]) +
	               (p2.y[j] - p1.y[i+1])*(p2.y[j] - p1.y[i+1]);
	  if (j + 1 == length2)
	    dist_i0_j1 = (p2.x[0] - p1.x[i])*(p2.x[0] - p1.x[i]) +
	                 (p2.y[0] - p1.y[i])*(p2.y[0] - p1.y[i]);
	  else
	    dist_i0_j1 = (p2.x[j+1] - p1.x[i])*(p2.x[j+1] - p1.x[i]) +
	                 (p2.y[j+1] - p1.y[i])*(p2.y[j+1] - p1.y[i]);
	}

      // now for the last points, if they were not reached because one iterator ended before the other:
      // // TODO there is some problem here, since the while loops up to length1 -1 or length2 -1 !!!
      if (j < length2)
	{
	  for (k=j; k<length2 -1; k++)
	    {
	      sum += /*dist_i0_j1 == */ (p2.x[k+1] - p1.x[i])*(p2.x[k+1] - p1.x[i]) +
		                        (p2.y[k+1] - p1.y[i])*(p2.y[k+1] - p1.y[i]);
	    }
	}

      if (i < length1)
	{
	  for (k=i; k<length1 -1; k++)
	    {
	      sum += /*dist_i1_j0 ==*/ (p2.x[j] - p1.x[k+1])*(p2.x[j] - p1.x[k+1]) +
		                       (p2.y[j] - p1.y[k+1])*(p2.y[j] - p1.y[k+1]);
	    }
	}

      // record value, if lower than previously recorded one:
      if (sum < min_sum)
	{
	  min_sum = sum;
	  start = s;
	}

    } // loop over s

  return start;
}

/** A recursive function to find the min_j for the minimum distance between p1 and p2.
 *  Uses a divide and conquer approach: given the interval length, it will recurse using halfs of it.
 *  Puts more than 65 minutes to 5 minutes !!!! Awesome. But it's not perfect:
 *  	- ends may be computed twice
 *  	- one of the planes was not aligned correctly in the thresholded, hard model.
 */
MinDist
MorphCurve::findMinDist(Perimeter p1, Perimeter p2,
			double delta, int first, int last, int interval_length,
			MinDist result)
{
  double** matrix = result.matrix;
  double** matrix_e = result.matrix_e; // editable matrix
  double** matrix1 = matrix_e;
  double** matrix2 = matrix;
  
  // the iterator over p2
  int j;

  // size of the interval to explore
  int k = 0;
  int length;
  if (last < first)
    length = p2.length - first + last;
  else
    length = last - first + 1;

  // gather data
  int n = p1.length;
  int m = p2.length;
  int min_j = result.min_j;
  double min_dist = result.min_dist;

  while (k < length)
    {
      j = first + k;
      // correct circular array
      if (j >= p2.length)
	j = j - p2.length;

      // don't do some twice: TODO this setup does not save the case when the computation was done not in the previous iteration but before.
      if (j == result.min_j)
	matrix_e = result.matrix_e;
      else
	matrix_e = findEditMatrix(p1, p2, j, delta, matrix_e);
    
      if (matrix_e[n][m] < min_dist)
	{
	  // record values
	  min_j = j;
	  matrix = matrix_e;
	  min_dist = matrix[n][m];
	  // swap editable matrix
	  if (*matrix_e == *matrix1)
	    //dereference, to compare the addresses of the arrays.
	    matrix_e = matrix2;
	  else
	    matrix_e = matrix1;
	}
		
      // advance iterator
      if (length -1 != k && k + interval_length >= length)
	// do the last one (and then finish)
	k = length -1;
      else
	k += interval_length;      
    }

  MinDist res;
  res.min_j = min_j;
  res.min_dist = min_dist;
  res.set(matrix, matrix_e, n+1, m+1);

  if (interval_length > 1)
    {
      first = min_j - (interval_length -1);
      last = min_j + (interval_length -1);
      // correct first:
      if (first < 0)
	first = p2.length + first; // a '+' so it is substracted from p2.length

      // correct last:
      if (last >= p2.length)
	last -= p2.length;

      // recurse, with half the interval length
      interval_length = (int) ceil(interval_length / 2.0f);
      return findMinDist(p1, p2, delta, first, last, interval_length, res);
    }

  // otherwise done!
  return res;
}

/** Find the minimum edit distance between Perimeter p1 and p2.
 * Returns the matrix as returned by findEditMatrix().
 */
double**
MorphCurve::findMinimumEditDistance(Perimeter& p1, Perimeter& p2, double delta, int is_closed_curve)
{

  // iterators
  int j, i;
  
  int min_j = 0;
  int n = p1.length;
  int m = p2.length;

  // allocate the first matrix
  double** matrix1 = new double*[n+1];
  for(int i=0; i<n+1; i++)
    {
      matrix1[i] = new double[m+1];
      memset(matrix1[i], 0, (m+1)*sizeof(double));
      // make the first element be i*delta
      matrix1[i][0] = i * delta;
    }

  // make the first j row be j * delta
  for (j=0; j < m + 1; j++)
    matrix1[0][j] = j * delta;

  // return the matrix made matching point 0 of both curves, if the curve is open.
  if (is_closed_curve == 0)
    return findEditMatrix(p1, p2, 0, delta, matrix1);

  // else try every point in the second curve to see which one is the best possible match.
  
  // allocate the second matrix
  double** matrix2 = new double*[n+1];
  for(int i=0; i<n+1; i++)
    {
      matrix2[i] = new double[m+1];
      memset(matrix2[i], 0, (m+1)*sizeof(double));
      // make the first element be i*delta
      matrix2[i][0] = i * delta;
    }

  // make the first j row be j * delta
  for (j=0; j < m + 1; j++)
    matrix2[0][j] = j * delta;

  // Store the distances, to trace a curve and find the different valleys.

  // A 'divide and conquer' approach:
  // Find the value of one every 10% of points. Then find those intervals with the lowest starting and ending values, and then look for 50% intervals inside those, and so on, until locking into the lowest value. It will save about 80% or more of all computations.
  MinDist mindata;
  mindata.min_j = -1;
  mindata.min_dist = DLARGE;
  mindata.ndim = n+1;
  mindata.mdim = m+1;
  mindata.matrix = matrix2;
  mindata.matrix_e = matrix1;

  MinDist min_data = findMinDist(p1, p2, delta,
				 0, m-1, (int) ceil(m * 0.1), mindata);

  min_j = min_data.min_j;

  // the one to keep
  double** matrix = new double*[n+1];
  for(int i=0; i<n+1; i++)
    {
      matrix[i] = new double[m+1];
      memcpy(matrix[i], min_data.matrix[i], (m+1)*sizeof(double));
    }

  // Reorder the second array, so that min_j is index zero (i.e. simply making both curves start at points that are closest to each other in terms of curve similarity).
  if (min_j != 0)
    {
      QVector<double> tmp;
      tmp.resize(m);
      QVector<double> src;

      // x
      src = p2.x;
      for (i=0, j=min_j; j<m; i++, j++) { tmp[i] = src[j]; }
      for (j=0; j<min_j; i++, j++) { tmp[i] = src[j]; }
      p2.x = tmp;

      // y
      src = p2.y;
      for (i=0, j=min_j; j<m; i++, j++) { tmp[i] = src[j]; }
      for (j=0; j<min_j; i++, j++) { tmp[i] = src[j]; }
      p2.y = tmp;

      // v_x
      src = p2.v_x;
      for (i=0, j=min_j; j<m; i++, j++) { tmp[i] = src[j]; }
      for (j=0; j<min_j; i++, j++) { tmp[i] = src[j]; }
      p2.v_x = tmp;

      // v_y
      src = p2.v_y;
      for (i=0, j=min_j; j<m; i++, j++) { tmp[i] = src[j]; }
      for (j=0; j<min_j; i++, j++) { tmp[i] = src[j]; }
      p2.v_y = tmp;

      // release
      tmp.clear();
    }

  return matrix;
}

// Makes an enlarged copy of the given editions array.
QVector<int>
MorphCurve::resizeAndFillEditionsCopy(QVector<int> editions, int ed_length, int new_length)
{
  //check: 
  if ( new_length <= ed_length)
    {
      QMessageBox::information(0, "", "editions array not resized, new_length is shorter or equal.");
      return editions;
    }

  QVector<int> editions2;
  editions2.resize(3*new_length);

  for(int i=0; i<3*ed_length; i++)
    editions[i] = editions[i];

  return editions2;
}

/** Find the optimal path in the matrix.
 *  Returns a 2D array such as [n][3]:
 *  	- the second dimension records:
 *  		- the edition:
 *  			1 == DELETION
 *  			2 == INSERTION
 *  			3 == MUTATION
 *  		- the i
 *  		- the j
 * 
 * Find the optimal edit sequence, and also reorder the second array to be matchable with the first starting at index zero of both. This method, applied to all consecutive pairs of curves, reorders all but the first (whose zero will be used as zero for all) of the non-interpolated perimeters (the originals) at findMinimumEditDistance.
 * */
Editions
MorphCurve::findOptimalEditSequence(Perimeter p1, Perimeter p2,
				    double delta, int is_closed_curve)
{
  // fetch the optimal matrix:
  double** matrix = findMinimumEditDistance(p1, p2, delta, is_closed_curve);

  int n = p1.length;
  int m = p2.length;
  int initial_length = sqrtf((n*n)+(m*m));
  int i = 0;
  QVector<int> editions;
  editions.resize(3*initial_length);
  
  int ed_length = initial_length;
  int next = 0;
  i = n;
  int j = m;
  int k;
  double error = 0.00001; //for floating-point math.
  while (0 != i && 0 != j)
    { // the matrix is n+1 x m+1 in size
      // check editions array
      if (next == ed_length)
	{
	  //enlarge editions array
	  editions = resizeAndFillEditionsCopy(editions, ed_length, ed_length + 20);
	  ed_length += 20;
	}
      // find next i, j and the type of transform:
      if (error > qAbs(matrix[i][j] - matrix[i-1][j] - delta))
	{
	  // a deletion:
	  editions[3*next+0] = DELETION;
	  editions[3*next+1] = i;
	  editions[3*next+2] = j;
	  i = i-1;
	}
      else if (error > qAbs(matrix[i][j] - matrix[i][j-1] - delta))
	{
	  // an insertion:
	  editions[3*next+0] = INSERTION;
	  editions[3*next+1] = i;
	  editions[3*next+2] = j;
	  j = j-1;
	}
      else
	{
	  // a mutation (a trace between two elements of the string of vectors)
	  editions[3*next+0] = MUTATION;
	  editions[3*next+1] = i;
	  editions[3*next+2] = j;
	  i = i-1;
	  j = j-1;
	}
      //prepare next
      next++;
    }

  // add unnoticed insertions/deletions. It happens if 'i' and 'j' don't reach the zero value at the same time.
  if (j != 0)
    {
      for (k=j; k>-1; k--)
	{
	  if (next == ed_length)
	    {
	      //enlarge editions array
	      editions = resizeAndFillEditionsCopy(editions, ed_length, ed_length + 20);
	      ed_length += 20;
	    }
	  editions[3*next+0] = INSERTION;
	  editions[3*next+1] = 0;
	  editions[3*next+2] = k;
	  next++;
	}
    }
  if (i != 0)
    {
      for (k=i; k>-1; k--)
	{
	  if (next == ed_length)
	    {
	      //enlarge editions array
	      editions = resizeAndFillEditionsCopy(editions, ed_length, ed_length + 20);
	      ed_length += 20;
	    }
	  editions[3*next+0] = DELETION;
	  editions[3*next+1] = k;
	  editions[3*next+2] = 0;
	  next++;
	}
    }

  // override edition at zero! A mutation by definition
  // check editions array
  //editions[next-1][0] = MUTATION;
  //editions[next-1][1] = 0;
  //editions[next-1][2] = 0;
  


  // reverse and resize editions array, and DO NOT slice out the last element.
  QVector<int> editions2;
  if (next != ed_length)
    {
      // allocate a new editions array  ('next' is the length now, since it is the index of the element after the last element)
      editions2.resize(3*next);
      for (i=0, j=next-1; i<next; i++, j--)
	{
	  editions2[3*i+0] = editions[3*j+0];
	  editions2[3*i+1] = editions[3*j+1];
	  editions2[3*i+2] = editions[3*j+2];
	}
      // release old editions array
      editions.clear();
    }
  else
    {
      //simply reorder in place
      for (i=0; i<next; i++)
	{
	  int temp = editions[3*i+0];
	  editions[3*i+0] = editions[3*(next-1-i)+0];
	  editions[3*(next-1-i)+0] = temp;

	  temp = editions[3*i+1];
	  editions[3*i+1] = editions[3*(next-1-i)+1];
	  editions[3*(next-1-i)+1] = temp;

	  temp = editions[3*i+2];
	  editions[3*i+2] = editions[3*(next-1-i)+2];
	  editions[3*(next-1-i)+2] = temp;
	}
      editions2 = editions;
    }

  // return in a struct:
  Editions e;
  e.editions = editions2;
  e.length = next;
  e.distance = matrix[n][m];

  // release the matrix
  for (i=0; i<n+1; i++)
    delete [] matrix[i];
  delete [] matrix;
  
  return e;
}

/** Returns a perimeter morphed from p1 to p2 by the weight specified as alpha, where: 0 < alpha < 1.0.
 *
 *	TODO: there is I believe still a fundamental problem: if the first point is fixed, what sense does it make to apply the editions of the first point to the curve? This oddity is I believe patched by the fact that when subsampling, the vector used to make a point is stored with the same index of that point, instead of with the previous point. The whole thing results in a somewhat balanced algorithm, but then what other explanation there is for, in some intances, having the interpolated curves slightly displaced from where they should be?
 *	I have tried to store the vectors in the previous index, as it should be, and to ignore the first edition. This approach is wrong in that the first edition is not necessarily the only edition affecting the first point. If the vectors were to be stored properly in the previous index, so that any given point can be generated by the previous point at 'i' plus the vector at 'i' as well, then the reading of the editions below should take into account that editions over i=0 or j=0 should be either blatanly ignored, or added to the last point.
 *
 * 'alpha' is the weight.
 * 
 * 
 * */
Perimeter
MorphCurve::getMorphedPerimeter(Perimeter p1, Perimeter p2,
				QVector<int> editions, int n_editions, double alpha)
{
  // catch
  int n = p1.length;
  int m = p2.length;

  // the weighted vectors to generate:
  double vs_x = 0; // no need to store them in arrays.
  double vs_y = 0;

  // the points to create. There is one point for each edition.
  QVector<double> x;
  x.resize(n_editions+1);
  QVector<double> y;
  y.resize(n_editions+1);
  //starting point: a weighted average between both starting points

  x[0] = (p1.x[0] * (1-alpha) + p2.x[0] * alpha);
  y[0] = (p1.y[0] * (1-alpha) + p2.y[0] * alpha);

  // the iterators
  int next = 0;
  int i;
  int j;
  int e;

  // generate weighted vectors to make weighted points


  //// PATCH to avoid displacement of the interpolated curves when edition[1][0] is a MUTATION. //////
  int start = 0;
  int end = n_editions; // was: -1
  /*
    if (MUTATION == editions[1][0]) {
    start = 0;
    end = n_editions-1;
    }
  */
  if (INSERTION == editions[0] || DELETION == editions[0])
    {
      start = 1;
      end = n_editions;
    }
  
  for (e=start; e<end; e++)
    {
      i = editions[3*e+1];
      j = editions[3*e+2];

      // check for deletions and insertions at the lower-right edges of the matrix:
      if (i == n)
	i = 0; //i = n-1; // zero, so the starting vector is applied.
      if (j == m)
	  j = 0; //j = m-1;

      // do:
      switch (editions[3*e+0])
	{
	case INSERTION:
	  vs_x = p2.v_x[j] * alpha;
	  vs_y = p2.v_y[j] * alpha;
	  break;
	case DELETION:
	  vs_x = p1.v_x[i] * (1.0 - alpha);
	  vs_y = p1.v_y[i] * (1.0 - alpha);
	  break;
	case MUTATION:
	  vs_x = p1.v_x[i] * (1.0 - alpha) + p2.v_x[j] * alpha;
	  vs_y = p1.v_y[i] * (1.0 - alpha) + p2.v_y[j] * alpha;
	  break;
	default:
	  QMessageBox::information(0, "", "\ngetMorphedPerimeter: Nothing added!");
	  break;
	}

      if (next+1 > n_editions/*-1*/)
	QMessageBox::information(0, "", "\nWARNING: out of bounds at 1259");

      // store the point
      x[next+1] = x[next] + vs_x;
      y[next+1] = y[next] + vs_y;

      // advance
      next++;
    }

  // return as a Perimeter
  Perimeter p;
  p.x = x;
  p.y = y;
  p.length = next; //next works as a length
  p.z = p1.z + alpha * (p2.z - p1.z); // alpha * z_diff !! Otherwise the curves are packed within one z level. 
  p.editions.clear();

  return p;
}

/* Returns as many as n_interpolates perimeters packed in a Perimeter* array, interpolated by weighted strings of vectors using the Jiang-Bunke-Wagner method.
 * The perimeters are first subsampled so that the point interdistance becomes delta.
 * An n_morphed_perimeters of -1 means automatic mode (proportional to the Levenshtein's distance between both perimeters).
 * */
Result*
MorphCurve::getMorphedPerimeters(Perimeter p1_, Perimeter p2_,
				 int n_morphed_perimeters, double delta, int is_closed_curve)
{
  //check preconditions:
  if (n_morphed_perimeters < -1 ||
      delta <= 0 ||
      p1_.length <= 0 || p2_.length <=0)
    {
      QMessageBox::information(0, "", QString("\nERROR: args are not acceptable at getMorphedPerimeters:\n\t n_morphed_perimeters %1, delta %2, p1->length %3, p2->length %4").arg(n_morphed_perimeters).arg(delta).arg(p1_.length).arg(p2_.length));
      return NULL;
    }

  Perimeter p1 = subsample(p1_, delta);
  Perimeter p2 = subsample(p2_, delta);

  Editions ed = findOptimalEditSequence(p1, p2, delta, is_closed_curve);
  QVector<int> editions = ed.editions;
  int n_editions = ed.length;

  // check automatic mode:
  if (n_morphed_perimeters == -1)
    n_morphed_perimeters = (int)(sqrt(sqrt(ed.distance)));

  // Store the editions array in p1 ( so not freeing it below)
  // to make the skin by string match
  p1.editions << ed;

  if (n_morphed_perimeters == 0)
    {
      return NULL;
    }

  // so we loop over n_morphed_perimeters curves to generate. The n_interpolates parameter has been used to compute the number of n_morphed_perimeters, multiplied by the z_diff (so if there are any z levels without a curve, they will be interpolated as well, without streching the distance between interpolated curves).
  
  double alpha = 1.0 / (n_morphed_perimeters +1); // to have 10 subdivisions we need 11 boxes
  QList<Perimeter> p_list;

  int a;
  double aa;
  for (a=0; a<n_morphed_perimeters; a++)
    {
      aa = alpha * (a+1); // aa 0 would be p1, aa 1 would be p2.
      p_list << getMorphedPerimeter(p1, p2, editions, n_editions, aa);
    }

  Result* r = new Result;
  r->p = p_list;
  r->length = n_morphed_perimeters;
  return r;
}

/** Needs an array of Perimeter structs pointers and returns a larger array that includes the given perimeters (subsampled to the average point interdistance of them all) and the weighted averages in between.
 * The resuls are packed in a Result struct, so that the length (the number of perimeters) is also known.
 *  This is the main function of the algorithm for the pure C side.
 */
Result*
MorphCurve::getAllPerimeters(QList<Perimeter> perimeters,
			     int n_perimeters, int n_morphed_perimeters,
			     double delta_, int is_closed_curve)
{
  //check preconditions:
  if (n_morphed_perimeters < -1 || n_perimeters <=0)
    {
      QMessageBox::information(0, "", QString("\nERROR: args are not acceptable at getAllPerimeters:\n\t n_morphed_perimeters %1, n_perimeters %2").arg(n_morphed_perimeters).arg(n_perimeters));
      return NULL;
    }

  QProgressDialog progress(QString("Morphing Curves"),
			   "",
			   0, 100,
			   0);
  progress.setMinimumDuration(0);

  // get all morphed curves and return them.
  int i, j;

  double delta = 0.0;
  // calculate delta, or taken the user-given one if acceptable (TODO acceptable is not only delta > 0, but also smaller than half the curve length or so.
  if (delta_ > 0)
    delta = delta_;
  else
    {
      for (i=0; i<n_perimeters; i++)
	delta += getAndSetAveragePointInterdistance(perimeters[i]);
      delta = delta / n_perimeters;
    }
  
  // subsample perimeters so that point interdistance becomes delta
  for (i=0; i<n_perimeters; i++)
    perimeters[i] = subsample(perimeters[i], delta);


  // allocate space for all_perimeters storage
  QList<Perimeter> all_perimeters;

  // fetch morphed ones and fill all_perimeters array:
  int next = 0;
  for (i=1; i<n_perimeters; i++)
    {
      all_perimeters << perimeters[i-1];
      next++;

      progress.setValue((int)(100*(float)i/(float)n_perimeters));
      progress.setLabelText(QString("morphing curves between %1 and %2").\
			    arg(perimeters[i-1].z).\
			    arg(perimeters[i].z));
      qApp->processEvents();

      // get every morphed curve
      int nmp = qAbs(perimeters[i-1].z - perimeters[i].z);      
      if (n_morphed_perimeters > 0)
	nmp = n_morphed_perimeters;
      Result* morphed_perimeters = getMorphedPerimeters(perimeters[i-1], perimeters[i],
							nmp, delta, is_closed_curve);

      if (morphed_perimeters != NULL && morphed_perimeters->length > 0)
	{
//	  QMessageBox::information(0, "",
//				   QString("%1 morphed perimeters between z1=%2 and z2=%3").\
//				   arg(morphed_perimeters->length).\
//				   arg(perimeters[i-1].z).\
//				   arg(perimeters[i].z));
	  for (j=0; j<morphed_perimeters->length; j++)
	    all_perimeters << morphed_perimeters->p[j];
	  delete morphed_perimeters;
	}
      else
	{
	  QMessageBox::information(0, "", QString("No morphed perimeters between z1=%1 and z2=%2").\
				   arg(perimeters[i-1].z).arg(perimeters[i].z));
	}
    }
  // append the last one
  all_perimeters << perimeters[n_perimeters -1];

  // release the vector arrays in the perimeters, as they are no longer relevant
  // (only the original perimeters will have their vector arrays filled)
  for (i=0; i<n_perimeters; i++)
    {
      perimeters[i].v_x.clear();
      perimeters[i].v_y.clear();
      perimeters[i].subsampled = 0; //used as a flag to indicate if vectors exist
    }

  progress.setValue(100);

  // Done!
  Result* result = new Result;
  result->p = all_perimeters;
  result->length = next;
  return result;
}




