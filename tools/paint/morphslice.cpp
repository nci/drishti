#include "morphslice.h"
#include <QPainter>
#include <QLabel>
#include <QImage>

MorphSlice::MorphSlice()
{
  m_startSlice = 0;
  m_endSlice = 0;

  m_nX = 0;
  m_nY = 0;

  m_layout = 0;
}

MorphSlice::~MorphSlice()
{
  clearSlices();

  if (m_layout)
    delete m_layout;
  m_layout = 0;  
}

void
MorphSlice::clearSlices()
{
  if (m_startSlice) delete [] m_startSlice;
  if (m_endSlice) delete [] m_endSlice;

  m_startSlice = 0;
  m_endSlice = 0;

  m_nX = 0;
  m_nY = 0;
}

void
MorphSlice::showSliceImage(uchar *slice, int nX, int nY)
{
  if (!m_layout)
    {
      m_layout = new QVBoxLayout();
      QWidget *m_showSlices = new QWidget();
      m_showSlices->setLayout(m_layout);
//
//      QScrollArea *scroll = new QScrollArea();
//      scroll->setWidget(m_showSlices);
//      scroll->show();
//      scroll->resize(2*m_nY, 2*m_nX);

      m_showSlices->show();
    }

  QImage pimg = QImage(nX, nY, QImage::Format_RGB32);
  pimg.fill(0);
  uchar *bits = pimg.bits();
  for(int i=0; i<nX*nY; i++)
    {
      bits[4*i+0] = slice[i];
      bits[4*i+1] = slice[i];
      bits[4*i+2] = slice[i];
      bits[4*i+3] = 255;
    }
  
  //QImage qimg = pimg.scaled(pimg.width(), pimg.height());
  QLabel *lbl = new QLabel();
  lbl->setPixmap(QPixmap::fromImage(pimg));

  m_layout->addWidget(lbl);
  m_layout->update();
}

void
MorphSlice::showCurves(QList<QPolygonF> paths)
{
  QImage pimg = QImage(m_nX, m_nY, QImage::Format_RGB32);
  pimg.fill(0);

  QPainter p(&pimg);
  p.setPen(QPen(Qt::red, 1));
  //p.setBrush(Qt::transparent);
  p.setBrush(Qt::yellow);
  for(int i=0; i<paths.count(); i++)
    p.drawPolygon(paths[i]);
    //p.drawPoints(paths[i]);
  
  QImage qimg = pimg.scaled(pimg.width(), pimg.height());
  QLabel *lbl = new QLabel();
  lbl->setPixmap(QPixmap::fromImage(qimg));

  m_layout->addWidget(lbl);
}

QMap<int, QList<QPolygonF> >
MorphSlice::setPaths(QMap<int, QList<QPolygonF> > paths)
{
  clearSlices();

  QList<int> keys = paths.keys();
  // assuming only 2 keys
  int nSlices = keys[1]-keys[0]-1;
  int startkey = keys[0];
  int endkey = keys[1];

  //---------------
  {
    QList<QPolygonF> pf = paths.value(startkey);	  
    for(int ii=0; ii<pf.count(); ii++)
      for(int p=0; p<pf[ii].count(); p++)
	{
	  m_nX = qMax(m_nX, (int)qRound(pf[ii][p].x()));
	  m_nY = qMax(m_nY, (int)qRound(pf[ii][p].y()));
	}
  }
  {
    QList<QPolygonF> pf = paths.value(endkey);	  
    for(int ii=0; ii<pf.count(); ii++)
      {
	for(int p=0; p<pf[ii].count(); p++)
	  {
	    m_nX = qMax(m_nX, (int)qRound(pf[ii][p].x()));
	    m_nY = qMax(m_nY, (int)qRound(pf[ii][p].y()));
	  }
      }
  }

  // just add a bit of margin
  m_nX += 5;
  m_nY += 5;

  m_startSlice = new uchar[m_nX*m_nY];
  m_endSlice = new uchar[m_nX*m_nY];

  // plot start slice
  {
    QImage pimg = QImage(m_nX, m_nY, QImage::Format_RGB32);
    pimg.fill(0);

    QPainter p(&pimg);

    QList<QPolygonF> pf = paths.value(startkey);
	  
    for(int ii=0; ii<pf.count(); ii++)
      {
	p.setPen(QPen(QColor(255, 255, 255), 1));
        p.setBrush(QColor(255, 255, 255));
	p.drawPolygon(pf[ii]);
      }
    
    QRgb *rgb = (QRgb*)(pimg.bits());
    for(int i=0; i<m_nX*m_nY; i++)
      m_startSlice[i] = qRed(rgb[i]);
  }

  // plot end slice
  {
    QImage pimg = QImage(m_nX, m_nY, QImage::Format_RGB32);
    pimg.fill(0);

    QPainter p(&pimg);
    QList<QPolygonF> pf = paths.value(endkey);
    for(int ii=0; ii<pf.count(); ii++)
      {
	p.setPen(QPen(QColor(255, 255, 255), 1));
        p.setBrush(QColor(255, 255, 255));
	p.drawPolygon(pf[ii]);
      }

    QRgb *rgb = (QRgb*)(pimg.bits());
    for(int i=0; i<m_nX*m_nY; i++)
      m_endSlice[i] = qRed(rgb[i]);
  }
  
  QMap<int, QList<QPolygonF> > allcurves;
  allcurves = mergeSlices(nSlices);

  clearSlices();

  QMap<int, QList<QPolygonF> > finalCurves;
  {
    QList<int> keys = allcurves.keys();
    int nperi = keys.count();
    for(int n=0; n<nperi; n++)
      {
	int zv = keys[n];
	QList<QPolygonF> poly = allcurves[zv];	
	finalCurves.insert(zv, poly);
      }
  }

  return finalCurves;
}

QMap<int, QList<QPolygonF> >
MorphSlice::mergeSlices(int nSlices)
{
  uchar *slice = new uchar[m_nX*m_nY];
  float *fslice = new float[m_nX*m_nY];
  float *fstartSlice = new float[m_nX*m_nY];
  float *fendSlice = new float[m_nX*m_nY];

  // calculate signed distance transform using difference of two distance transforms
  distanceTransform(fstartSlice, m_startSlice, m_nY, m_nX, false);
  distanceTransform(fslice, m_startSlice, m_nY, m_nX, true);
  for(int s=0; s<m_nX*m_nY; s++)
    fstartSlice[s] -= fslice[s];
  
  // calculate signed distance transform using difference of two distance transforms
  distanceTransform(fendSlice, m_endSlice, m_nY, m_nX, false);
  distanceTransform(fslice, m_endSlice, m_nY, m_nX, true);
  for(int s=0; s<m_nX*m_nY; s++)
    fendSlice[s] -= fslice[s];

  
  QMap<int, QList<QPolygonF> > allcurves;
  for (int i=1; i<=nSlices; i++)
    {
      float frc = (float)i/(float)(nSlices+1);
      memset(slice, 0, m_nX*m_nY);
      for(int s=0; s<m_nX*m_nY; s++)
	{
	  float v = (fstartSlice[s]*(1.0-frc) + fendSlice[s]*frc);
	  if (v > 0)
	    slice[s] = 255;
	}
      QList<QPolygonF> curves = boundaryCurves(slice, m_nX, m_nY);
      allcurves.insert(i, curves);
    }

  delete [] slice;
  delete [] fslice;
  delete [] fstartSlice;
  delete [] fendSlice;
  
  return allcurves;
}

QList<QPolygonF>
MorphSlice::boundaryCurves(uchar *slice, int nX, int nY, bool shrinkwrap)
{
  uchar BLACK = 0;
  uchar WHITE = 255;

  int width = nX;
  int height = nY;

  uchar *paddedImage = new uchar[(width+2)*(height+2)];
  memset(paddedImage, BLACK, (width+2)*(height+2));
  for(int y=0; y<height; y++)
    for(int x=0; x<width; x++)
      if (slice[x+y*width] == 255)
	paddedImage[(x+1) + (y+1)*(width+2)] = WHITE;
  
  uchar *borderImage = new uchar[(width+2)*(height+2)];
  memset(borderImage, BLACK, (width+2)*(height+2));

  bool inside = false;
  int pos = 0;

  QList<QPolygonF> curves;
  for(int y = 2; y < height-1; y ++)
    for(int x = 2; x < width-1; x ++)
      {
	pos = x + y*(width+2);
	
	// Scan for WHITE pixel
	
	// Entering an already discovered border
	if(borderImage[pos] == WHITE && !inside)
	  {
	    inside = true;
	  }
	// Already discovered border point
	else if(paddedImage[pos] == WHITE && inside)
	  {
	    continue;
	  }
	// Leaving a border
	else if(paddedImage[pos] == BLACK && inside)
	  {
	    inside = false;
	  }
	// Undiscovered border point
	else if(paddedImage[pos] == WHITE && !inside)
	  {
	    borderImage[pos] = WHITE; // Mark the start pixel

	    // The neighbour number of the location
	    // we want to check for a new border point
	    int checkLocationNr = 1;
	    
	    // The corresponding absolute array address of checkLocationNr
	    int checkPosition;
	    
	    // Variable that holds the neighborhood position 
	    // we want to check if we find a new border at checkLocationNr
	    int newCheckLocationNr;
	    
	    int startPos = pos; // Set start position
	    
	    // Counter is used for the jacobi stop criterion
	    int counter = 0;
	    
	    // Counter2 is used to determine if the point
	    // we have discovered is one single point
	    int counter2 = 0;
	    
	    // Defines the neighborhood offset position
	    // from current position and the neighborhood position
	    // we want to check next if we find a new border at checkLocationNr
	    int neighborhood[8][2] = {
	      {-1,7},
	      {-3-width,7},
	      {-width-2,1},
	      {-1-width,1},
	      {1,3},
	      {3+width,3},
	      {width+2,5},
	      {1+width,5}
	    };
	    // Trace around the neighborhood
	    QPolygonF c;
	    while(true)
	      {
		checkPosition = pos + neighborhood[checkLocationNr-1][0];
		newCheckLocationNr = neighborhood[checkLocationNr-1][1];
		
		if(paddedImage[checkPosition] == WHITE) // Next border point found
		  {
		    if(checkPosition == startPos)
		      {
			counter ++;
			
			// Stopping criterion (jacob)
			if(newCheckLocationNr == 1 || counter >= 3)
			  {
			    // Close loop since we are starting the search
			    // at where we first started we must set inside to true
			    inside = true;
			    break;
			  }
		      } // checkPosition == startPos
		    
		    // Update which neighborhood position we should check next
		    checkLocationNr = newCheckLocationNr;
		    
		    pos = checkPosition;
		    
		    // Reset the counter that keeps track of how many neighbors we have visited
		    counter2 = 0;
		    
		    borderImage[checkPosition] = WHITE; // Set the border pixel
		    
		    int dx, dy;
		    dy = checkPosition/(width+2);
		    dx = checkPosition - dy*(width+2);
		    c << QPointF(dx,dy);
		  }
		else
		  {
		    // Rotate clockwise in the neighborhood
		    checkLocationNr = 1 + (checkLocationNr % 8);
		    if(counter2 > 8)
		      {
			// If counter2 is above 8 we have traced around the neighborhood and
			// therefore the border is a single black pixel and we can exit
			counter2 = 0;
			break;
		      }
		    else
		      {
			counter2 ++;
		      }
		  }
	      } // while
	    
	    if (c.count() > 1)
	      {
		// polygon returned is shifted by (1,1) so shift it back to original position
		c.translate(-1,-1);

		if (shrinkwrap)
		  {
		    // make sure that the interior is black
		    QImage pimg = QImage(width+2, height+2, QImage::Format_RGB32);
		    pimg.fill(0);
		    QPainter p(&pimg);
		    p.setPen(QPen(Qt::white, 1));
		    p.setBrush(Qt::white);
		    p.drawPolygon(c);
		    QRgb *rgb = (QRgb*)(pimg.bits());
		    for(int yp = 0; yp < height+2; yp ++)
		      for(int xp = 0; xp < width+2; xp ++)
			{
			  int rp = xp + yp*(width+2);
			  if (qRed(rgb[rp]) > 0)
			    {
			      int pos = xp + yp*(width+2);
			      paddedImage[pos] = 0;
			    }	
			}
		  }
		
		curves << c;
	      }
	  } // else
      } // for

//  showSliceImage(slice, width, height);
//  showSliceImage(paddedImage, width+2, height+2);
//  showSliceImage(borderImage, width+2, height+2);

  delete [] paddedImage;
  delete [] borderImage;

  return curves;
}


//------------------------------------------------------------
//------------------------------------------------------------
// distance transform using squared distance
//P. Felzenszwalb, D. Huttenlocher
//Theory of Computing, Vol. 8, No. 19, September 2012
//------------------------------------------------------------
#define INF 1E20
// 1D distance transform using squared distance
void
MorphSlice::distanceTransform(float *d, float *f, int n)
{
  int *v = new int[n];
  float *z = new float[n+1];

  int k = 0;
  v[k] = 0;
  z[k] = -INF;
  z[k+1]=+INF;

  for (int q=1; q<=n-1; q++)
    {
      float s;
      if (q != v[k])
	s = ((f[q]+(q*q))-(f[v[k]]+(v[k]*v[k])))/(2*q-2*v[k]);
      else
	s = 0;
      while (s <= z[k])
	{
	  k--;
	  if (q!=v[k])	    
	    s = ((f[q]+(q*q))-(f[v[k]]+(v[k]*v[k])))/(2*q-2*v[k]);
	  else
	    s = 0;
	}
      k++;
      v[k] = q;
      z[k] = s;
      z[k+1] = +INF;
    }

  k = 0;
  for (int q=0; q<=n-1; q++)
    {
      while (z[k+1] < q)
	{
	  k++;
	}
      d[q] = (q-v[k])*(q-v[k]) + f[v[k]];
    }

  delete [] v;
  delete [] z;
}

// 2D distance transform using squared distance
void
MorphSlice::distanceTransform(float *f0slice, uchar *slice, int width, int height, bool flip)
{
  float *fslice = new float[width*height];
  float *f = new float[qMax(width,height)];
  float *d = new float[qMax(width,height)];

  
  memset(fslice, 0, sizeof(float)*width*height);

  if (flip)
    {
      for (int i=0; i<width*height; i++)
	{
	  if (slice[i] == 0)
	    fslice[i] = INF;
	}
    }
  else
    {
      for (int i=0; i<width*height; i++)
	{
	  if (slice[i] > 0)
	    fslice[i] = INF;	  
	}
    }

  // transform along columns
  memset(f, 0, height*sizeof(float));
  for (int x=0; x<width; x++)
    {
      memcpy(f, fslice+x*height, height*sizeof(float));
      distanceTransform(d, f, height);
      memcpy(fslice+x*height, d, height*sizeof(float));
    }
  
  // transform along rows
  memset(f, 0, width*sizeof(float));
  for (int y=0; y<height; y++)
    {
      for (int x=0; x<width; x++)
	{
	  f[x] = fslice[x*height + y];
	}
      
      distanceTransform(d, f, width);

      for (int x=0; x<width; x++)
	{
	  fslice[x*height + y] = d[x];
	}
    }

  delete [] d;
  delete [] f;

  memcpy(f0slice, fslice, width*height*sizeof(float));
  
//  uchar *imgslice = new uchar[m_nX*m_nY];
//  for (int i=0; i<width*height; i++)
//    {
//      imgslice[i] = qMin(fslice[i],255.0f);
//    }
//
//  showSliceImage(imgslice, height, width);
//  QMessageBox::information(0, "", "dt end");
//  delete [] imgslice;

  delete [] fslice;
}
