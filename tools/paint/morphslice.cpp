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
  
  QImage qimg = pimg.scaled(pimg.width(), pimg.height());
  QLabel *lbl = new QLabel();
  lbl->setPixmap(QPixmap::fromImage(qimg));

  m_layout->addWidget(lbl);
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

  m_xlate = QPointF(0,0);

  //---------------
  QPointF cen0, cen1;
  {
    cen0 = QPointF(0,0);
    QList<QPolygonF> pf = paths.value(startkey);	  
    int npts = 0;
    for(int ii=0; ii<pf.count(); ii++)
      {
	npts += pf[ii].count();
	for(int p=0; p<pf[ii].count(); p++)
	  cen0 += pf[ii][p];
      }
    cen0 /= npts;
  }
  {
    cen1 = QPointF(0,0);
    QList<QPolygonF> pf = paths.value(endkey);	  
    int npts = 0;
    for(int ii=0; ii<pf.count(); ii++)
      {
	npts += pf[ii].count();
	for(int p=0; p<pf[ii].count(); p++)
	  cen1 += pf[ii][p];
      }
    cen1 /= npts;
  }

  m_xlate = cen0-cen1;  
  //---------------

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
	pf[ii].translate(m_xlate);
	for(int p=0; p<pf[ii].count(); p++)
	  {
	    m_nX = qMax(m_nX, (int)qRound(pf[ii][p].x()));
	    m_nY = qMax(m_nY, (int)qRound(pf[ii][p].y()));
	  }
      }
  }
//  for(int k=0; k<keys.count(); k++)
//  {
//    QList<QPolygonF> pf = paths.value(keys[k]);
//    for(int ii=0; ii<pf.count(); ii++)
//      {
//	for(int i=0; i<pf[ii].count(); i++)
//	  {
//	    m_nX = qMax(m_nX, (int)qRound(pf[ii][i].x()));
//	    m_nY = qMax(m_nY, (int)qRound(pf[ii][i].y()));
//	  }
//      }
//  }

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
	p.setPen(QPen(QColor(255, 0, 0), 1));
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
	pf[ii].translate(m_xlate);
	p.setPen(QPen(QColor(255, 0, 0), 1));
	p.setBrush(QColor(255, 255, 255));
	p.drawPolygon(pf[ii]);
      }

    QRgb *rgb = (QRgb*)(pimg.bits());
    for(int i=0; i<m_nX*m_nY; i++)
      m_endSlice[i] = qBlue(rgb[i]);
  }

  QMap<int, QList<QPolygonF> > allcurves;
  allcurves = mergeSlices(nSlices);

  clearSlices();

  //return allcurves;

  QMap<int, QList<QPolygonF> > finalCurves;
  {
    QList<int> keys = allcurves.keys();
    int nperi = keys.count();
    for(int n=0; n<nperi; n++)
      {
	int zv = keys[n];
	QList<QPolygonF> poly = allcurves[zv];
	QPointF move = m_xlate*(float)zv/(float)keys.count();
	for(int p=0; p<poly.count(); p++)
	  poly[p].translate(-move);
	
	finalCurves.insert(zv, poly);
      }
  }

  return finalCurves;
}

QMap<int, QList<QPolygonF> >
MorphSlice::mergeSlices(int nSlices)
{
  uchar *startSlice = new uchar[m_nX*m_nY];
  uchar *endSlice = new uchar[m_nX*m_nY];
  
  memcpy(startSlice, m_startSlice, m_nX*m_nY);
  memcpy(endSlice, m_endSlice, m_nX*m_nY);

  QList<uchar *> slices;
  slices << startSlice;
  slices << endSlice;
  bool done = false;
  while (!done)
    {
      QList<uchar *> newslices;
      for(int i=0; i<slices.count()-1; i++)
	{
	  uchar *slice0, *slice1;
	  slice0 = slices[i];
	  slice1 = slices[i+1];

	  // find overlap between start and end
	  uchar *overlapSlice = new uchar[m_nX*m_nY];
	  memset(overlapSlice, 0, m_nX*m_nY);
	  for(int i=0; i<m_nX*m_nY; i++)
	    if (slice1[i] == 255 && slice0[i] == 255)
	      overlapSlice[i] = 255;

	  // dilate overlap to start slice
	  QList<uchar*> o2S = dilateOverlap(slice0, overlapSlice);

	  // dilate overlap to end slice
	  QList<uchar*> o2E = dilateOverlap(slice1, overlapSlice);

	  // add start slice to slices list
	  newslices << slice0;

	  // get median slice to slices list
	  if (o2S.count() > 0 || o2E.count() > 0) 
	    newslices << getMedianSlice(o2S, o2E);
	  else
	    {
	      uchar *dummy0 = new uchar[m_nX*m_nY];
	      memcpy(dummy0, slice0, m_nX*m_nY);	  
	      newslices << slice0; // repeat the slice
	    }


	  // free the dilation lists
	  for(int i=0; i<o2S.count(); i++) delete [] o2S[i];
	  for(int i=0; i<o2E.count(); i++) delete [] o2E[i];
	  o2S.clear();
	  o2E.clear();
	}

      newslices << slices[slices.count()-1]; // last slice
      slices = newslices;

      if (slices.count() >= nSlices)
	done = true;      
    }
  
  int totslices = slices.count();

//   m_layout = new QVBoxLayout();

//  QWidget *m_showSlices = new QWidget();
//  m_showSlices->setLayout(m_layout);

  // find boundary curves
  QMap<int, QList<QPolygonF> > allcurves;
  for(int n=1; n<=nSlices; n++)
    {
      int si = (totslices-1)*(float)n/(float)nSlices;
      QList<QPolygonF> curves = boundaryCurves(slices[si], m_nX, m_nY);
      allcurves.insert(n, curves);      
    }

//  QScrollArea *scroll = new QScrollArea();
//  scroll->setWidget(m_showSlices);
//  scroll->show();
//  scroll->resize(2*m_nY, 2*m_nX);


  // free slices list
  for(int i=0; i<slices.count(); i++) delete [] slices[i];
  slices.clear();

  return allcurves;
}

QList<uchar*>
MorphSlice::dilateOverlap(uchar *slice, uchar *overlapSlice)
{
  QList<uchar*> slist;

  int cdo[] = { -1, 0, 0, 1, 1, 0, 0, -1 };

  uchar *t0 = new uchar[m_nX*m_nY];

  memcpy(t0, overlapSlice, m_nX*m_nY);

  uchar *t1 = new uchar[m_nX*m_nY];
  memcpy(t1, t0, m_nX*m_nY);
  slist << t1;

  // identify boundary pixels
  QList<QPoint> bpixels;
  for(int y=0; y<m_nY; y++)
    for(int x=0; x<m_nX; x++)
      {
	int idx = y*m_nX+x;
	if (slice[idx] == 255 && t0[idx] == 255)
	  {
	    bool bdry = false;
	    for(int k=0; k<4; k++)
	      {
		int dx = qBound(0, x + cdo[2*k], m_nX-1);
		int dy = qBound(0, y + cdo[2*k+1], m_nY-1);
		int didx = dy*m_nX+dx;
		if (slice[didx] == 255 && t0[didx] == 0)
		  {
		    bdry = true;
		    break;
		  }
	      }
	    if (bdry) bpixels << QPoint(x, y);
	  }
      }
  

  bool done = false;
  int rad = 1;
  while (!done)
    {      
      bool voxChanged = false;
      for(int i=0; i<bpixels.count(); i++)
	{
	  int x0 = bpixels[i].x();
	  int y0 = bpixels[i].y();
	  for(int y=qMax(0, y0-rad); y<qMin(m_nY, y0+rad+1); y++)
	    for(int x=qMax(0, x0-rad); x<qMin(m_nX, x0+rad+1); x++)
	      {
		float p = ((x-x0)*(x-x0)+
			   (y-y0)*(y-y0));
		if (p <= rad*rad)
		  {
		    int didx = y*m_nX+x;
		    if (slice[didx] == 255 && t0[didx] == 0)
		      {
			t0[didx] = 255;
			voxChanged = true;
		      }
		  }
	      }
	}
      if (voxChanged)
	{      
	  rad ++;
	  
	  uchar *t1 = new uchar[m_nX*m_nY];
	  memcpy(t1, t0, m_nX*m_nY);
	  slist << t1;
	}
      else
	done = true;
    }

  delete[] t0;

  return slist;
}

uchar*
MorphSlice::getMedianSlice(QList<uchar*> o2S, QList<uchar*> o2E)
{
  int tns = o2S.count()-1;
  int tne = o2E.count()-1;

  int maxslc = qMax(tns, tne);

  QList<int> imed;
  QList<uchar*> seq;
  for(int s=0; s<=maxslc; s++)
    {
      uchar *slice = new uchar[m_nX*m_nY];
      memset(slice, 0, m_nX*m_nY);

      if (s <= qMin(tns, tne))
	{
	  memcpy(slice, o2S[tns-s], m_nX*m_nY);
	  
	  for(int i=0; i<m_nX*m_nY; i++)
	    slice[i] = qMax(slice[i], o2E[s][i]);
	}
      else if (s > tns)
	memcpy(slice, o2E[s], m_nX*m_nY);
      else if (s > tne)
	memcpy(slice, o2S[tns-s], m_nX*m_nY);

      seq << slice;

      int diffS = 0;
      int diffE = 0;
      if (tns >= 0)
	{
	  for(int p=0; p<m_nX*m_nY; p++)
	    if (slice[p] != o2S[0][p]) diffS++;
	}

      if (tne >= 0)
	{
	  for(int p=0; p<m_nX*m_nY; p++)
	    if (slice[p] != o2E[tne][p]) diffE++;
	}
      imed << qAbs(diffS - diffE);
    }

  // take smallest difference element - this would be median element
  QString str;
  int slc = 0;
  int slcmin = imed[0];
  for(int s=1; s<imed.count(); s++)
    {
      if (slcmin > imed[s])
	{
	  slcmin = imed[s];
	  slc = s;
	}
    }
  
  uchar *slice = new uchar[m_nX*m_nY];
  memcpy(slice, seq[slc], m_nX*m_nY);

  for(int i=0; i<seq.count(); i++)
    delete [] seq[i];
  seq.clear();

  return slice;
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
  for(int y = 1; y < height; y ++)
    for(int x = 1; x < width; x ++)
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
