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


void
Slicer3D::drawSlices(Vec bbmin, Vec bbmax, Vec dataMax,
		     Vec pn, Vec minvert, Vec maxvert,
		     int layers, float stepsize,
		     QList<Vec> cpos, QList<Vec> cnorm)
{
  float xmin, xmax, ymin, ymax, zmin, zmax;
  Vec subdim, subcorner, subvol[8];

//  Vec bbmin, bbmax;
//  m_boundingBox.bounds(bbmin, bbmax);
  subcorner = bbmin;
  subdim = bbmax-bbmin;

  subvol[0] = Vec(bbmin.x, bbmin.y, bbmin.z);
  subvol[1] = Vec(bbmax.x, bbmin.y, bbmin.z);
  subvol[2] = Vec(bbmax.x, bbmax.y, bbmin.z);
  subvol[3] = Vec(bbmin.x, bbmax.y, bbmin.z);
  subvol[4] = Vec(bbmin.x, bbmin.y, bbmax.z);
  subvol[5] = Vec(bbmax.x, bbmin.y, bbmax.z);
  subvol[6] = Vec(bbmax.x, bbmax.y, bbmax.z);
  subvol[7] = Vec(bbmin.x, bbmax.y, bbmax.z);

  Vec step = stepsize*pn;
  Vec po = minvert+layers*step;
  for(int s=0; s<layers; s++)
    {
      po -= step;
      drawpoly(po, pn, subvol, subdim, subcorner, dataMax,
	       cpos, cnorm);
    }

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
		   Vec subdim, Vec subcorner,
		   Vec dataMax,
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
      Vec tx = tpoly[i];
      Vec tc = VECDIVIDE(tx,dataMax);

      glTexCoord3f(tc.x, tc.y, tc.z);
      glVertex3f(tx.x, tx.y, tx.z);
    }
  glEnd();


  return 1;
}

