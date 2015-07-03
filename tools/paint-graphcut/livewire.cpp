#include "livewire.h"
#include "global.h"

#include <QMessageBox>

LiveWire::LiveWire()
{
  m_Oimage = 0;
  m_image = 0;
  m_grad = 0;
  m_normal = 0;
  m_tmp = 0;
  m_edgeWeight.clear();
  m_cost.clear();
  m_prev.clear();

  m_gradImage = QImage(100, 100, QImage::Format_Indexed8);

  m_Owidth = m_Oheight = 0;
  m_width = m_height = 0;

  m_seedpos.clear();
  m_seeds.clear();
  m_poly.clear();
  m_livewire.clear();

  m_propagateLivewire = false;
  m_guessCurve.clear();
  m_guessSeeds.clear();

  m_gradType = 0;
  m_smoothType = 0;

  m_wtG = 0.7;
  m_wtN = 0.3;

  m_useDynamicTraining = true;
  m_gradCost = 0;

  m_seedMoveMode = false;
  m_activeSeed = -1;
  m_polyA.clear();
  m_polyB.clear();
  m_closed = true;
  m_type = 0;

  m_lod = 1;
}

bool LiveWire::seedMoveMode() { return m_seedMoveMode; }

void
LiveWire::setSeedMoveMode(bool b)
{
  m_seedMoveMode = b;
  m_activeSeed = -1;
}

void
LiveWire::setGuessCurve(QVector<QPointF> gc, QVector<QPointF> gs)
{
  m_guessCurve = gc;
  m_guessSeeds = gs;
}
// renew guess curve from current livewire polygon
void
LiveWire::renewGuessCurve()
{
  m_guessCurve = m_poly;
  
  m_guessSeeds.clear();
  if (m_seedpos.count() > 0)
    {
      for(int i=0; i<m_seedpos.count(); i++)
	m_guessSeeds << m_poly[m_seedpos[i]];
    }
}

bool LiveWire::propagateLivewire() { return m_propagateLivewire; }
void
LiveWire::setPropagateLivewire(bool b)
{
  m_propagateLivewire = b;
  m_guessCurve.clear();
  m_guessSeeds.clear();
}

bool LiveWire::closed() { return m_closed; }
uchar LiveWire::type() { return m_type; }
QVector<QPointF> LiveWire::seeds() { return m_seeds; }
void
LiveWire::setPolygonToUpdate(QVector<QPointF> pts,
			     QVector<int> seedpos,
			     bool closed,
			     uchar type,
			     QVector<QPointF> seeds)
{
  m_seedMoveMode = true;
  m_activeSeed = -1;
  m_poly = pts;
  m_seedpos = seedpos;  
  m_seeds = seeds;
  m_closed = closed;
  m_type = type;
  m_livewire.clear();
}

void LiveWire::setUseDynamicTraining(bool b) { m_useDynamicTraining = b; }

void LiveWire::setWeightG(float w) { m_wtG = w; }
void LiveWire::setWeightN(float w) { m_wtN = w; }

LiveWire::~LiveWire() { reset(); }

void
LiveWire::resetPoly()
{
  m_seeds.clear();
  m_seedpos.clear();
  m_poly.clear();
  m_livewire.clear();
}

void
LiveWire::reset()
{
  if (m_Oimage) delete [] m_Oimage;
  if (m_image) delete [] m_image;
  if (m_grad) delete [] m_grad; 
  if (m_normal) delete [] m_normal;
  if (m_tmp) delete [] m_tmp;
  if (m_gradCost) delete [] m_gradCost;

  m_image = 0;
  m_grad = 0;
  m_normal = 0;
  m_tmp = 0;
  m_gradCost = 0;
  m_edgeWeight.clear();
  m_cost.clear();
  m_prev.clear();

  m_width = m_height = 0;
  m_Owidth = m_Oheight = 0;

  m_seedpos.clear();
  m_seeds.clear();
  m_poly.clear();
  m_livewire.clear();

  m_propagateLivewire = false;
  m_guessCurve.clear();
  m_guessSeeds.clear();

  m_gradCost = new float[256];
  for(int i=0; i<256; i++)
    {
      float gc = 1.0 - (float)i/255.0;
      // scale gc;
      gc = (3*gc*gc - 2*gc*gc*gc);
      m_gradCost[i] = gc;
    }

  m_lod = 1;
}

QVector<QPointF> LiveWire::poly() { return m_poly; }
QVector<int> LiveWire::seedpos() { return m_seedpos; }
QVector<QPointF> LiveWire::livewire() { return m_livewire; }

void
LiveWire::setLod(int lod)
{
  if (m_lod == lod)
    return;

  m_lod = qMax(1,lod);

  if (m_image) delete [] m_image;
  if (m_grad) delete [] m_grad;
  if (m_normal) delete [] m_normal;

  m_width = m_Owidth/m_lod;
  m_height = m_Oheight/m_lod;

  m_image = new uchar[m_width*m_height];
  m_grad = new float[sizeof(float)*m_width*m_height];  
  m_normal = new float[2*sizeof(float)*m_width*m_height];  
  
  m_edgeWeight.clear();
  m_cost.clear();
  m_prev.clear();
  m_edgeWeight.resize(8*m_width*m_height);
  m_cost.resize(m_width*m_height);
  m_prev.resize(m_width*m_height);


  if (m_lod == 1)
    memcpy(m_image, m_Oimage, m_width*m_height);
  else
    {
      //applySmoothing(m_Oimage, m_Owidth, m_Oheight, m_lod-1);
      for(int i=0; i<m_height; i++)
	for(int j=0; j<m_width; j++)
	  m_image[i*m_width+j] = m_Oimage[m_lod*(i*m_Owidth+j)];
    }
	   
  applySmoothing(m_image, m_width, m_height, m_smoothType);
  calculateGradients();
  calculateEdgeWeights();
}

void
LiveWire::setImageData(int w, int h, uchar *img)
{
  //if (m_width != w || m_height != h)
  if (m_Owidth != w ||
      m_Oheight != h ||
      m_width != w/m_lod ||
      m_height != h/m_lod)
    {
      if (m_Oimage) delete [] m_Oimage;
      if (m_image) delete [] m_image;
      if (m_grad) delete [] m_grad;
      if (m_normal) delete [] m_normal;
      if (m_tmp) delete [] m_tmp;

      m_Owidth = w;
      m_Oheight = h;
      m_Oimage = new uchar[m_Owidth*m_Oheight];

      m_width = w/m_lod;
      m_height = h/m_lod;

      m_image = new uchar[m_width*m_height];
      m_grad = new float[sizeof(float)*m_width*m_height];  
      m_normal = new float[2*sizeof(float)*m_width*m_height];  
      m_tmp = new uchar[4*m_Owidth*m_Oheight];  

      m_edgeWeight.clear();
      m_cost.clear();
      m_prev.clear();
      m_edgeWeight.resize(8*m_width*m_height);
      m_cost.resize(m_width*m_height);
      m_prev.resize(m_width*m_height);
    }

  memcpy(m_Oimage, img, m_Owidth*m_Oheight);
  if (m_lod == 1)
    memcpy(m_image, img, m_width*m_height);
  else
    {
      //applySmoothing(m_Oimage, m_Owidth, m_Oheight, m_lod-1);
      for(int i=0; i<m_height; i++)
	for(int j=0; j<m_width; j++)
	  m_image[i*m_width+j] = m_Oimage[m_lod*(i*m_Owidth+j)];
    }
	   
  applySmoothing(m_image, m_width, m_height, m_smoothType);
  calculateGradients();
  calculateEdgeWeights();
}

void
LiveWire::mouseReleaseEvent(QMouseEvent *event)
{
  m_activeSeed = -1;
}

bool
LiveWire::mousePressEvent(int xpos, int ypos, QMouseEvent *event)
{
  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;

  if (m_seedMoveMode)
    {
      if (shiftModifier && event->button() == Qt::LeftButton)
	m_activeSeed = insertSeed(xpos, ypos);
      else if (shiftModifier && event->button() == Qt::RightButton)
	removeSeed(xpos, ypos);
      else
	m_activeSeed = getActiveSeed(xpos, ypos);

      return true;
    }

  int button = event->button();
  
//  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
//  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
//  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (button == Qt::LeftButton)
    {
      m_poly += m_livewire;
      m_poly << QPointF(xpos, ypos);

      m_seedpos << m_poly.count()-1;

      m_livewire.clear();
      calculateCost(xpos, ypos, 500);

      return true;
    }

  if (button == Qt::RightButton)
    {
      for(int i=m_poly.count()-1; i>=0; i--)
	{
	  if ((m_poly[i]-QPointF(xpos, ypos)).manhattanLength() < Global::selectionPrecision())
	    {
	      m_poly.remove(i, m_poly.count()-i);

	      //---------------
	      // update seeds to accommodate the removal of points
	      int pc = m_poly.count();
	      for(int j=m_seedpos.count()-1; j>=0; j--)
		{
		  if (m_seedpos[j] < pc)
		    {
		      m_seedpos.remove(j, m_seedpos.count()-j);
		      m_seedpos << m_poly.count()-1;
		      break;
		    }
		}
	      //---------------

	      m_livewire.clear();
	      calculateCost(xpos, ypos, 500);
	      break;
	    }
	}
      return true;
    }

  return false;
}

bool
LiveWire::mouseMoveEvent(int xpos, int ypos, QMouseEvent *event)
{
//  int button = event->button();
//  
//  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
//  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
//  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (m_seedMoveMode)
    {
      if (event->buttons() == Qt::LeftButton)
	updateLivewireFromSeeds(xpos, ypos);
      return true;
    }

  if (m_poly.count() > 0)
    {
      calculateLivewire(xpos, ypos);
      return true;
    }
  
  return false;
}

void
LiveWire::freeze()
{
  // close the livewire by connecting to the first point
  if (m_poly.count() > 0)
    {
      int xpos = m_poly[0].x();
      int ypos = m_poly[0].y();
      m_livewire.clear();
      calculateLivewire(xpos, ypos);
      m_poly += m_livewire;
    }
}

bool
LiveWire::keyPressEvent(QKeyEvent *event)
{
  bool shiftModifier = event->modifiers() & Qt::ShiftModifier;
  bool ctrlModifier = event->modifiers() & Qt::ControlModifier;
  bool altModifier = event->modifiers() & Qt::AltModifier;

  if (event->key() == Qt::Key_Escape)
    {
      resetPoly();
      return true;
    }
    
  return false;
}

QImage
LiveWire::gradientImage()
{
  if (m_lod == 1)
    return m_gradImage;
  else
    return m_gradImage.scaled(m_Owidth, m_Oheight,
			      Qt::IgnoreAspectRatio,
			      Qt::SmoothTransformation);
}

void
LiveWire::applySmoothing(uchar *img, int w, int h, int sz)
{
  if (sz == 0) return; // no smoothing

  memcpy(m_tmp, img, w*h);
  gaussBlur_4(m_tmp, img, w, h, sz);
}

void
LiveWire::calculateGradients()
{
//  int sobel_x[3][3] = {{-1,  0,  1},
//			 {-2,  0,  2},
//			 {-1,  0,  1}, };
//
//  int sobel_y[3][3] = {{-1, -2, -1},
//			 { 0,  0,  0},
//			 { 1,  2,  1}, };  

  memset(m_grad, 0, sizeof(float)*m_width*m_height);
  memset(m_normal, 0, 2*sizeof(float)*m_width*m_height);  
  memset(m_tmp, 0, 4*m_width*m_height);

  float maxGrad = 0;
  float minGrad = 1000000;

  // central difference
  if (m_gradType == 0)
    {
      for(int i=1; i<m_height-1; i++)
	for(int j=1; j<m_width-1; j++)
	  {	
	    float dx = 0;
	    float dy = 0;
	    {
	      dx += (m_image[(i+1)*m_width+j] -
		     m_image[(i-1)*m_width+j]);
	      
	      dy += (m_image[i*m_width+(j+1)] -
		     m_image[i*m_width+(j-1)]);
	    }
	    
	    float grd = qSqrt(dx*dx + dy*dy);

	    // calculate normal cost
	    if (qAbs(grd) > 0)
	      {
		dx /= grd;
		dy /= grd;
		m_normal[2*(i*m_width+j)] = dy;
		m_normal[2*(i*m_width+j)+1] = -dx;
	      }	    

	    maxGrad = qMax(maxGrad, grd);
	    minGrad = qMin(minGrad, grd);
	    m_grad[i*m_width+j] = grd;
	  }
    }
      

  // sobel
  if (m_gradType == 1)
    {
      for(int i=1; i<m_height-1; i++)
	for(int j=1; j<m_width-1; j++)
	  {	
	    float dx = 0;
	    float dy = 0;
	    {
	      dx += (m_image[(i+1)*m_width+(j-1)] -
		     m_image[(i-1)*m_width+(j-1)]);
	      dx += 2*(m_image[(i+1)*m_width+j] -
		       m_image[(i-1)*m_width+j]);
	      dx += (m_image[(i+1)*m_width+(j+1)] -
		     m_image[(i-1)*m_width+(j+1)]);
	      
	      dy += (m_image[(i+1)*m_width+(j+1)] -
		     m_image[(i+1)*m_width+(j-1)]);
	      dy += 2*(m_image[i*m_width+(j+1)] -
		       m_image[i*m_width+(j-1)]);
	      dy += (m_image[(i-1)*m_width+(j+1)] -
		     m_image[(i-1)*m_width+(j-1)]);
	    }
	    
	    float grd = qSqrt(dx*dx + dy*dy);

	    // calculate normal cost
	    if (qAbs(grd) > 0)
	      {
		dx /= grd;
		dy /= grd;
		m_normal[2*(i*m_width+j)] = dy;
		m_normal[2*(i*m_width+j)+1] = -dx;
	      }	    

	    maxGrad = qMax(maxGrad, grd);
	    minGrad = qMin(minGrad, grd);
	    m_grad[i*m_width+j] = grd;
	  }
    }

  if (maxGrad > minGrad)
    {
      for(int i=0; i<m_width*m_height; i++)
	m_grad[i] = (m_grad[i]-minGrad)/(maxGrad-minGrad);
  

      for(int i=0; i<m_width*m_height; i++)
	{
	  int gidx = 255*(1-m_grad[i]);
	  int c = 200*m_gradCost[gidx];
	  m_tmp[4*i+0] = c;
	  m_tmp[4*i+1] = 0;
	  m_tmp[4*i+2] = c/2;
	  m_tmp[4*i+3] = c;
	}
    }

  m_gradImage = QImage(m_tmp,
		       m_width,
		       m_height,
		       QImage::Format_ARGB32);

}   

void
LiveWire::calculateEdgeWeights()
{  
  for(int i=0; i<m_edgeWeight.count(); i++)
    m_edgeWeight[i] = 10000000;

  for(int i=0; i<m_height; i++)
    for(int j=0; j<m_width; j++)
      {
	int midx = i*m_width+j;
	for(int a=-1; a<=1; a++)
	  for(int b=-1; b<=1; b++)
	    {	      
	      int ia = i+a;
	      int jb = j+b;
	      if (ia < 0 || ia >= m_height ||
		  jb < 0 || jb >= m_width)
		{ }
	      else
		{
		  int idx = (a+1)*3+(b+1);
		  // 0  1  2
		  // 3  4  5
		  // 6  7  8
		  float scl = 1;
		  if (idx%2 == 0) scl = 1.414; // scale for diagonal links

		  if (idx != 4)
		    {
		      if (idx > 4) idx--;
		      // 0  1  2         0  1  2
		      // 3  4  5   ==>   3     4
		      // 6  7  8         5  6  7
		      m_edgeWeight[8*midx + idx] = scl;
		    }
		}
	    }
      }
}

void
LiveWire::calculateCost(int wposO, int hposO, int boxSizeO)
{
  int wpos = wposO/m_lod;
  int hpos = hposO/m_lod;
  int boxSize = boxSizeO/m_lod;

  if (wposO < 0 || wposO >= m_Owidth ||
      hposO < 0 || hposO >= m_Oheight)
    return;

  for(int i=0; i<m_cost.count(); i++)
    m_cost[i] = 10000000.0;
  
  for(int i=0; i<m_prev.count(); i++)
    m_prev[i] = QPointF(-1, -1);

  // used as priority queue
  QMultiMap<float, QPointF> qmap;

  bool *visited = new bool[m_width*m_height];
  memset(visited, 0, m_width*m_height);

  int x = hpos;
  int y = wpos;
  m_cost[x*m_width + y] = 0;
  qmap.insert(0, QPointF(x, y));

  float pi23 = 2.0/(3.0*3.1415926535);
  
  while(qmap.count() > 0)
    {
      float key = qmap.firstKey();
      QList<QPointF> qpr = qmap.values(key);
      float dcost = key;
      int x = qpr[0].x();
      int y = qpr[0].y();
      qmap.remove(key, qpr[0]);
      if (qpr.count() == 1)
	qmap.remove(key);

      int midx = x*m_width+y;

      visited[midx] = true;      
      
      // visit all neighbours
      for(int a=-1; a<=1; a++)
	for(int b=-1; b<=1; b++)
	  {
	    if (x+a < hpos-boxSize || x+a >= hpos+boxSize ||
		y+b < wpos-boxSize || y+b >= wpos+boxSize ||
		x+a < 0 || x+a >= m_height ||
		y+b < 0 || y+b >= m_width)
	      {}
	    else
	      {
		int idx = (a+1)*3+(b+1);
		// 0  1  2
		// 3  4  5
		// 6  7  8
		if (idx != 4 && !visited[(x+a)*m_width+(y+b)])
		  {
		    if (idx > 4) idx--;
		    // 0  1  2         0  1  2
		    // 3  4  5   ==>   3     4
		    // 6  7  8	       5  6  7
		    float lx = (float)a/qSqrt(a*a+b*b);
		    float ly = (float)b/qSqrt(a*a+b*b);
		    float dp = (lx*m_normal[2*midx] +
				ly*m_normal[2*midx+1]);
		    float dq = (lx*m_normal[2*((x+a)*m_width+(y+b))] +
				ly*m_normal[2*((x+a)*m_width+(y+b))+1]);
		    float normalCost = pi23*(qAcos(dp)+qAcos(dq));

		    normalCost *= m_wtN*normalCost;
		    float gradCost = m_wtG*(1.0-m_grad[(x+a)*m_width+(y+b)]);
		    if (m_useDynamicTraining) // take value from gradCost
		      gradCost = m_wtG*m_gradCost[(int)(255*m_grad[(x+a)*m_width+(y+b)])];
		    float ewt = m_edgeWeight.at(8*midx + idx);
		    float newcost = dcost + ewt*(gradCost+normalCost);
		    float oldcost = m_cost.at((x+a)*m_width+(y+b)); 
		    if (newcost < oldcost)
		      {
			m_cost[(x+a)*m_width+(y+b)] = newcost;
			m_prev[(x+a)*m_width+(y+b)] = QPointF(x, y);
			
			qmap.insert(newcost, QPointF(x+a, y+b));
		      }
		  }
	      }
	  }
      
    }

  delete [] visited;
}


void
LiveWire::calculateLivewire(int wpos, int hpos)
{
  if (m_seedMoveMode && m_activeSeed < 0)
    return;

  if (wpos < 0 || wpos >= m_Owidth ||
      hpos < 0 || hpos >= m_Oheight)
    return;

  int x = hpos/m_lod;
  int y = wpos/m_lod;
  QVector<QPointF> pts;
  while(x > -1)
    {
      pts << QPointF(m_lod*y, m_lod*x);
      int idx = x*m_width+y;
      x = m_prev[idx].x();
      y = m_prev[idx].y();
    }

  m_livewire.clear();
  int pc = pts.count();
  for(int i=0; i<pc; i++)
    m_livewire << pts[pc-1-i];
}

// taken from http://blog.ivank.net/fastest-gaussian-blur.html
QVector<int>
LiveWire::boxesForGauss(float sigma, int n)  // standard deviation, number of boxes
{
  float wIdeal = qSqrt((12*sigma*sigma/n)+1);  // Ideal averaging filter width 
  int wl = wIdeal;
  if(wl%2==0) wl--;
  int wu = wl+2;
				
  float mIdeal = (12*sigma*sigma - n*wl*wl - 4*n*wl - 3*n)/(-4*wl - 4);
  int m = mIdeal;
  
  QVector<int> sizes;
  for(int i=0; i<n; i++)
    sizes << (i<m ? wl : wu);

  return sizes;
}
void
LiveWire::gaussBlur_4(uchar *scl, uchar *tcl,
		      int w, int h, int r)
{
  QVector<int> bxs = boxesForGauss(r, 3);
  boxBlur_4 (scl, tcl, w, h, (bxs[0]-1)/2);
  boxBlur_4 (tcl, scl, w, h, (bxs[1]-1)/2);
  boxBlur_4 (scl, tcl, w, h, (bxs[2]-1)/2);
}
void
LiveWire::boxBlur_4(uchar *scl, uchar *tcl,
		    int w, int h, int r)

{
  for(int i=0; i<w*h; i++)
    tcl[i] = scl[i];
  boxBlurH_4(tcl, scl, w, h, r);
  boxBlurT_4(scl, tcl, w, h, r);
}
void
LiveWire::boxBlurH_4(uchar *scl, uchar *tcl,
		     int w, int h, int r)
{
  float iarr = 1.0f/(r+r+1);
  for(int i=0; i<h; i++)
    {
      int ti = i*w;
      int li = ti;
      int ri = ti+r;
      int fv = scl[ti];
      int lv = scl[ti+w-1];
      int val = (r+1)*fv;
      for(int j=0; j<r; j++)
	val += scl[ti+j];
      for(int j=0; j<=r; j++)
	{
	  val += scl[ri++] - fv ;
	  tcl[ti++] = val*iarr;
	}
      for(int j=r+1; j<w-r; j++)
	{
	  val += scl[ri++] - scl[li++];
	  tcl[ti++] = val*iarr;
	}
      for(int j=w-r; j<w; j++)
	{
	  val += lv - scl[li++];
	  tcl[ti++] = val*iarr;
	}
  }
}
void
LiveWire::boxBlurT_4(uchar *scl, uchar *tcl,
		     int w, int h, int r)
{
  float iarr = 1.0f/(r+r+1);
  for(int i=0; i<w; i++)
    {
      int ti = i;
      int li = ti;
      int ri = ti+r*w;
      int fv = scl[ti];
      int lv = scl[ti+w*(h-1)];
      int val = (r+1)*fv;
      for(int j=0; j<r; j++)
	val += scl[ti+j*w];
      for(int j=0; j<=r; j++)
	{
	  val += scl[ri] - fv;
	  tcl[ti] = val*iarr;
	  ri+=w; ti+=w;
	}
    for(int j=r+1; j<h-r; j++)
      {
	val += scl[ri] - scl[li];
	tcl[ti] = val*iarr;
	li+=w; ri+=w; ti+=w;
      }
    for(int j=h-r; j<h; j++)
      {
	val += lv - scl[li];
	tcl[ti] = val*iarr;
	li+=w; ti+=w;
      }
  }
}

void
LiveWire::livewireFromSeeds(QVector<QPointF> oseeds)
{
  resetPoly();
  
  QVector<bool> used;
  used.fill(false, oseeds.count());
  QVector<QPointF> tseeds;

  // ----------
  // lineup all the seeds from orthogonal slices
  for(int i=0; i<m_guessCurve.count(); i++)
    {
      QPointF pt = m_guessCurve[i];
      for(int j=0; j<oseeds.count(); j++)
	{
	  if (!used[j])
	    {
	      if ((pt-oseeds[j]).manhattanLength()<5)
		{
		  used[j] = true;
		  tseeds << oseeds[j];
		}
	    }
	}
    }
  // ----------

  // ----------
  // use guessSeeds if no other seeds present
  if (tseeds.count() == 0)
    tseeds = m_guessSeeds;
  // ----------

  if (tseeds.count() == 0)
    {
      QMessageBox::information(0, "", "No seed point found for livewire propagation.");
      return;
    }

  // filter out closely placed seeds
  QVector<QPointF> seeds;
  seeds << tseeds[0];
  for(int i=1; i<tseeds.count()-1; i++)
    {
      QPointF v = seeds.last()-tseeds[i];
      if (QPointF::dotProduct(v,v) > 25)
	seeds << tseeds[i];
    }
  seeds << tseeds.last();

  
  // find nearest least energy positions for all seeds
  for(int i=0; i<seeds.count(); i++)
    {
      int h = seeds[i].x()/m_lod;
      int w = seeds[i].y()/m_lod;

      int h0=h;
      int w0=w;
      int xpos = h;
      int ypos = w;
      int midx = h*m_width+w;
      float minCost = 1.0-m_grad[midx]; // currently look at maximum gradient position
      for(int a=-2; a<=2; a++)
	for(int b=-2; b<=2; b++)
	  {
	    int midx = (qBound(0,(h+a), m_height-1)*m_width+
			qBound(0,(w+b),m_width-1)); 
	    float cst = 1.0-m_grad[midx]; // currently look at maximum gradient position
	    if (minCost > cst)
	      {
		minCost = cst;
		// replace seed
		seeds[i] = m_lod*QPointF(h+a, w+b);
	      }
	  }
    }

  for(int i=0; i<seeds.count(); i++)
    {
      m_poly += m_livewire;
      m_poly << seeds[i];

      m_seedpos << m_poly.count()-1;

      m_livewire.clear();

      int sz = 250;
      if (i < seeds.count()-1)
	sz = qMax(qAbs(seeds[i].x()-seeds[i+1].x()),
		  qAbs(seeds[i].y()-seeds[i+1].y()));
      else
	sz = qMax(qAbs(seeds[i].x()-m_poly[m_seedpos[0]].x()),
		  qAbs(seeds[i].y()-m_poly[m_seedpos[0]].y()));
	
      //sz*=1.5; // make a bigger sized cost matrix 
      sz += 25; // make a bigger sized cost matrix 

      calculateCost(seeds[i].x(),
		    seeds[i].y(), sz);

      if (i < seeds.count()-1)
	calculateLivewire(seeds[i+1].x(),
			  seeds[i+1].y());
    }
}

void
LiveWire::updateLivewireFromSeeds(int xpos, int ypos)
{
  if (m_activeSeed < 0)
    return;

  if (m_type > 0)
    {
      updateShapeFromSeeds(xpos, ypos);
      return;
    }

  int totseeds = m_seedpos.count();
  int ps = (m_activeSeed-1);
  int ns = (m_activeSeed+1)%totseeds;;
  if (ps < 0) ps = totseeds-1;
  
  m_poly[m_seedpos[m_activeSeed]] = QPointF(xpos, ypos);
  QPointF sps = m_poly[m_seedpos[ps]];
  QPointF sns = m_poly[m_seedpos[ns]];

  int sz0 = qMax(qAbs(xpos-sps.x()),
		 qAbs(ypos-sps.y()));
  int sz1 = qMax(qAbs(xpos-sns.x()),
		 qAbs(ypos-sns.y()));
  sz0 += 10;
  sz1 += 10;
  
  m_poly.clear();

  if (m_activeSeed == 0)
    {
      m_livewire.clear();
      calculateCost(xpos, ypos, sz1);
      calculateLivewire(sns.x(), sns.y());
      int lwlen = m_livewire.count();
      m_poly += m_livewire;
      int offset = m_poly.count()-1-m_seedpos[ns];
      m_poly += m_polyB;
      for(int i=ns; i<m_seedpos.count(); i++)
	m_seedpos[i] += offset;
      
      if (m_closed)
	{
	  m_livewire.clear();
	  calculateCost(sps.x(), sps.y(), sz0);
	  calculateLivewire(xpos, ypos);
	  m_poly += m_livewire;
	}
    }
  else
    {
      m_poly += m_polyA;

      m_livewire.clear();
      calculateCost(sps.x(), sps.y(), sz0);
      calculateLivewire(xpos, ypos);
      m_poly += m_livewire;
      int offset = m_poly.count()-1-m_seedpos[m_activeSeed];
      for(int i=m_activeSeed; i<m_seedpos.count(); i++)
	m_seedpos[i] += offset;

      if (!m_closed && ns == 0) {}
      else
	{
	  m_livewire.clear();
	  calculateCost(xpos, ypos, sz1);
	  calculateLivewire(sns.x(), sns.y());      
	  m_poly += m_livewire;
	}
      if (ns > 0)
	{
	  offset = m_poly.count()-1-m_seedpos[ns];
	  for(int i=ns; i<m_seedpos.count(); i++)
	    m_seedpos[i] += offset;
	  
	  m_poly += m_polyB;
	}
    }


  if (!m_closed)
    m_seedpos[m_seedpos.count()-1] = m_poly.count()-1;

  m_livewire.clear();
}

int
LiveWire::getActiveSeed(int xpos, int ypos)
{
  if (m_activeSeed > -1)
    return m_activeSeed;

  for(int i=0; i<m_seedpos.count(); i++)
    {
      if ((m_poly[m_seedpos[i]]-QPointF(xpos, ypos)).manhattanLength() < Global::selectionPrecision())
	{
	  splitPolygon(i);
	  return i;
	}
    }

  m_polyA.clear();
  m_polyB.clear();
  return -1;
}

int
LiveWire::insertSeed(int xpos, int ypos)
{
  if (m_type == 1)
    return -1;
  
  int ic;
  if (m_type == 0)
    ic = getActiveSeed(xpos, ypos);
  else
    ic = getActiveSeedFromShape(xpos, ypos);
  
  if (ic >= 0)
    return ic;
  
  int sp = -1;
  ic = -1;
  for(int is=1; is<m_seedpos.count(); is++)
    {
      for (int i=m_seedpos[is-1]; i<m_seedpos[is]; i++)
	{
	  int ml = (m_poly[i]-QPointF(xpos, ypos)).manhattanLength();
	  if (ml < Global::selectionPrecision())
	    {
	      for (int j=i; j<m_seedpos[is]; j++)
		{
		  int mhl = (m_poly[j]-QPointF(xpos, ypos)).manhattanLength();
		  if (mhl < ml)
		    {
		      ml = mhl;
		      sp = is;
		      ic = j;
		    }
		}
	      break;
	    }
	}
    }
  
  if (sp == -1 || ic == -1)
    {
      if (m_closed)
	{
	  int is = m_seedpos.count()-1;
	  for (int i=m_seedpos[is]; i<m_poly.count(); i++)
	    {
	      int ml = (m_poly[i]-QPointF(xpos, ypos)).manhattanLength();
	      if (ml < Global::selectionPrecision())
		{
		  for (int j=i; j<m_poly.count(); j++)
		    {
		      int mhl = (m_poly[j]-QPointF(xpos, ypos)).manhattanLength();
		      if (mhl < ml)
			{
			  ml = mhl;
			  sp = is+1;
			  ic = j;
			}
		    }
		  break;
		}
	    }
	}

      if (sp == -1 || ic == -1)
	return -1;
    }
  
  if (m_type > 0)
    m_seeds.insert(sp, QPointF(xpos, ypos));
  
  m_seedpos.insert(sp, ic);
  
  if (m_type > 0)
    updatePolygonFromSeeds();
  
  return ic;
}

int
LiveWire::removeSeed(int xpos, int ypos)
{
  m_activeSeed = -1;

  if (m_type == 1)
    return -1;
  

  if (m_seedpos.count() < 3 ||
      (m_type > 0 && m_seedpos.count() < 4))
    {
      QMessageBox::information(0, "",
			       "Less seed points : Cannot remove seed points for this curve");
      return -1;
    }
    

  int ic = -1;
  for(int i=0; i<m_seedpos.count(); i++)
    {
      if ((m_poly[m_seedpos[i]]-QPointF(xpos, ypos)).manhattanLength() < Global::selectionPrecision())
	{
	  ic = i;
	  break;
	}
    }

  if (!m_closed && (ic == 0 || ic == m_seedpos.count()-1))
    {
      QMessageBox::information(0, "",
			       "Cannot remove endpoints for open curves");
      return -1;
    }

  if (ic == 0)
    {
      QVector<QPointF> pts;
      for(int i=0; i<m_seedpos[1]; i++)
	m_poly << m_poly[i];
      m_poly.remove(0, m_seedpos[1]);

      int offset = m_seedpos[1];
      m_seedpos.removeAt(0);
      if (m_type > 0)
	m_seeds.removeAt(0);
      for(int i=0; i<m_seedpos.count(); i++)
	m_seedpos[i] -= offset;
    }
  else
    {
      m_seedpos.removeAt(ic);
      if (m_type > 0)
	m_seeds.removeAt(ic);
    }

  if (m_type > 0)
    updatePolygonFromSeeds();

  return ic;
}

void
LiveWire::splitPolygon(int sp)
{
  m_polyA.clear();
  m_polyB.clear();
  int totseeds = m_seedpos.count();
  int ps = (sp-1);
  int ns = (sp+1)%totseeds;;
  if (ps < 0) ps = totseeds-1;

  if (sp > 1)
    {
      for(int i=0; i<m_seedpos[ps]; i++)
	m_polyA << m_poly[i];
    }

  if (ns > 0)
    {
      int pend = m_poly.count();
      if (m_closed && sp == 0)
	pend = m_seedpos[m_seedpos.count()-1]+1;

      for(int i=m_seedpos[ns]+1; i<pend-1; i++)
	m_polyB << m_poly[i];
    }
}

int
LiveWire::getActiveSeedFromShape(int xpos, int ypos)
{
  if (m_activeSeed > -1)
    return m_activeSeed;

  for(int i=0; i<m_seedpos.count(); i++)
    {
      if ((m_poly[m_seedpos[i]]-QPointF(xpos, ypos)).manhattanLength() < Global::selectionPrecision())
	return i;
    }

  return -1;
}

void
LiveWire::moveShape(int dx, int dy)
{
  if (m_type == 0)
    return;

  for(int i=0; i<m_seeds.count(); i++)
    m_seeds[i] += QPointF(dx, dy);

  for(int i=0; i<m_poly.count(); i++)
    m_poly[i] += QPointF(dx, dy);
}

void
LiveWire::updateShapeFromSeeds(int xpos, int ypos)
{
  m_seeds[m_activeSeed] = QPointF(xpos, ypos);

  if (m_type == 1)
    updateEllipseFromSeeds();
  else
    updatePolygonFromSeeds();
}

void
LiveWire::updateEllipseFromSeeds()
{
  if (m_activeSeed == 0)
    {      
      QPointF cen = (m_poly[0] + m_poly[180])/2;
      QPointF b = m_poly[90] - cen;
      float blen = qSqrt(QPointF::dotProduct(b,b));
      QPointF a = m_seeds[0] - cen;
      float alen = qSqrt(QPointF::dotProduct(a,a));
      a /= alen;
      m_seeds[0] = cen + alen*a;
      m_seeds[2] = cen - alen*a;
      m_seeds[1] = cen + blen*QPointF(-a.y(), a.x());
    }
  else if (m_activeSeed == 1)
    {
      QPointF cen = (m_poly[0] + m_poly[180])/2;
      QPointF a = m_poly[0] - cen;
      float alen = qSqrt(QPointF::dotProduct(a,a));
      QPointF b = m_seeds[1] - cen;
      float blen = qSqrt(QPointF::dotProduct(b,b));
      b /= blen;
      m_seeds[0] = cen + alen*QPointF(b.y(), -b.x());
      m_seeds[2] = cen - alen*QPointF(b.y(), -b.x());
    }
  else if (m_activeSeed == 2)
    {
      QPointF delta = m_seeds[2] - m_poly[180];
      m_seeds[0] += delta;
      m_seeds[1] += delta;
    }

  QPointF cen = (m_seeds[0] + m_seeds[2])/2;
  QPointF a = m_seeds[0] - cen;
  QPointF b = m_seeds[1] - cen;

  m_poly.clear();
  m_seedpos.clear();
  for(int i=0; i<360; i++)
    {
      if (i%90 == 0 && m_seedpos.count() < 3)
	m_seedpos << m_poly.count();

      float ir = qDegreesToRadians((float)i);
      QPointF v = cen + a*cos(ir) + b*sin(ir);
      m_poly << v;
    }

  m_seeds.clear();
  m_seeds << m_poly[0];
  m_seeds << m_poly[90];
  m_seeds << m_poly[180];
}

void
LiveWire::updatePolygonFromSeeds()
{
  m_livewire.clear();
  m_poly.clear();
  m_seedpos.clear();
  
  QVector <QPointF> v, tv;
  v.resize(m_seeds.count());
  tv.resize(m_seeds.count());

  for(int i=0; i<m_seeds.count(); i++)
    v[i] = m_seeds[i];

  // polygon/polyline
  if (m_type == 3 || m_type == 5)
    {
      int ptend = m_seeds.count();
      if (m_type == 5) ptend--;

      for(int ptn=0; ptn<ptend; ptn++)
	{
	  m_seedpos << m_poly.count();
	  int nxt = (ptn+1)%m_seeds.count();
	  QPointF diff = v[nxt]-v[ptn];
	  int len = qSqrt(QPointF::dotProduct(diff, diff))/5;
	  len = qMax(2, len);
	  for(int i=0; i<len; i++)
	    {
	      float frc = i/(float)len;
	      QPointF p = v[ptn] + frc*diff;
	      m_poly << QPointF(p.x(), p.y());
	    }
	}

      if (m_type == 5) // polyline
	m_seedpos << m_poly.count()-1;
      return;
    }


  // smooth polygon/polyline
  if (m_type == 4) // smooth polyline
    {
      for(int i=0; i<m_seeds.count(); i++)
	{
	  int nxt = qMin(i+1, m_seeds.count()-1);
	  int prv = qMax(0, i-1);
	  tv[i] = (v[nxt]-v[prv])/2;
	}
    }
  else // smooth polygon
    {
      for(int i=0; i<m_seeds.count(); i++)
	{
	  int nxt = (i+1)%m_seeds.count();
	  int prv = i-1;
	  if (prv<0) prv = m_seeds.count()-1;
	  tv[i] = (v[nxt]-v[prv])/2;
	}
    }

  m_poly.clear();
  m_seedpos.clear();
  int ptend = m_seeds.count();
  if (m_type == 4) ptend--;
  for(int ptn=0; ptn<ptend; ptn++)
    {
      m_seedpos << m_poly.count();
      int nxt = (ptn+1)%m_seeds.count();
      QPointF diff = v[nxt]-v[ptn];
      int len = qSqrt(QPointF::dotProduct(diff, diff))/3;
      len = qMax(2, len);
      for(int i=0; i<len; i++)
	{
	  float frc = i/(float)len;
	  QPointF a1 = 3*diff - 2*tv[ptn] - tv[nxt];
	  QPointF a2 = -2*diff + tv[ptn] + tv[nxt];
	  QPointF p = v[ptn] + frc*(tv[ptn] + frc*(a1 + frc*a2));
	  m_poly << QPointF(p.x(), p.y());
	}
    }

  if (m_type == 4) // polyline
    m_seedpos << m_poly.count()-1;
}
