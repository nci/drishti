#include "morphslice.h"
#include <QPainter>
#include <QLabel>
#include <QImage>

MorphSlice::MorphSlice()
{
  m_startSlice = 0;
  m_endSlice = 0;
  m_overlapSlice = 0;
  m_o2S.clear();
  m_o2E.clear();
}

MorphSlice::~MorphSlice()
{
  clearSlices();
}

void
MorphSlice::clearSlices()
{
  delete [] m_startSlice;
  delete [] m_endSlice;
  delete [] m_overlapSlice;
  for(int i=0; i<m_o2E.count(); i++)
    delete [] m_o2E[i];
  for(int i=0; i<m_o2S.count(); i++)
    delete [] m_o2S[i];

  m_startSlice = 0;
  m_endSlice = 0;
  m_overlapSlice = 0;
  m_o2E.clear();
  m_o2S.clear();

  m_nX = 0;
  m_nY = 0;
}

void
MorphSlice::showSliceImage(QVBoxLayout *layout, int *slice)
{
  QImage pimg = QImage(m_nX, m_nY, QImage::Format_RGB32);
  pimg.fill(0);
  uchar *bits = pimg.bits();
  for(int i=0; i<m_nX*m_nY; i++)
    {
      bits[4*i+0] = slice[i];
      bits[4*i+1] = slice[i];
      bits[4*i+2] = slice[i];
      bits[4*i+3] = 255;
    }
  
  QImage qimg = pimg.scaled(pimg.width(), pimg.height());
  QLabel *lbl = new QLabel();
  lbl->setPixmap(QPixmap::fromImage(qimg));

  layout->addWidget(lbl);
}

void
MorphSlice::showCurves(QVBoxLayout *layout, QList<QPolygonF> paths)
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

  layout->addWidget(lbl);
}

QMap<int, QList<QPolygonF> >
MorphSlice::setPaths(QMap<int, QList<QPolygonF> > paths)
{
  clearSlices();

  QList<int> keys = paths.keys();

  m_nX = 0;
  m_nY = 0;

  int nSlices = keys[1]-keys[0]-1;
  //int nSlices = 11;

  // assuming only 2 paths
  for(int k=0; k<keys.count(); k++)
  {
    QList<QPolygonF> pf = paths.value(keys[k]);
    for(int ii=0; ii<pf.count(); ii++)
      {
	for(int i=0; i<pf[ii].count(); i++)
	  {
	    m_nX = qMax(m_nX, (int)qRound(pf[ii][i].x()));
	    m_nY = qMax(m_nY, (int)qRound(pf[ii][i].y()));
	  }
      }
//    QVector<QPointF> c = paths.value(keys[k]);
//    for(int i=0; i<c.count(); i++)
//      {
//	m_nX = qMax(m_nX, (int)qRound(c[i].x()));
//	m_nY = qMax(m_nY, (int)qRound(c[i].y()));
//      }
  }
  m_nX += 50;
  m_nY += 50;

  m_startSlice = new int[m_nX*m_nY];
  m_endSlice = new int[m_nX*m_nY];
  m_overlapSlice = new int[m_nX*m_nY];

  int nextkey = 0;
  {
    QImage pimg = QImage(m_nX, m_nY, QImage::Format_RGB32);
    pimg.fill(0);

    QPainter p(&pimg);
//    QVector<QPointF> c = paths.value(keys[0]);
//    p.setPen(QPen(QColor(255, 0, 0), 1));
//    p.setBrush(QColor(255, 255, 255));
//    p.drawPolygon(c);

    for(int k=0; k<keys.count(); k++)
      {
	//QVector<QPointF> c = paths.value(keys[k]);
	if (keys[k] == keys[nextkey])
	  {
	    QList<QPolygonF> pf = paths.value(keys[k]);
	    for(int ii=0; ii<pf.count(); ii++)
	      {
		p.setPen(QPen(QColor(255, 0, 0), 1));
		p.setBrush(QColor(255, 255, 255));
		p.drawPolygon(pf[ii]);
	      }
	  }
	else
	  {
	    nextkey = k;
	    break;
	  }
      }
    
    QRgb *rgb = (QRgb*)(pimg.bits());
    for(int i=0; i<m_nX*m_nY; i++)
      m_startSlice[i] = 255*qBound(0, qRed(rgb[i]), 1);
  }

  {
    QImage pimg = QImage(m_nX, m_nY, QImage::Format_RGB32);
    pimg.fill(0);

    QPainter p(&pimg);
    for(int k=nextkey; k<keys.count(); k++)
      {
	QList<QPolygonF> pf = paths.value(keys[k]);
	for(int ii=0; ii<pf.count(); ii++)
	  {
	    p.setPen(QPen(QColor(255, 0, 0), 1));
	    p.setBrush(QColor(255, 255, 255));
	    p.drawPolygon(pf[ii]);
	  }
      }

    QRgb *rgb = (QRgb*)(pimg.bits());
    for(int i=0; i<m_nX*m_nY; i++)
      m_endSlice[i] = 255*qBound(0, qBlue(rgb[i]), 1);
  }

  memset(m_overlapSlice, 0, m_nX*m_nY*sizeof(int));
  for(int i=0; i<m_nX*m_nY; i++)
    if (m_endSlice[i] == 255 && m_startSlice[i] == 255)
      m_overlapSlice[i] = 255;


//  QWidget *m_showSlices = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout();
//  m_showSlices->setLayout(layout);
//
//  showSliceImage(layout, m_startSlice);
//  showSliceImage(layout, m_endSlice);
//  showSliceImage(layout, m_overlapSlice);

  m_o2E = dilateOverlap(m_endSlice);
  m_o2S = dilateOverlap(m_startSlice);

  QMap<int, QList<QPolygonF> > allcurves;
  allcurves = mergeSlices(layout, nSlices);
  
//  QScrollArea *scroll = new QScrollArea();
//  scroll->setWidget(m_showSlices);
//  scroll->show();
//  scroll->resize(2*m_nY, 2*m_nX);

  for(int i=0; i<m_o2E.count(); i++)
    delete[] m_o2E[i];
  for(int i=0; i<m_o2S.count(); i++)
    delete[] m_o2S[i];
  m_o2E.clear();
  m_o2S.clear();

  return allcurves;
}

QMap<int, QList<QPolygonF> >
MorphSlice::mergeSlices(QVBoxLayout *layout, int nSlices)
{
  QMap<int, QList<QPolygonF>> allcurves;

  int tns = m_o2S.count()-1;
  int tne = m_o2E.count()-1;
  //int n = 10;
  for(int n=1; n<=nSlices; n++)
    {
      int *slice = new int[m_nX*m_nY];
      memset(slice, 0, m_nX*m_nY*sizeof(int));

      float frc = (float)n/(float)(nSlices+1);

      if (tns > 0)
	{
	  int sn = qRound((1.0-frc)*tns);
	  memcpy(slice, m_o2S[sn], m_nX*m_nY*sizeof(int));
	}

      if (tne > 0)
	{
	  int sn = qRound(frc*tne);
	  for(int i=0; i<m_nX*m_nY; i++)
	    slice[i] = qMax(slice[i], m_o2E[sn][i]);
	}

      //boundary(slice);
      //showSliceImage(layout, slice);

      QList<QPolygonF> curves = boundaryCurves(slice);
      //showCurves(layout, curves);

      allcurves.insert(n, curves);

      //showSliceImage(layout, slice);
    }


//  QMessageBox::information(0, "", QString("%1 %2 : %3").\
//			   arg(tns).arg(tne).arg(nSlices));
  return allcurves;
}

QList<int*>
MorphSlice::dilateOverlap(int *slice)
{
  QList<int*> slist;

  int cdo[] = { -1, 0, 0, 1, 1, 0, 0, -1 };

  int *t0 = new int[m_nX*m_nY];

  memcpy(t0, m_overlapSlice, m_nX*m_nY*sizeof(int));

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
	  
	  int *t1 = new int[m_nX*m_nY];
	  memcpy(t1, t0, m_nX*m_nY*sizeof(int));
	  slist << t1;
	}
      else
	done = true;
    }

  delete[] t0;
  //QMessageBox::information(0, "", QString("%1 : %2").arg(rad).arg(bpixels.count()));

  return slist;
}

//QList<int*>
//MorphSlice::dilateOverlap(int *slice)
//{
//  QList<int*> slist;
//
//  int cdo[] = { -1, 0, 0, 1, 1, 0, 0, -1 };
//
//  int *t0 = new int[m_nX*m_nY];
//
//  memcpy(t0, m_overlapSlice, m_nX*m_nY*sizeof(int));
//
//  bool done = false;
//  int nslices = 0;
//
//  while (!done)
//    {
//      done = true;
//
//      // identify boundary pixels
//      QList<QPoint> bpixels;
//      for(int y=0; y<m_nY; y++)
//	for(int x=0; x<m_nX; x++)
//	  {
//	    int idx = y*m_nX+x;
//	    if (t0[idx] == 255 && slice[idx] == 255)
//	      {
//		bool bdry = false;
//		for(int k=0; k<4; k++)
//		  {
//		    int dx = qBound(0, x + cdo[2*k], m_nX-1);
//		    int dy = qBound(0, y + cdo[2*k+1], m_nY-1);
//		    int didx = dy*m_nX+dx;
//		    if (slice[didx] == 255 && t0[didx] == 0)
//		      {
//			bdry = true;
//			bpixels << QPoint(dx, dy);
//		      }
//		  }
//	      }
//	  }
//      
//      if (bpixels.count() > 0)
//	{
//	  for(int i=0; i<bpixels.count(); i++)
//	    {
//	      int x = bpixels[i].x();
//	      int y = bpixels[i].y();
//	      t0[y*m_nX+x] = 255;	  
//	    }
//
//	  
//	  int *t1 = new int[m_nX*m_nY];
//	  memcpy(t1, t0, m_nX*m_nY*sizeof(int));
//	  slist << t1;
//
//	  done = false;
//	  nslices ++;
//	}
//    }
//
//  delete[] t0;
//
//  return slist;
//  //QMessageBox::information(0, "", QString("%1").arg(nslices));
//}

void
MorphSlice::boundary(int *slice)
{
  //int cdo[] = { 0,1, -1,1, -1,0, -1,-1, 0,-1, 1,-1, 1,0, 1,1 };
  int cdo[] = { -1,0, 0,1, 1,0, 0,-1 };

  for(int y=0; y<m_nY; y++)
    for(int x=0; x<m_nX; x++)
      {
	int idx = y*m_nX+x;
	if (slice[idx] == 255)
	  {
	    for(int k=0; k<4; k++)
	    //for(int k=0; k<8; k++)
	      {
		int dx = qBound(0, x + cdo[2*k], m_nX-1);
		int dy = qBound(0, y + cdo[2*k+1], m_nY-1);
		int didx = dy*m_nX+dx;
		if (slice[didx] == 0)
		  {
		    slice[idx] = 200;
		    break;
		  }
	      }
	  }
      }

  for(int y=0; y<m_nY; y++)
    for(int x=0; x<m_nX; x++)
      if (slice[y*m_nX+x] == 255)
	slice[y*m_nX+x] = 30;
}

QList<QPolygonF>
MorphSlice::boundaryCurves(int *slice)
{
  uchar BLACK = 0;
  uchar WHITE = 255;

  int width = m_nX;
  int height = m_nY;

  uchar *paddedImage = new uchar[(width+2)*(height+2)];
  memset(paddedImage, WHITE, (height+2)*(height+2));
  for(int y=0; y<height; y++)
    for(int x=0; x<width; x++)
      if (slice[x+y*width] == 255)
	paddedImage[(x+1) + (y+1)*(width+2)] = BLACK;
  
  uchar *borderImage = new uchar[(width+2)*(height+2)];
  memset(borderImage, WHITE, (width+2)*(height+2));

  bool inside = false;
  int pos = 0;

  QList<QPolygonF> curves;
  for(int y = 1; y < height-20; y ++)
    for(int x = 1; x < width-20; x ++)
      {
	pos = x + y*(width+2);
	
	// Scan for BLACK pixel
	
	// Entering an already discovered border
	if(borderImage[pos] == BLACK && !inside)
	  {
	    inside = true;
	  }
	// Already discovered border point
	else if(paddedImage[pos] == BLACK && inside)
	  {
	    continue;
	  }
	// Leaving a border
	else if(paddedImage[pos] == WHITE && inside)
	  {
	    inside = false;
	  }
	// Undiscovered border point
	else if(paddedImage[pos] == BLACK && !inside)
	  {
	    borderImage[pos] = BLACK; // Mark the start pixel
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
		
		if(paddedImage[checkPosition] == BLACK) // Next border point found
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
		    
		    borderImage[checkPosition] = BLACK; // Set the border pixel
		    
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
	      curves << c;
	  } // else
      } // for

  delete [] paddedImage;
  delete [] borderImage;

  return curves;
}
