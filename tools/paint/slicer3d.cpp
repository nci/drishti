#include <GL/glew.h>
#include "slicer3d.h"
#include "staticfunctions.h"

void
Slicer3D::getMinMaxVertices(Camera *cam,
			    Vec *box, float &zdepth,
			    Vec &minvert, Vec &maxvert)
{
 float mindepth, maxdepth;

 for (int i=0; i<8; i++)
   {
     Vec camCoord = cam->cameraCoordinatesOf(box[i]);
     float zval = camCoord.z;
     
      if (i == 0)
	{
	  mindepth = maxdepth = zval;
	  minvert = maxvert = box[i];
	}
      else
	{
	  if (zval < mindepth)
	    {
	      mindepth = zval;
	      minvert = box[i];
	    }

	  if (zval > maxdepth)
	    {
	      maxdepth = zval;
	      maxvert = box[i];
	    }
	}
    }

 if (maxdepth > 0) // camera is inside the volume
   {
     maxdepth = 0;
     maxvert = cam->position();
   }
 if (mindepth > 0)
   {
     mindepth = 0;
     minvert = cam->position();
   }
   

 zdepth = maxdepth - mindepth;
}


int
Slicer3D::intersectType1(Vec po, Vec pn,
			 Vec v0, Vec v1,
			 Vec &v)
{
  Vec v1m0 = v1-v0;
  float deno = pn*v1m0;
  if (fabs(deno) > 0.0001)
    {
      float t = pn*(po - v0)/deno;
      if (t >= 0 && t <= 1)
	{
	  v = v0 + v1m0*t;
	  return 1;
	}
    }
  return 0;
}

int
Slicer3D::intersectType2(Vec Po, Vec Pn,
			 Vec& v0, Vec& v1)
{
  float d0, d1;
  d0 = Pn*(v0-Po); 
  d1 = Pn*(v1-Po); 

  if (d0 > 0 && d1 > 0)
    // both points are clipped
    return 0;

  if (d0 <= 0 && d1 <= 0)
    // both points not clipped
    return 1;

  Vec Rnew, Rd;
  float RdPn;

  Rd = v1-v0;
  
  if (d0 > 0)
    Rd = -Rd;
  
  RdPn = Rd*Pn;

  if (d1 > 0) 
    {
      v1 = v0 + Rd * (((Po-v0)*Pn)/RdPn);
      return 2;
    }
  else
    {
      v0 = v1 + Rd * (((Po-v1)*Pn)/RdPn);
      return 1;
    }
}

int
Slicer3D::drawpoly(Vec po, Vec pn,
		   Vec *subvol,
		   Vec dataMin, Vec dataMax,
		   QList<Vec> cpos, QList<Vec> cnorm)
{
  Vec poly[10];
  int edges = 0;

  edges += intersectType1(po, pn,  subvol[0], subvol[1], poly[edges]);
  edges += intersectType1(po, pn,  subvol[0], subvol[3], poly[edges]);
  edges += intersectType1(po, pn,  subvol[2], subvol[1], poly[edges]);
  edges += intersectType1(po, pn,  subvol[2], subvol[3], poly[edges]);
  edges += intersectType1(po, pn,  subvol[4], subvol[5], poly[edges]);
  edges += intersectType1(po, pn,  subvol[4], subvol[7], poly[edges]);
  edges += intersectType1(po, pn,  subvol[6], subvol[5], poly[edges]);
  edges += intersectType1(po, pn,  subvol[6], subvol[7], poly[edges]);
  edges += intersectType1(po, pn,  subvol[0], subvol[4], poly[edges]);
  edges += intersectType1(po, pn,  subvol[1], subvol[5], poly[edges]);
  edges += intersectType1(po, pn,  subvol[2], subvol[6], poly[edges]);
  edges += intersectType1(po, pn,  subvol[3], subvol[7], poly[edges]);

  if (!edges) return 0;

  Vec cen;
  int i;
  for(i=0; i<edges; i++)
    cen += poly[i];
  cen/=edges;

  float angle[6];
  Vec vaxis, vperp;
  vaxis = poly[0]-cen;
  vaxis.normalize();

  vperp = vaxis^(poly[1]-cen);
  vperp = vperp^vaxis;
  vperp.normalize();

  angle[0] = 1;
  for(i=1; i<edges; i++)
    {
      Vec v;
      v = poly[i]-cen;
      v.normalize();
      angle[i] = vaxis*v;
      if (vperp*v < 0)
	angle[i] = -2 - angle[i];
    }

  // sort angle
  int order[] = {0, 1, 2, 3, 4, 5 };
  for(i=edges-1; i>=0; i--)
    for(int j=1; j<=i; j++)
      {
	if (angle[order[i]] < angle[order[j]])
	  {
	    int tmp = order[i];
	    order[i] = order[j];
	    order[j] = tmp;
	  }
      }


  //---- apply clipping
  int tedges;
  Vec tpoly[100];

  for(i=0; i<edges; i++)
    tpoly[i] = poly[order[i]];
  for(i=0; i<edges; i++)
    poly[i] = tpoly[i];

  //Vec voxelScaling = Global::voxelScaling();
  //---- apply clipping
  for(int ci=0; ci<cpos.count(); ci++)
    {
      Vec cpo = cpos[ci];
      Vec cpn =  cnorm[ci];
      //cpn =  VECPRODUCT(cpn, voxelScaling);
      
      tedges = 0;
      for(i=0; i<edges; i++)
	{
	  Vec v0, v1;
	  
	  v0 = poly[i];
	  if (i<edges-1)
	    v1 = poly[i+1];
	  else
	    v1 = poly[0];
	  
	  // clip on texture coordinates
	  int ret = intersectType2(cpo, cpn,
				   v0, v1);
	  if (ret)
	    {
	      tpoly[tedges] = v0;
	      tedges ++;
	      if (ret == 2)
		{
		  tpoly[tedges] = v1;
		  tedges ++;
		}
	    }
	}
      edges = tedges;
      for(i=0; i<tedges; i++)
	poly[i] = tpoly[i];
    }
  //---- clipping applied

  glBegin(GL_POLYGON);
  for(i=0; i<edges; i++)
    {  
      Vec tv = tpoly[i];
      Vec tx = tv - dataMin;
      tx = VECDIVIDE(tx,dataMax);

      glTexCoord3f(tx.x, tx.y, tx.z);
      glVertex3f(tv.x, tv.y, tv.z);
    }
  glEnd();


  return 1;
}

QList<int> Slicer3D::m_polyidx;
QList<Vec> Slicer3D::m_polyvt;

void
Slicer3D::start()
{
  m_polyidx.clear();
  m_polyvt.clear();
}

void
Slicer3D::draw()
{
//  GLuint *indices = new GLuint[m_polyidx.count()];
//  for(int i=0; i<m_polyidx.count(); i++)
//    indices[i] = i;
  
  GLfloat *vt = new GLfloat[3*m_polyvt.count()];
  for(int i=0; i<m_polyvt.count(); i++)
    {
      vt[3*i+0] = m_polyvt[i].x;
      vt[3*i+1] = m_polyvt[i].y;
      vt[3*i+2] = m_polyvt[i].z;
    }
  
  glEnableClientState(GL_VERTEX_ARRAY);
  glEnableClientState(GL_TEXTURE_COORD_ARRAY);

  glVertexPointer(3, GL_FLOAT, 6*sizeof(GLfloat), vt);
  glTexCoordPointer(3, GL_FLOAT, 6*sizeof(GLfloat), vt+3);
  
  //glDrawElements(GL_TRIANGLES, m_polyidx.count(), GL_UNSIGNED_INT, indices);
  glDrawArrays(GL_TRIANGLES, 0, m_polyidx.count());

  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableClientState(GL_TEXTURE_COORD_ARRAY);

  //delete [] indices;
  delete [] vt;
}

int
Slicer3D::genpoly(Vec po, Vec pn,
		  Vec *subvol,
		  Vec dataMin, Vec dataMax,
		  QList<Vec> cpos, QList<Vec> cnorm)
{
  Vec poly[10];
  int edges = 0;

  edges += intersectType1(po, pn,  subvol[0], subvol[1], poly[edges]);
  edges += intersectType1(po, pn,  subvol[0], subvol[3], poly[edges]);
  edges += intersectType1(po, pn,  subvol[2], subvol[1], poly[edges]);
  edges += intersectType1(po, pn,  subvol[2], subvol[3], poly[edges]);
  edges += intersectType1(po, pn,  subvol[4], subvol[5], poly[edges]);
  edges += intersectType1(po, pn,  subvol[4], subvol[7], poly[edges]);
  edges += intersectType1(po, pn,  subvol[6], subvol[5], poly[edges]);
  edges += intersectType1(po, pn,  subvol[6], subvol[7], poly[edges]);
  edges += intersectType1(po, pn,  subvol[0], subvol[4], poly[edges]);
  edges += intersectType1(po, pn,  subvol[1], subvol[5], poly[edges]);
  edges += intersectType1(po, pn,  subvol[2], subvol[6], poly[edges]);
  edges += intersectType1(po, pn,  subvol[3], subvol[7], poly[edges]);

  if (!edges) return 0;

  Vec cen;
  int i;
  for(i=0; i<edges; i++)
    cen += poly[i];
  cen/=edges;

  float angle[6];
  Vec vaxis, vperp;
  vaxis = poly[0]-cen;
  vaxis.normalize();

  vperp = vaxis^(poly[1]-cen);
  vperp = vperp^vaxis;
  vperp.normalize();

  angle[0] = 1;
  for(i=1; i<edges; i++)
    {
      Vec v;
      v = poly[i]-cen;
      v.normalize();
      angle[i] = vaxis*v;
      if (vperp*v < 0)
	angle[i] = -2 - angle[i];
    }

  // sort angle
  int order[] = {0, 1, 2, 3, 4, 5 };
  for(i=edges-1; i>=0; i--)
    for(int j=1; j<=i; j++)
      {
	if (angle[order[i]] < angle[order[j]])
	  {
	    int tmp = order[i];
	    order[i] = order[j];
	    order[j] = tmp;
	  }
      }


  //---- apply clipping
  int tedges;
  Vec tpoly[100];

  for(i=0; i<edges; i++)
    tpoly[i] = poly[order[i]];
  for(i=0; i<edges; i++)
    poly[i] = tpoly[i];

  //Vec voxelScaling = Global::voxelScaling();
  //---- apply clipping
  for(int ci=0; ci<cpos.count(); ci++)
    {
      Vec cpo = cpos[ci];
      Vec cpn =  cnorm[ci];
      //cpn =  VECPRODUCT(cpn, voxelScaling);
      
      tedges = 0;
      for(i=0; i<edges; i++)
	{
	  Vec v0, v1;
	  
	  v0 = poly[i];
	  if (i<edges-1)
	    v1 = poly[i+1];
	  else
	    v1 = poly[0];
	  
	  // clip on texture coordinates
	  int ret = intersectType2(cpo, cpn,
				   v0, v1);
	  if (ret)
	    {
	      tpoly[tedges] = v0;
	      tedges ++;
	      if (ret == 2)
		{
		  tpoly[tedges] = v1;
		  tedges ++;
		}
	    }
	}
      edges = tedges;
      for(i=0; i<tedges; i++)
	poly[i] = tpoly[i];
    }
  //---- clipping applied

  if (edges < 3)
    return 0;

//  glBegin(GL_POLYGON);
//  for(i=0; i<edges; i++)
//    {  
//      Vec tv = tpoly[i];
//      Vec tx = tv - dataMin;
//      tx = VECDIVIDE(tx,dataMax);
//
//      glTexCoord3f(tx.x, tx.y, tx.z);
//      glVertex3f(tv.x, tv.y, tv.z);
//    }
//  glEnd();

  for(i=0; i<edges-2; i++)
    {  
      Vec tv = tpoly[0];
      Vec tx = tv - dataMin;
      tx = VECDIVIDE(tx,dataMax);
      
      m_polyidx << m_polyvt.count();
      m_polyvt << tv;
      m_polyvt << tx;

      for(int t=i+1; t<=i+2; t++)
	{
	  Vec tv = tpoly[t];
	  Vec tx = tv - dataMin;
	  tx = VECDIVIDE(tx,dataMax);
	  
	  m_polyidx << m_polyvt.count();
	  m_polyvt << tv;
	  m_polyvt << tx;
	}
    }

  return 1;
}

