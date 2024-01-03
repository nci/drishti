#include "brickinformation.h"
#include "staticfunctions.h"

//----------------------------------
int DrawBrickInformation::subvolSize() { return m_subvol.size(); }
Vec DrawBrickInformation::subvol(int i)
{
  if (i < m_subvol.size())
    return m_subvol[i];
  else
    return Vec(0,0,0);
}
DrawBrickInformation::DrawBrickInformation()
{
  m_numBricks = 0;
  m_tfSet.clear();
  m_subvol.clear();
  m_subcorner.clear();
  m_subdim.clear();
  m_clips.clear();
  m_scalepivot.clear();
  m_scale.clear();
  m_texture.clear();
}
DrawBrickInformation::~DrawBrickInformation()
{
  m_numBricks = 0;
  m_tfSet.clear();
  m_subvol.clear();
  m_subcorner.clear();
  m_subdim.clear();
  m_scalepivot.clear();
  m_scale.clear();
  m_texture.clear();

  if (m_clips.size() > 0)
    {
      for(int i=0; i<m_clips.size(); i++)
	m_clips[i].clear();
    }
  m_clips.clear();
}
void
DrawBrickInformation::reset()
{
  m_numBricks = 0;
  m_tfSet.clear();
  m_subvol.clear();
  m_subcorner.clear();
  m_subdim.clear();
  m_scalepivot.clear();
  m_scale.clear();
  m_texture.clear();

  if (m_clips.size() > 0)
    {
      for(int i=0; i<m_clips.size(); i++)
	m_clips[i].clear();
    }
  m_clips.clear();
}
int DrawBrickInformation::numBricks() { return m_numBricks; }
bool
DrawBrickInformation::get(int i,
			  int& tfset,
			  Vec *subvol,
			  Vec *texture,
			  Vec& subcorner, Vec& subdim,
			  QList<bool>& clips,
			  Vec& scalepivot, Vec& scale)
{
  if (i >= m_numBricks)
    return false;

  tfset = m_tfSet[i];

  for (int j=0; j<8; j++)
    subvol[j] = m_subvol[8*i+j];

  for (int j=0; j<8; j++)
    texture[j] = m_texture[8*i+j];

  subcorner = m_subcorner[i];
  subdim = m_subdim[i];

  clips = m_clips[i];

  scalepivot = m_scalepivot[i];
  scale = m_scale[i];
  
  return true;
}
void
DrawBrickInformation::append( int tfset,
			      Vec *subvol,
			      Vec *texture,
			      Vec subcorner, Vec subdim,
			      QList<bool> clips,
			      Vec scalepivot, Vec scale)
{
  m_tfSet.append(tfset);

  for (int j=0; j<8; j++)
    m_subvol.append(subvol[j]);

  for (int j=0; j<8; j++)
    m_texture.append(texture[j]);

  m_subcorner.append(subcorner);
  m_subdim.append(subdim);

  m_clips.append(clips);

  m_scalepivot.append(scalepivot);
  m_scale.append(scale);

  m_numBricks ++;
}
//----------------------------------


//----------------------------------
BrickBounds::BrickBounds()
{
  brickMin = Vec(0,0,0);
  brickMax = Vec(1,1,1);
}
BrickBounds::BrickBounds(Vec bmin, Vec bmax)
{
  brickMin = bmin;
  brickMax = bmax;
}
BrickBounds&
BrickBounds::operator=(const BrickBounds &bb)
{
  brickMin = bb.brickMin;
  brickMax = bb.brickMax;
  return *this;
}
//----------------------------------

Vec
BrickInformation::getCorrectedPivot()
{
  Vec sz = brickMax - brickMin;
  Vec p = brickMin + VECPRODUCT(pivot, sz);
  return p;
}
Vec
BrickInformation::getCorrectedScalePivot()
{
  Vec sz = brickMax - brickMin;
  Vec p = brickMin + VECPRODUCT(scalepivot, sz);
  return p;
}

void
BrickInformation::reset()
{
  tfSet = 0;
  linkBrick = 0;
  brickMin = Vec(0,0,0);
  brickMax = Vec(1,1,1);
  position = Vec(0,0,0);
  pivot = Vec(0.5,0.5,0.5);
  axis = Vec(1,0,0);
  angle = 0;
  scalepivot = Vec(0.5,0.5,0.5);
  scale = Vec(1,1,1);

  clippers.clear();
}

BrickInformation::BrickInformation()
{
  reset();
}
BrickInformation::~BrickInformation()
{
  reset();
}

BrickInformation::BrickInformation(const BrickInformation& bi)
{
  tfSet = bi.tfSet;
  linkBrick = bi.linkBrick;
  brickMin = bi.brickMin;
  brickMax = bi.brickMax;
  position = bi.position;
  pivot = bi.pivot;
  axis = bi.axis;
  angle = bi.angle;
  scalepivot = bi.scalepivot;
  scale = bi.scale;

  clippers = bi.clippers;
}

BrickInformation&
BrickInformation::operator=(const BrickInformation& bi)
{
  tfSet = bi.tfSet;
  linkBrick = bi.linkBrick;
  brickMin = bi.brickMin;
  brickMax = bi.brickMax;
  position = bi.position;
  pivot = bi.pivot;
  axis = bi.axis;
  angle = bi.angle;
  scalepivot = bi.scalepivot;
  scale = bi.scale;

  clippers = bi.clippers;

  return *this;
}

BrickInformation
BrickInformation::interpolate(const BrickInformation brickInfo1,
			      const BrickInformation brickInfo2,
			      float frc)
{
  BrickInformation brickInfo;

  brickInfo.tfSet = brickInfo1.tfSet;
  brickInfo.linkBrick = brickInfo1.linkBrick;
  
  brickInfo.brickMin = StaticFunctions::interpolate(brickInfo1.brickMin,
						    brickInfo2.brickMin,
						    frc);

  brickInfo.brickMax = StaticFunctions::interpolate(brickInfo1.brickMax,
						    brickInfo2.brickMax,
						    frc);

  brickInfo.position = StaticFunctions::interpolate(brickInfo1.position,
						    brickInfo2.position,
						    frc);

  brickInfo.pivot = StaticFunctions::interpolate(brickInfo1.pivot,
						    brickInfo2.pivot,
						    frc);

  brickInfo.scalepivot = StaticFunctions::interpolate(brickInfo1.scalepivot,
						      brickInfo2.scalepivot,
						      frc);

  brickInfo.scale = StaticFunctions::interpolate(brickInfo1.scale,
						 brickInfo2.scale,
						 frc);

  if ((brickInfo1.axis-brickInfo2.axis).squaredNorm() < 0.001)
    { // same axis
      brickInfo.axis = brickInfo1.axis;
      brickInfo.angle = brickInfo1.angle + frc*(brickInfo2.angle-
						brickInfo1.angle);
    }
  else
    {
      if (fabs(brickInfo1.angle) < 0.001 &&
	  fabs(brickInfo2.angle) < 0.001)
	{ // axis change for 0 angle
	  if (frc < 0.5)
	    {
	      brickInfo.axis = brickInfo1.axis;
	      brickInfo.angle = brickInfo1.angle;
	    }
	  else
	    {
	      brickInfo.axis = brickInfo2.axis;
	      brickInfo.angle = brickInfo2.angle;
	    }
	}
      else
	{
	  Quaternion q, q1, q2;
	  q1 = Quaternion(brickInfo1.axis, DEG2RAD(brickInfo1.angle));
	  q2 = Quaternion(brickInfo2.axis, DEG2RAD(brickInfo2.angle));
	  q = Quaternion::slerp(q1, q2, frc);
	  brickInfo.axis = q.axis();
	  brickInfo.angle = RAD2DEG(q.angle());
	}
    }


  if (brickInfo1.clippers.size() == brickInfo2.clippers.size())
    {
      brickInfo.clippers = brickInfo1.clippers;
    }
  else
    {
      for(int ci=0;
	  ci<qMin(brickInfo1.clippers.size(),
		  brickInfo2.clippers.size());
	  ci++)
	{
	  brickInfo.clippers.append(brickInfo1.clippers[ci]);
	}
      if (brickInfo2.clippers.size() < brickInfo1.clippers.size())
	{
	  for(int ci=brickInfo2.clippers.size();
	      ci<brickInfo1.clippers.size();
	      ci++)
	    {
	      brickInfo.clippers.append(brickInfo1.clippers[ci]);
	    }
	}
    }

  return brickInfo;
}


QList<BrickInformation>
BrickInformation::interpolate(const QList<BrickInformation> brickInfo1,
			      const QList<BrickInformation> brickInfo2,
			      float frc)
{
  int i;
  QList <BrickInformation> brickInfo;

  for(i=0; i<qMin(brickInfo1.size(), brickInfo2.size()); i++)
    brickInfo.append(interpolate(brickInfo1[i], brickInfo2[i], frc));

  if (brickInfo1.size() > brickInfo2.size())
    {
      for(i=brickInfo2.size(); i<brickInfo1.size(); i++)
	brickInfo.append(brickInfo1[i]);
    }

  return brickInfo;
}

void
BrickInformation::load(fstream &fin)
{
  bool done = false;
  char keyword[100];
  float f[3];
  
  reset();

  while(!done)
    { 
      fin.getline(keyword, 100, 0);
      if (strcmp(keyword, "end") == 0)
	done = true;
      else if (strcmp(keyword, "tfset") == 0)
	fin.read((char*)&tfSet, sizeof(int));
      else if (strcmp(keyword, "linkbrick") == 0)
	fin.read((char*)&linkBrick, sizeof(int));
      else if (strcmp(keyword, "brickmin") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  brickMin = Vec(f[0],f[1],f[2]);
	}
      else if (strcmp(keyword, "brickmax") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  brickMax = Vec(f[0],f[1],f[2]);
	}
      else if (strcmp(keyword, "position") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  position = Vec(f[0],f[1],f[2]);
	}
      else if (strcmp(keyword, "pivot") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  pivot = Vec(f[0],f[1],f[2]);
	}
      else if (strcmp(keyword, "axis") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  axis = Vec(f[0],f[1],f[2]);
	}
      else if (strcmp(keyword, "angle") == 0)
	fin.read((char*)&angle, sizeof(float));
      else if (strcmp(keyword, "scalepivot") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  scalepivot = Vec(f[0],f[1],f[2]);
	}
      else if (strcmp(keyword, "scale") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  scale = Vec(f[0],f[1],f[2]);
	}
      else if (strcmp(keyword, "clippers") == 0)
	{
	  int n;
	  fin.read((char*)&n, sizeof(int));
	  for(int i=0; i<n; i++)
	    {
	      bool b;
	      fin.read((char*)&b, sizeof(bool));
	      clippers.append(b);
	    }
	}
    }
}

void
BrickInformation::save(fstream &fout)
{
  char keyword[100];
  float f[3];

  memset(keyword, 0, 100);
  sprintf(keyword, "brickinformation");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "tfset");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&tfSet, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "linkbrick");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&linkBrick, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "brickmin");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = brickMin.x;
  f[1] = brickMin.y;
  f[2] = brickMin.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "brickmax");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = brickMax.x;
  f[1] = brickMax.y;
  f[2] = brickMax.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "position");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = position.x;
  f[1] = position.y;
  f[2] = position.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "pivot");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = pivot.x;
  f[1] = pivot.y;
  f[2] = pivot.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "axis");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = axis.x;
  f[1] = axis.y;
  f[2] = axis.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "angle");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&angle, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "scalepivot");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = scalepivot.x;
  f[1] = scalepivot.y;
  f[2] = scalepivot.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "scale");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = scale.x;
  f[1] = scale.y;
  f[2] = scale.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "clippers");
  fout.write((char*)keyword, strlen(keyword)+1);
  int n = clippers.size();
  fout.write((char*)&n, sizeof(int));
  for(int i=0; i<clippers.size(); i++)
    fout.write((char*)&clippers[i], sizeof(bool));


  memset(keyword, 0, 100);
  sprintf(keyword, "end");
  fout.write((char*)keyword, strlen(keyword)+1);
}
