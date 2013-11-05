#include "bricks.h"
#include "staticfunctions.h"
#include "matrix.h"
#include "global.h"

QList<BrickInformation> Bricks::bricks() { return m_bricks; }
QList<BrickBounds> Bricks::ghostBricks() { return m_ghostBricks; }

void Bricks::setShowAxis(bool flag) { m_showAxis = flag; }

int Bricks::selected() { return m_selected; }
void Bricks::setSelected(int i)
{
  m_selected = i;
  deactivateBounds();
  if (m_selected > 0)
    activateBounds(m_selected);
}

BrickInformation Bricks::brickInformation(int bno)
{
  if (bno < m_bricks.size())
    return m_bricks[bno];

  BrickInformation binfo;
  return binfo;
}

double* Bricks::getMatrix() { return m_Xform[0]; }
double* Bricks::getMatrixInv() { return m_XformInv[0]; }
Vec
Bricks::getPivot()
{
  Vec pivot = m_dataMin + VECPRODUCT(m_dataSize, m_bricks[0].getCorrectedPivot());
  return pivot;
}
Vec Bricks::getAxis() { return m_bricks[0].axis.unit(); }
float Bricks::getAngle() { return m_bricks[0].angle; }
Vec
Bricks::getTranslation()
{
  Vec translation = VECPRODUCT(m_dataSize,m_bricks[0].position);
  return translation;
}
Vec
Bricks::getScalePivot()
{
  Vec scalepivot = m_dataMin + VECPRODUCT(m_dataSize, m_bricks[0].getCorrectedScalePivot());
  return scalepivot;
}
Vec Bricks::getScale() { return m_bricks[0].scale; }


double* Bricks::getMatrix(int i) { return m_Xform[i]; }
double* Bricks::getMatrixInv(int i) { return m_XformInv[i]; }
Vec
Bricks::getPivot(int i)
{
  Vec pivot = m_dataMin + VECPRODUCT(m_dataSize, m_bricks[i].getCorrectedPivot());
  return pivot;
}
Vec Bricks::getAxis(int i) { return m_bricks[i].axis.unit(); }
float Bricks::getAngle(int i) { return m_bricks[i].angle; }
Vec
Bricks::getTranslation(int i)
{
  Vec translation = VECPRODUCT(m_dataSize,m_bricks[i].position);
  return translation;
}
Vec
Bricks::getScalePivot(int i)
{
  Vec scalepivot = m_dataMin + VECPRODUCT(m_dataSize, m_bricks[i].getCorrectedScalePivot());
  return scalepivot;
}
Vec Bricks::getScale(int i) { return m_bricks[i].scale; }


bool Bricks::updateFlag() { return m_updateFlag; }
void Bricks::resetUpdateFlag() { m_updateFlag = false; }

void
Bricks::addClipper()
{
  for(int i=0; i<m_bricks.size(); i++)
    m_bricks[i].clippers.append(true);
  m_updateFlag = true;
}
void
Bricks::removeClipper(int ci)
{
  for(int i=0; i<m_bricks.size(); i++)
    {
      QList<bool>::iterator it = m_bricks[i].clippers.begin() + ci;
      m_bricks[i].clippers.erase(it);
    }
  m_updateFlag = true;
}

Bricks::Bricks()
{
  m_updateFlag = false;
  m_showAxis = false;

  m_selected = -1;

  m_dataMin = Vec(0,0,0);
  m_dataMax = Vec(0,0,0);
  m_dataSize = m_dataMax - m_dataMin;

  m_bricks.clear();
  m_ghostBricks.clear();
  m_brickBox.clear();
  m_Xform.clear();
  m_XformInv.clear();

  BrickInformation brickInfo;
  m_bricks.append(brickInfo);

  double *xform = new double[16];
  Matrix::identity(xform);
  m_Xform.append(xform);

  double *xformInv = new double[16];
  Matrix::identity(xformInv);
  m_XformInv.append(xformInv);

  BrickBounds brickBounds;
  m_ghostBricks.append(brickBounds);

  // do not allow changing of bounds for zeroeth brick
  // zeroeth brick always occupies the whole subvolume
  BoundingBox *bbox = new BoundingBox();
  bbox->deactivateBounds();
  m_brickBox.append(bbox);
}

Bricks::~Bricks()
{
  m_bricks.clear();
  m_ghostBricks.clear();

  for(int i=0; i<m_Xform.size(); i++)
    delete [] m_Xform[i];
  m_Xform.clear();

  for(int i=0; i<m_XformInv.size(); i++)
    delete [] m_XformInv[i];
  m_XformInv.clear();

  for(int i=0; i<m_brickBox.size(); i++)
    delete m_brickBox[i];
  m_brickBox.clear();
}

void
Bricks::reset()
{
  m_updateFlag = true;

  m_bricks.clear();
  m_ghostBricks.clear();

  for(int i=0; i<m_Xform.size(); i++)
    delete [] m_Xform[i];
  m_Xform.clear();

  for(int i=0; i<m_XformInv.size(); i++)
    delete [] m_XformInv[i];
  m_XformInv.clear();

  for(int i=0; i<m_brickBox.size(); i++)
    delete m_brickBox[i];
  m_brickBox.clear();

  m_dataMin = Vec(0,0,0);
  m_dataMax = Vec(0,0,0);
  m_dataSize = m_dataMax - m_dataMin;

  BrickInformation brickInfo;
  m_bricks.append(brickInfo);  

  double *xform = new double[16];
  Matrix::identity(xform);
  m_Xform.append(xform);

  double *xformInv = new double[16];
  Matrix::identity(xformInv);
  m_XformInv.append(xformInv);

  BrickBounds brickBounds;
  m_ghostBricks.append(brickBounds);

  // do not allow changing of bounds for zeroeth brick
  // zeroeth brick always occupies the whole subvolume
  BoundingBox *bbox = new BoundingBox();
  bbox->deactivateBounds();
  m_brickBox.append(bbox);
}

void
Bricks::activateBounds(int i)
{
  if (i<m_brickBox.size())
    m_brickBox[i]->activateBounds();
}

void
Bricks::activateBounds()
{
  // do not allow changing of bounds for zeroeth brick
  // zeroeth brick always occupies the whole subvolume
  for(int i=1; i<m_brickBox.size(); i++)
    m_brickBox[i]->activateBounds();
}

void
Bricks::deactivateBounds()
{
  // do not allow changing of bounds for zeroeth brick
  // zeroeth brick always occupies the whole subvolume
  for(int i=1; i<m_brickBox.size(); i++)
    m_brickBox[i]->deactivateBounds();
}

void
Bricks::updateScaling()
{
  Vec voxelScaling = Global::voxelScaling();

  for(int i=0; i<m_brickBox.size(); i++)
    {
      Vec bmin, bmax;

      bmin = VECPRODUCT(m_dataMin, voxelScaling);
      bmax = VECPRODUCT(m_dataMax, voxelScaling);
      m_brickBox[i]->setBounds(bmin, bmax);

      bmin = m_dataMin + VECPRODUCT(m_bricks[i].brickMin, m_dataSize);
      bmax = m_dataMin + VECPRODUCT(m_bricks[i].brickMax, m_dataSize);
      bmin = VECPRODUCT(bmin, voxelScaling);
      bmax = VECPRODUCT(bmax, voxelScaling);
      m_brickBox[i]->setPositions(bmin, bmax);
    }

  update();
}

void
Bricks::setBounds(Vec datamin, Vec datamax)
{
  m_dataMin = datamin;
  m_dataMax = datamax;
  m_dataSize = m_dataMax - m_dataMin;

  Vec voxelScaling = Global::voxelScaling();

  for(int i=0; i<m_brickBox.size(); i++)
    {
      Vec bmin, bmax;

      bmin = VECPRODUCT(datamin, voxelScaling);
      bmax = VECPRODUCT(datamax, voxelScaling);
      //m_brickBox[i]->setBounds(datamin, datamax);
      m_brickBox[i]->setBounds(bmin, bmax);

      bmin = m_dataMin + VECPRODUCT(m_bricks[i].brickMin, m_dataSize);
      bmax = m_dataMin + VECPRODUCT(m_bricks[i].brickMax, m_dataSize);
      bmin = VECPRODUCT(bmin, voxelScaling);
      bmax = VECPRODUCT(bmax, voxelScaling);
      m_brickBox[i]->setPositions(bmin, bmax);
    }

  update();
}

bool
Bricks::keyPressEvent(QKeyEvent *event)
{
  bool doupdate = false;

  // do not allow any updates for zeroeth brick
  // zeroeth brick always occupies the whole subvolume
  for(int i=1; i<m_brickBox.size(); i++)
    doupdate |= m_brickBox[i]->keyPressEvent(event);

  if (doupdate)
    update();

  return doupdate;
}

void
Bricks::genMatrices()
{
  QList<double*> mat;

  Vec voxelScaling = Global::voxelScaling();
  for(int i=0; i<m_Xform.size(); i++)
    {
      Vec translation = VECPRODUCT(getTranslation(i), voxelScaling);
      Vec pivot = VECPRODUCT(getPivot(i),voxelScaling);
      Vec axis = getAxis(i);
      float angle = DEG2RAD(getAngle(i));
      double *Xform = new double[16];
      Matrix::createTransformationMatrix(Xform,
					 translation,
					 pivot, axis, angle);
      mat.append(Xform);
    }

  for(int i=0; i<m_Xform.size(); i++)
    {
      double Xform[16];	  
      bool done = false;
      Matrix::identity(Xform);
      int plinkBrick = i;
      memcpy(Xform, mat[i], 16*sizeof(double));
      while(!done)
	{
	  int linkBrick = m_bricks[plinkBrick].linkBrick;
	  if (linkBrick == plinkBrick)
	    done = true;
	  else
	    {
	      Matrix::matmult(mat[linkBrick], Xform, m_Xform[i]);
	      memcpy(Xform, m_Xform[i], 16*sizeof(double));
	      plinkBrick = linkBrick;
	    }
	}
      memcpy(m_Xform[i], Xform, 16*sizeof(double));

      double XformInv[16];
      Matrix::inverse(Xform, XformInv);
      memcpy(m_XformInv[i], XformInv, 16*sizeof(double));
    }

  for(int i=0; i<mat.size(); i++)
    delete [] mat[i];
  mat.clear();
}

void
Bricks::setBrick(int bno, BrickInformation binfo)
{
  if (bno >= m_bricks.size())
    return;

  m_bricks[bno] = binfo;

  if (bno > 0)
    update();
  else
    {
      genMatrices();
      m_updateFlag = true;
    }
}

void
Bricks::setBricks(QList<BrickInformation> binfo)
{
  deactivateBounds();
  m_bricks.clear();

  if (binfo.size() > 0)
    m_bricks = binfo;
  else
    {
      BrickInformation brickInfo;
      m_bricks.append(brickInfo);
    }

  for(int i=0; i<m_brickBox.size(); i++)
    delete m_brickBox[i];
  m_brickBox.clear();

  Vec voxelScaling = Global::voxelScaling();

  BoundingBox *bbox = new BoundingBox();
  m_brickBox.append(bbox);
  Vec bmin = VECPRODUCT(m_dataMin, voxelScaling);
  Vec bmax = VECPRODUCT(m_dataMax, voxelScaling);
  bbox->setBounds(bmin, bmax);
  // do not allow changing of bounds for zeroeth brick
  // zeroeth brick always occupies the whole subvolume
  m_brickBox[0]->deactivateBounds();

  for(int i=1; i<m_bricks.size(); i++)
    {
      Vec bmin, bmax;
      BoundingBox *bbox = new BoundingBox();

      bmin = VECPRODUCT(m_dataMin, voxelScaling);
      bmax = VECPRODUCT(m_dataMax, voxelScaling);
      //bbox->setBounds(m_dataMin, m_dataMax);
      bbox->setBounds(bmin, bmax);

      bmin = m_dataMin + VECPRODUCT(m_bricks[i].brickMin, m_dataSize);
      bmax = m_dataMin + VECPRODUCT(m_bricks[i].brickMax, m_dataSize);
      bmin = VECPRODUCT(bmin, voxelScaling);
      bmax = VECPRODUCT(bmax, voxelScaling);
      bbox->setPositions(bmin, bmax);

      connect(bbox, SIGNAL(updated()), this, SLOT(update()));
      m_brickBox.append(bbox);
    }


  for(int i=1; i<m_bricks.size(); i++)
    m_brickBox[i]->activateBounds();


  // -- allocate matrices
  for(int i=0; i<m_Xform.size(); i++)
    delete [] m_Xform[i];
  m_Xform.clear();

  for(int i=0; i<m_XformInv.size(); i++)
    delete [] m_XformInv[i];
  m_XformInv.clear();

  for(int i=0; i<m_bricks.size(); i++)
    {
      double *xform = new double[16];
      Matrix::identity(xform);
      m_Xform.append(xform);

      double *xformInv = new double[16];
      Matrix::identity(xformInv);
      m_XformInv.append(xformInv);
    }


  update();
}

void
Bricks::addBrick(BrickInformation binfo)
{
  m_bricks.append(binfo);

  BoundingBox *bbox = new BoundingBox();
  Vec voxelScaling = Global::voxelScaling();
  Vec bmin = VECPRODUCT(m_dataMin, voxelScaling);
  Vec bmax = VECPRODUCT(m_dataMax, voxelScaling);
  bbox->setBounds(bmin, bmax);
  if (m_brickBox.size() > 0)
    {
      bbox->activateBounds();
      connect(bbox, SIGNAL(updated()), this, SLOT(update()));
    }
  m_brickBox.append(bbox);

  double *xform = new double[16];
  Matrix::identity(xform);
  m_Xform.append(xform);

  double *xformInv = new double[16];
  Matrix::identity(xformInv);
  m_XformInv.append(xformInv);

  update();
}

void
Bricks::addBrick()
{
  BrickInformation binfo;
  for(int ci=0; ci<m_bricks[0].clippers.size(); ci++)
    binfo.clippers.append(true);
  m_bricks.append(binfo);

  BoundingBox *bbox = new BoundingBox();
  Vec voxelScaling = Global::voxelScaling();
  Vec bmin = VECPRODUCT(m_dataMin, voxelScaling);
  Vec bmax = VECPRODUCT(m_dataMax, voxelScaling);
  bbox->setBounds(bmin, bmax);
  if (m_brickBox.size() > 0)
    {
      bbox->activateBounds();
      connect(bbox, SIGNAL(updated()), this, SLOT(update()));
    }
  m_brickBox.append(bbox);

  double *xform = new double[16];
  Matrix::identity(xform);
  m_Xform.append(xform);

  double *xformInv = new double[16];
  Matrix::identity(xformInv);
  m_XformInv.append(xformInv);

  update();  
}

void
Bricks::removeBrick(int bno)
{
  if (bno == 0 || bno >= m_brickBox.size())
    return;

  delete m_brickBox[bno];
  delete [] m_Xform[bno];
  delete [] m_XformInv[bno];

  m_brickBox.removeAt(bno);
  m_Xform.removeAt(bno);
  m_XformInv.removeAt(bno);
  m_bricks.removeAt(bno);

  update();
}

void
Bricks::drawAxisAngle(double *xform, int bno)
{
  if (!m_showAxis)
    return;

  Vec voxelScaling = Global::voxelScaling();
  Vec pivot = VECPRODUCT(getPivot(bno),voxelScaling);
  Vec axis = getAxis(bno);
  float len = (m_dataSize.x+m_dataSize.y+m_dataSize.z)/6;
  Vec p1 = pivot + len * axis;
  p1 = Matrix::xformVec(xform, p1);
  Vec p2 = pivot - len * axis;
  p2 = Matrix::xformVec(xform, p2);

  Vec p0 = Matrix::xformVec(xform, pivot);

  glLineWidth(5);
  glColor3f(1.0f, 0.3f, 0.1f);
  glBegin(GL_LINES);
  glVertex3fv(p1);
  glVertex3fv(p2);
  glEnd();

  glLineWidth(3);
  glColor3f(0.5f,0,0);
  glBegin(GL_LINES);
  glVertex3fv(p1);
  glVertex3fv(p2);
  glEnd();

  glPointSize(7);
  glColor3f(0.4f, 1.0f, 0.6f);
  glBegin(GL_POINTS);
  glVertex3fv(p0);
  glEnd();

  glPointSize(5);
  glColor3f(0,0.5f,0);
  glBegin(GL_POINTS);
  glVertex3fv(p0);
  glEnd();

  glLineWidth(1);
}

void
Bricks::draw()
{
 if (m_selected < 0)
   return;

  Vec voxelScaling = Global::voxelScaling();

  Vec lineColor = Vec(0.9f, 0.6f, 0.0f);
  Vec bgcolor = Global::backgroundColor();
  float bgintensity = (0.3*bgcolor.x +
		       0.5*bgcolor.y +
		       0.2*bgcolor.z);
  if (bgintensity > 0.5)
    lineColor = Vec(0.3f, 0.2f, 0.0f);

//-------------------------------------------------------------------------------
//-- draw only for debugging
//  for(int i=0; i<m_ghostBricks.size(); i++)
//    {
//      Vec dataMin = m_dataMin +
//	            VECPRODUCT(m_ghostBricks[i].brickMin, m_dataSize);
//      Vec dataMax = m_dataMin +
//	            VECPRODUCT(m_ghostBricks[i].brickMax, m_dataSize);
//      dataMin = VECPRODUCT(dataMin, voxelScaling);
//      dataMax = VECPRODUCT(dataMax, voxelScaling);
//      StaticFunctions::drawEnclosingCubeWithTransformation(dataMin,
//							   dataMax,
//							   m_Xform[0],   
//							   Vec(0.5f, 0.5f, 0.7f));
//    }
//-------------------------------------------------------------------------------

  if (m_selected == 0)
    {
      drawAxisAngle(m_Xform[0], m_selected);
    }
  else
    {
      Vec dataMin = m_dataMin +
	            VECPRODUCT(m_bricks[m_selected].brickMin, m_dataSize);
      Vec dataMax = m_dataMin +
	            VECPRODUCT(m_bricks[m_selected].brickMax, m_dataSize);
      dataMin = VECPRODUCT(dataMin, voxelScaling);
      dataMax = VECPRODUCT(dataMax, voxelScaling);

      StaticFunctions::drawEnclosingCubeWithTransformation(dataMin,
							   dataMax,
							   m_Xform[m_selected],   
							   lineColor);

      drawAxisAngle(m_Xform[m_selected], m_selected);

      m_brickBox[m_selected]->draw();
    }
}

//-----
#define CHECKSPLIT()						\
  int a,b;							\
  if (k==0) { a=1; b=2; }					\
  else if (k==1) { a=0; b=2; }					\
  else if (k==2) { a=0; b=1; }					\
  if ((int)(gbmin[a]) >= (int)(bmax[a]) ||			\
      (int)(gbmax[a]) <= (int)(bmin[a]) ||			\
      (int)(gbmin[b]) >= (int)(bmax[b]) ||			\
      (int)(gbmax[b]) <= (int)(bmin[b]))			\
    {								\
      gbrick2.append(BrickBounds(gbmin, gbmax));		\
    }								\
  else
//-----

void
Bricks::update()
{
  QList<BrickBounds> gbrick1;
  Vec voxelScaling = Global::voxelScaling();

//  Vec dmin = VECPRODUCT(m_dataMin, voxelScaling);
//  Vec dmax = VECPRODUCT(m_dataMax, voxelScaling);
//  BrickBounds ghost(dmin, dmax);

  BrickBounds ghost(m_dataMin, m_dataMax);
  gbrick1.append(ghost);

  m_updateFlag = true;

  for(int bno=1; bno<m_brickBox.size(); bno++)
    {
      Vec bmin, bmax;
      m_brickBox[bno]->bounds(bmin, bmax);

      bmin = VECDIVIDE(bmin, voxelScaling);
      bmax = VECDIVIDE(bmax, voxelScaling);

      bmin = StaticFunctions::clampVec(m_dataMin, m_dataMax, bmin);
      bmax = StaticFunctions::clampVec(m_dataMin, m_dataMax, bmax);

      // first update brickMin and brickMax
      m_bricks[bno].brickMin = VECDIVIDE((bmin-m_dataMin), m_dataSize);
      m_bricks[bno].brickMax = VECDIVIDE((bmax-m_dataMin), m_dataSize);

      for(int k=0; k<3; k++) // loop for x,y,z
	{
	  QList<BrickBounds> gbrick2;
	  gbrick2.clear();

	  // 6 cases for each x,y,z
	  // case 1 : gbmin < bmin && gbmax <= bmin -- don't split
	  // case 2 : gbmin >= bmax && gbmax > bmax -- don't split
	  // case 3 : gbmin < bmin && gbmax > bmin && gbmax <= bmax -- split at bmin
	  // case 4 : gbmin >= bmin && gbmin < bmax && gbmax > bmax -- split at bmax
	  // case 5 : gbmin < bmin && gbmax > bmax -- two splits at bmin and bmax
	  // case 6 : gbmin > bmin && gbmax < bmax -- might need deletion

	  for(int j=0; j<gbrick1.size(); j++)
	    {
	      Vec gbmin = gbrick1[j].brickMin;
	      Vec gbmax = gbrick1[j].brickMax;
	      //case 1
	      if ((int)(gbmax[k]) <= (int)(bmin[k]))
		{
		  gbrick2.append(BrickBounds(gbmin, gbmax));
		}
	      // case 2
	      else if ((int)(gbmin[k]) >=  (int)(bmax[k]))
		{
		  gbrick2.append(BrickBounds(gbmin, gbmax));
		}
	      // case 3
	      else if ((int)(gbmin[k]) <  (int)(bmin[k]) &&
		       (int)(gbmax[k]) >  (int)(bmin[k]) &&
		       (int)(gbmax[k]) <= (int)(bmax[k]))
		{
		  CHECKSPLIT()
		    {
		      // split at bmin[k]
		      Vec gmin, gmax;
		      gmin = gbmin;
		      gmax = gbmax;
		      gmax[k] = bmin[k];
		      gbrick2.append(BrickBounds(gmin, gmax));

		      gmin = gbmin;
		      gmax = gbmax;
		      gmin[k] = bmin[k];
		      gbrick2.append(BrickBounds(gmin, gmax));
		    }
		}
	      // case 4
	      else if ((int)(gbmin[k]) >= (int)(bmin[k]) &&
		       (int)(gbmin[k]) <  (int)(bmax[k]) &&
		       (int)(gbmax[k]) >  (int)(bmax[k]))
		{
		  CHECKSPLIT()
		    {
		      // split at bmax[k]
		      Vec gmin, gmax;
		      gmin = gbmin;
		      gmax = gbmax;
		      gmin[k] = bmax[k];
		      gbrick2.append(BrickBounds(gmin, gmax));
		      
		      gmin = gbmin;
		      gmax = gbmax;
		      gmax[k] = bmax[k];
		      gbrick2.append(BrickBounds(gmin, gmax));
		    }
		}
	      // case 5
	      else if ((int)(gbmin[k]) < (int)(bmin[k]) &&
		       (int)(gbmax[k]) > (int)(bmax[k]))
		{
		  CHECKSPLIT()
		    {
		      // split at bmin[k] and bmax[k]
		      Vec gmin, gmax;
		      gmin = gbmin;
		      gmax = gbmax;
		      gmax[k] = bmin[k];
		      gbrick2.append(BrickBounds(gmin, gmax));
		      
		      gmin = gbmin;
		      gmax = gbmax;
		      gmin[k] = bmax[k];
		      gbrick2.append(BrickBounds(gmin, gmax));
		      
		      gmin = gbmin;
		      gmax = gbmax;
		      gmin[k] = bmin[k];
		      gmax[k] = bmax[k];
		      gbrick2.append(BrickBounds(gmin, gmax));
		    }
		}
	      // case 6
	      else if ((int)(gbmin[k]) >= (int)(bmin[k]) &&
		       (int)(gbmax[k]) <= (int)(bmax[k]))
		{
		  gbrick2.append(BrickBounds(gbmin, gbmax));
		}
	    } // j loop

	  gbrick1.clear();
	  gbrick1 += gbrick2;
	} // k loop for x,y,z
    }

  QList<BrickBounds> gbrick2;
  gbrick2.clear();
  for(int i=0; i<gbrick1.size(); i++)
    {
      bool flag = true; // survived
      Vec gbmin = gbrick1[i].brickMin;
      Vec gbmax = gbrick1[i].brickMax;
      // don't want to check with brick0
      if ((gbmax.x-gbmin.x) < 2 ||
	  (gbmax.y-gbmin.y) < 2 ||
	  (gbmax.z-gbmin.z) < 2)
	flag = false; // don't want zero sized bricks
      else
	{
	  for(int b=1; b<m_brickBox.size(); b++)
	    {
	      Vec bmin, bmax;
	      m_brickBox[b]->bounds(bmin, bmax);
	      bmin = VECDIVIDE(bmin, voxelScaling);
	      bmax = VECDIVIDE(bmax, voxelScaling);
	      bmin = StaticFunctions::clampVec(m_dataMin, m_dataMax, bmin);
	      bmax = StaticFunctions::clampVec(m_dataMin, m_dataMax, bmax);
	      
	      if ((int)gbmin.x >= (int)bmin.x &&
		  (int)gbmax.x <= (int)bmax.x &&
		  (int)gbmin.y >= (int)bmin.y &&
		  (int)gbmax.y <= (int)bmax.y &&
		  (int)gbmin.z >= (int)bmin.z &&
		  (int)gbmax.z <= (int)bmax.z)
		{
		  flag = false;
		  break;
		}
	    }
	}
      if (flag)
	gbrick2.append(gbrick1[i]);
    }

  // save brick0 information before updating
  m_ghostBricks.clear();
  for(int i=0; i<gbrick2.size(); i++)
    {      
      BrickBounds brk;
      // gbrick2 contains brick bounds in voxel coordinates
      Vec gbmin = gbrick2[i].brickMin;
      Vec gbmax = gbrick2[i].brickMax;
      // convert these to normalized coordinates before copying
      brk.brickMin = VECDIVIDE((gbmin-m_dataMin), m_dataSize);
      brk.brickMax = VECDIVIDE((gbmax-m_dataMin), m_dataSize);
      m_ghostBricks.append(brk);
    }

  genMatrices();

  emit refresh(); // refresh brickswidget contents
}
