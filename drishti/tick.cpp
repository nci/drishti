#include "global.h"
#include "tick.h"
#include "volumeinformation.h"
#include "matrix.h"
#include "enums.h"
#include "staticfunctions.h"

QString Tick::m_labelX = "Height";
void Tick::setLabelX(QString lbl) { m_labelX = lbl; }
QString Tick::labelX() { return m_labelX; }

QString Tick::m_labelY = "Width";
void Tick::setLabelY(QString lbl) { m_labelY = lbl; }
QString Tick::labelY() { return m_labelY; }

QString Tick::m_labelZ = "Depth";
void Tick::setLabelZ(QString lbl) { m_labelZ = lbl; }
QString Tick::labelZ() { return m_labelZ; }

int Tick::m_tickSize = 0;
void Tick::setTickSize(int ts) { m_tickSize = ts; }
int Tick::tickSize() { return m_tickSize; }

int Tick::m_tickStep = 10;
void Tick::setTickStep(int ts) { m_tickStep = ts; }
int Tick::tickStep() { return m_tickStep; }

void
Tick::draw(Camera *cam, double *Xform)
{
  if (Tick::tickSize() <= 0)
    return;

  Vec dataMin, dataMax, dataSize;
  Global::bounds(dataMin, dataMax);
  Vec voxelScaling = Global::voxelScaling();
  Vec bmin = VECPRODUCT(dataMin,voxelScaling);
  Vec bmax = VECPRODUCT(dataMax,voxelScaling);

  int i;

  Vec Volcrd[8];
  Volcrd[0] = Vec(bmin.x, bmin.y, bmin.z);
  Volcrd[1] = Vec(bmax.x, bmin.y, bmin.z);
  Volcrd[2] = Vec(bmax.x, bmax.y, bmin.z);
  Volcrd[3] = Vec(bmin.x, bmax.y, bmin.z);
  Volcrd[4] = Vec(bmin.x, bmin.y, bmax.z);
  Volcrd[5] = Vec(bmax.x, bmin.y, bmax.z);
  Volcrd[6] = Vec(bmax.x, bmax.y, bmax.z);
  Volcrd[7] = Vec(bmin.x, bmax.y, bmax.z);
  
  for (i=0; i<8; i++)
    Volcrd[i] = Matrix::xformVec(Xform, Volcrd[i]);

  int pairx[4][2] = { {0,1}, {3,2}, {4,5}, {7,6} };
  int pairy[4][2] = { {0,3}, {1,2}, {4,7}, {5,6} };
  int pairz[4][2] = { {0,4}, {1,5}, {2,6}, {3,7} };

  //-----------------------------------------------------
  // get positioning

  GLdouble objz, sxmin, sxmax, symin, symax;
  GLdouble scrx[10], scry[10];

  GLdouble OModel[16];
  glMatrixMode(GL_MODELVIEW);
  glGetDoublev(GL_MODELVIEW_MATRIX, OModel);
	 
  GLdouble OProj[16];
  glMatrixMode(GL_PROJECTION);
  glGetDoublev(GL_PROJECTION_MATRIX, OProj);
	 
  GLint Oviewport[4];
  glGetIntegerv(GL_VIEWPORT, Oviewport);
	 
  Vec centroid;
  centroid = (Volcrd[0] + Volcrd[6])/2;

  gluProject((GLdouble)centroid.x,
	     (GLdouble)centroid.y,
	     (GLdouble)centroid.z,
	     OModel, OProj,
	     Oviewport,
	     &scrx[8], &scry[8], &objz);

  for(i=0; i<8; i++)
    {
      gluProject((GLdouble)Volcrd[i].x,
		 (GLdouble)Volcrd[i].y,
		 (GLdouble)Volcrd[i].z,
		 OModel, OProj,
		 Oviewport,
		 &scrx[i], &scry[i], &objz);
    }
  //-----------------------------------------------------


  //-----------------------------------------------------
  // now go into ortho mode
//  glDepthMask(GL_TRUE);
  glDisable(GL_DEPTH_TEST);
//  glDisable(GL_BLEND);
//  glDisable(GL_LIGHTING);  


  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, cam->screenWidth(),
	  0, cam->screenHeight(),
	  -1, 1);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  

  //-----------------------------------------------------
  // get positioning and scaling
  GLdouble MModel[16];
  GLdouble MProj[16];
  GLint viewport[4];

  glMatrixMode(GL_MODELVIEW);
  glGetDoublev(GL_MODELVIEW_MATRIX, MModel);
	 
  glMatrixMode(GL_PROJECTION);
  glGetDoublev(GL_PROJECTION_MATRIX, MProj);
	 
  glGetIntegerv(GL_VIEWPORT, viewport);
	 
  GLdouble mousez = (float)0/(pow((double)2, (double)32)-1);

  gluUnProject((GLdouble)10,
	       (GLdouble)10,
	       (GLdouble)mousez,
	       MModel, MProj,
	       viewport,
	       &sxmin, &symin, &objz);

  gluUnProject((GLdouble)20,
	       (GLdouble)20,
	       (GLdouble)mousez,
	       MModel, MProj,
	       viewport,
	       &sxmax, &symax, &objz);

  float deltax = fabs(sxmax-sxmin);
  float deltay = fabs(symax-symin);
  float sclx, scly;
  sclx = deltax*0.005 * (1+Tick::tickSize()*0.2);
  scly = deltay*0.005 * (1+Tick::tickSize()*0.2);

  GLdouble newx[10], newy[10];
  for (i=0; i<9; i++)
    {
      newx[i] = scrx[i];
      newy[i] = scry[i];
    }

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glEnable(GL_LINE_SMOOTH);
  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

  int idx;
  float xc0, yc0, xc1, yc1;
  float bgintensity;
  Vec bgcolor = Global::backgroundColor();
  bgintensity = (0.3*bgcolor.x +
                 0.5*bgcolor.y +
		 0.2*bgcolor.z);


  if (bgintensity < 0.5)
    glColor4f(0.9, 0.9, 0.9, 0.9);
  else
    glColor4f(0, 0, 0, 0.9);


  //-----------------------------------------------------

  glLineWidth(1.5);

  idx = Tick::findMaxDistance(pairx, newx, newy);
  xc0 = newx[pairx[idx][0]];
  yc0 = newy[pairx[idx][0]];
  xc1 = newx[pairx[idx][1]];
  yc1 = newy[pairx[idx][1]];
  Tick::drawTick(Tick::tickStep(),
		 Volcrd[pairx[idx][0]], Volcrd[pairx[idx][1]],
		 newx[8], newy[8], 0,
		 sclx, scly,
		 OModel, OProj, Oviewport);

  idx = Tick::findMaxDistance(pairy, newx, newy);
  xc0 = newx[pairy[idx][0]];
  yc0 = newy[pairy[idx][0]];
  xc1 = newx[pairy[idx][1]];
  yc1 = newy[pairy[idx][1]];
  Tick::drawTick(Tick::tickStep(),
		 Volcrd[pairy[idx][0]], Volcrd[pairy[idx][1]],
		 newx[8], newy[8], 1,
		 sclx, scly,
		 OModel, OProj, Oviewport);

  idx = Tick::findMaxDistance(pairz, newx, newy);
  xc0 = newx[pairz[idx][0]];
  yc0 = newy[pairz[idx][0]];
  xc1 = newx[pairz[idx][1]];
  yc1 = newy[pairz[idx][1]];
  Tick::drawTick(Tick::tickStep(),
		 Volcrd[pairz[idx][0]], Volcrd[pairz[idx][1]],
		 newx[8], newy[8], 2,
		 sclx, scly,
		 OModel, OProj, Oviewport);

  //-----------------------------------------------------

  //-----------------------------------------------------
  // get back to original mode
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glPopMatrix();

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glPopMatrix();

  glEnable(GL_DEPTH_TEST);
}

int
Tick::findMaxDistance(int pairs[4][2],
		      GLdouble *newx,
		      GLdouble *newy)
{
  int plotindex = 0;
  float maxdist = 0;
  for (int i=0; i<4; i++)
    {
      float dist;
      float vx, vy;
      float dlen;
      int p1, p2;
      p1 = pairs[i][0];
      p2 = pairs[i][1];

      vx = (newx[p2]-newx[p1]);
      vy = (newy[p2]-newy[p1]);
      dlen = sqrt(vx*vx + vy*vy);
      
      dist = fabs(vx*(newy[p1]-newy[8]) -
		  vy*(newx[p1]-newx[8])) / dlen;
      
      if (maxdist < dist)
	{
	  maxdist = dist;
	  plotindex = i;
	}
    }
  return plotindex;
}

bool
Tick::drawText(QString str,
	       int xc0, int xc1, int yc0, int yc1,
	       float perpx, float perpy,
	       float angle, bool angleFixed, float pshift,
	       int fsize)
{
  int len = str.length();
  if (len <= 0)
    return false;

  float fscl = 120.0/Global::dpi();
  QFont font = QFont();  
  font.setPointSize(fsize*fscl);
  QFontMetrics metric(font);
  int fht = metric.height();

  int x = (xc0+xc1)/2;
  int y = (yc0+yc1)/2;
  x += perpx*pshift;
  y += perpy*pshift;
  if (angleFixed)
    {
      x += 0.9*perpx;
      y += 0.9*perpy;
    }

  StaticFunctions::renderRotatedText(x, y,
				     str, font,
				     Qt::transparent, Qt::white,
				     angle,
				     false); // (0,0) is top left
  return true;
}

void
Tick::drawTick(int tickstep,
	       Vec wv0, Vec wv1,
	       float ox, float oy, int axis, float sclx, float scly,
	       GLdouble* OModel, GLdouble *OProj, GLint *Oviewport)
{

  int i, nvox, nsteps;
  float dlen, perpx, perpy;
  Vec step;  
  float xc0, yc0, xc1, yc1;
  float *ticx, *ticy;

  Vec dataMin, dataMax, dataSize;
  Global::bounds(dataMin, dataMax);
  dataSize = dataMax - dataMin + Vec(1,1,1);
 
  nvox = dataSize[axis];
  if (tickstep == 0)
    { // just get the end points when
      // we don't want to plot any ticks
      GLdouble tx,ty,tz;
      gluProject((GLdouble)wv0.x,
		 (GLdouble)wv0.y,
		 (GLdouble)wv0.z,
		 OModel, OProj,
		 Oviewport,
		 &tx, &ty, &tz);      
      xc0 = tx;
      yc0 = ty;

      gluProject((GLdouble)wv1.x,
		 (GLdouble)wv1.y,
		 (GLdouble)wv1.z,
		 OModel, OProj,
		 Oviewport,
		 &tx, &ty, &tz);      
      xc1 = tx;
      yc1 = ty;

      perpx = (yc1-yc0);
      perpy = -(xc1-xc0);
      dlen = sqrt(perpx*perpx + perpy*perpy);
      perpx/=dlen; perpy/=dlen;
      perpx*=200*sclx; perpy*=200*sclx;
      
      if (perpx*(xc0-ox) +
	  perpy*(yc0-oy) < 0)
	{
	  perpx = -perpx;
	  perpy = -perpy;
	}      
    }
  else
    { // get tick marker positions
      nsteps = nvox/tickstep;
      if (nvox%tickstep == 0)
	nsteps += 1;
      else
	nsteps += 2;
      if (nsteps < 2) nsteps = 2;

      ticx = new float[nsteps];
      ticy = new float[nsteps];
      
      step = tickstep*(wv1-wv0)/nvox;
      for (i=0; i<nsteps; i++)
	{
	  Vec w;
	  GLdouble tx,ty,tz;
	  w = wv0 + i*step;
	  if (i == nsteps-1)
	    w = wv1;
	  gluProject((GLdouble)w.x,
		     (GLdouble)w.y,
		     (GLdouble)w.z,
		     OModel, OProj,
		     Oviewport,
		     &tx, &ty, &tz);
	  
	  ticx[i] = tx;
	  ticy[i] = ty;
	}
      
      xc0 = ticx[0]; yc0 = ticy[0];
      xc1 = ticx[nsteps-1]; yc1 = ticy[nsteps-1];
      
      perpx = (yc1-yc0);
      perpy = -(xc1-xc0);
      dlen = sqrt(perpx*perpx + perpy*perpy);
      perpx/=dlen; perpy/=dlen;
      perpx*=200*sclx; perpy*=200*sclx;
      
      if (perpx*(xc0-ox) +
	  perpy*(yc0-oy) < 0)
	{
	  perpx = -perpx;
	  perpy = -perpy;
	}
      
      glBegin(GL_LINES);
      for (i=0; i<nsteps; i++)
	{
	  float x, y;
	  
	  x = ticx[i];
	  y = ticy[i];
	  
	  glVertex2f(x, y);
	  
	  if (i == 0 || i == nsteps-1)
	    glVertex2f(x+1.5*perpx, y+1.5*perpy);
	  else
	    glVertex2f(x+perpx, y+perpy);
	}
      if (tickstep < (dataMax[axis]-dataMin[axis]))
	{
	  glVertex2f(xc0+perpx*2.8, yc0+perpy*2.8);
	  glVertex2f(xc1+perpx*2.8, yc1+perpy*2.8);
	}
      glEnd();
      
      delete [] ticx;
      delete [] ticy;
    } //--- if (tickstep > 0) --------


  float x, y, angle;
  bool angleFixed = false;
  // find rotation angle for text
  x = perpx; y = perpy;
  dlen = sqrt(x*x+y*y);
  x/=dlen; y/=dlen;
  angle = atan2(-x, y);
  angle *= 180.0/3.1415926535;

  if (perpy < 0) { angle = 180+angle; angleFixed = true; }

  QString str;
  int len;
  float width;
  float pshift = 0.2;
  if (tickstep > 0) pshift = 1.5;

  if (axis == 0) str = Tick::labelX();
  else if (axis == 1) str = Tick::labelY();
  else if (axis == 2) str = Tick::labelZ();

  if (Tick::drawText(str,
		     xc0, xc1, yc0, yc1,
		     perpx, perpy,
		     angle, angleFixed, pshift, 12))
    pshift += 1.9;


  int flip = 1;
  if (perpx < 0)
    flip = -1;



  if (tickstep > 0)
    {
      str = QString("%1").arg(dataMin[axis]);
      x = xc0;
      y = yc0;
      x += perpx*2;
      y += perpy*2;
      float fangle = angle;
      if (angleFixed)
	fangle = angle-flip*90;
      else
	fangle = angle+flip*90;
      float fscl = 120.0/Global::dpi();
      QFont font = QFont();  
      font.setPointSize(10*fscl);
      StaticFunctions::renderRotatedText(x, y,
					 str, font,
					 Qt::transparent, Qt::white,
					 fangle,
					 false); // (0,0) is top left

      
      str = QString("%1").arg(dataMax[axis]);
      x = xc1;
      y = yc1;
      x += perpx*2;
      y += perpy*2;
      fangle = angle;
      if (angleFixed)
	fangle = angle-flip*90;
      else
	fangle = angle+flip*90;
      StaticFunctions::renderRotatedText(x, y,
					 str, font,
					 Qt::transparent, Qt::white,
					 fangle,
					 false); // (0,0) is top left
    }


  VolumeInformation pvlInfo = VolumeInformation::volumeInformation();

  if (pvlInfo.voxelUnit > 0)
    {
      str = QString("<- %1 %2 ->").\
	            arg(nvox*pvlInfo.voxelSize[axis], 0, 'f', Global::floatPrecision()). \
	            arg(pvlInfo.voxelUnitStringShort());

      if (Tick::drawText(str,
			 xc0, xc1, yc0, yc1,
			 perpx, perpy,
			 angle, angleFixed, pshift, 11))
	pshift += 1.3;
    }


  if (tickstep > 0 &&
      tickstep < (dataMax[axis]-dataMin[axis]))
    {
      if (pvlInfo.voxelUnit > 0)
	str = QString("ticks at %1 voxels (%2 %3)"). \
	  arg(tickstep).			     \
	  arg(tickstep*pvlInfo.voxelSize[axis], 0, 'f', Global::floatPrecision()).     \
	  arg(pvlInfo.voxelUnitStringShort());
      else
	str = QString("ticks at %1 voxels").arg(tickstep);

      Tick::drawText(str,
		     xc0, xc1, yc0, yc1,
		     perpx, perpy,
		     angle, angleFixed, pshift, 10);
    }
}

