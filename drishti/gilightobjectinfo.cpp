#include "gilightobjectinfo.h"

GiLightObjectInfo::GiLightObjectInfo() { clear(); }
GiLightObjectInfo::~GiLightObjectInfo() { clear(); }

void
GiLightObjectInfo::clear()
{
  points.clear();
  allowInterpolation = false;
  show = true;
  doShadows = true;
  lightType = 0; // point type
  rad = 5;
  decay = 1.0;
  angle = 60.0;
  color = Vec(1,1,1);
  opacity = 1.0;
  segments = 1;
  lod = 1;
  smooth = 1;
}

GiLightObjectInfo&
GiLightObjectInfo::operator=(const GiLightObjectInfo& gi)
{
  points = gi.points;
  allowInterpolation = gi.allowInterpolation;
  show = gi.show;
  doShadows = gi.doShadows;
  lightType = gi.lightType;
  rad = gi.rad;
  decay = gi.decay;
  angle = gi.angle;
  color = gi.color;
  opacity = gi.opacity;
  segments = gi.segments;
  lod = gi.lod;
  smooth = gi.smooth;
  return *this;
}

GiLightObjectInfo
GiLightObjectInfo::interpolate(GiLightObjectInfo gi1,
			       GiLightObjectInfo gi2,
			       float frc)
{
  GiLightObjectInfo gi;

  gi.lod = gi1.lod;
  gi.smooth = gi1.smooth;
  gi.points = gi1.points;
  gi.allowInterpolation = gi1.allowInterpolation;
  gi.show = gi1.show;
  gi.doShadows = gi1.doShadows;
  gi.lightType = gi1.lightType;
  gi.rad = (1.0-frc)*gi1.rad + frc*gi2.rad;
  gi.decay = (1.0-frc)*gi1.decay + frc*gi2.decay;
  gi.angle = (1.0-frc)*gi1.angle + frc*gi2.angle;
  gi.color = (1.0-frc)*gi1.color + frc*gi2.color;  
  gi.opacity = (1.0-frc)*gi1.opacity + frc*gi2.opacity;  
  gi.segments = (1.0-frc)*gi1.segments + frc*gi2.segments;  

  if (gi1.allowInterpolation)
    {  
      QList<Vec> po1;
      QList<Vec> po2;
      po1 = gi1.points;
      po2 = gi2.points;

      Vec pi;
      QList<Vec> po;
      for(int i=0; i<po1.count(); i++)
	{
	  if (i < po2.count())
	    pi = (1-frc)*po1[i] + frc*po2[i];
	  else
	    pi = po1[i];
	  po.append(pi);
	}
      gi.points = po;
    }

  return gi;
}

QList<GiLightObjectInfo>
GiLightObjectInfo::interpolate(QList<GiLightObjectInfo> po1,
			       QList<GiLightObjectInfo> po2,
			       float frc)
{
  QList<GiLightObjectInfo> po;
  for(int i=0; i<po1.count(); i++)
    {
      GiLightObjectInfo pi;
      if (i < po2.count())
	{
	  pi = interpolate(po1[i], po2[i], frc);
	  po.append(pi);
	}
      else
	{
	  pi = po1[i];
	  po.append(pi);
	}
    }

  return po;
}

void
GiLightObjectInfo::save(fstream &fout)
{
  char keyword[100];
  float f[3];

  memset(keyword, 0, 100);
  sprintf(keyword, "gloinfo");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "points");
  fout.write((char*)keyword, strlen(keyword)+1);
  int npts = points.count();
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<npts; i++)
    {
      f[0] = points[i].x;
      f[1] = points[i].y;
      f[2] = points[i].z;
      fout.write((char*)&f, 3*sizeof(float));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "allowinterpolation");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&allowInterpolation, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "doshadows");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&doShadows, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "show");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&show, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "lighttype");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&lightType, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "rad");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&rad, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "decay");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&decay, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "angle");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&angle, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "color");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = color.x;
  f[1] = color.y;
  f[2] = color.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "opacity");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&opacity, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "segments");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&segments, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "lod");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&lod, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "smooth");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&smooth, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "end");
  fout.write((char*)keyword, strlen(keyword)+1);
}

void
GiLightObjectInfo::load(fstream &fin)
{
  bool done = false;
  char keyword[100];
  float f[3];

  lod = 1;
  smooth = 1;
  points.clear();

  while(!done)
    { 
      fin.getline(keyword, 100, 0);
      if (strcmp(keyword, "end") == 0)
	done = true;
      else if (strcmp(keyword, "points") == 0)
	{
	  int npts = 0;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {
	      fin.read((char*)&f, 3*sizeof(float));
	      points << Vec(f[0], f[1], f[2]);
	    }
	}
      else if (strcmp(keyword, "allowinterpolation") == 0)
	fin.read((char*)&allowInterpolation, sizeof(bool));
      else if (strcmp(keyword, "doshadows") == 0)
	fin.read((char*)&doShadows, sizeof(bool));
      else if (strcmp(keyword, "show") == 0)
	fin.read((char*)&show, sizeof(bool));
      else if (strcmp(keyword, "lighttype") == 0)
	fin.read((char*)&lightType, sizeof(int));
      else if (strcmp(keyword, "rad") == 0)
	fin.read((char*)&rad, sizeof(float));
      else if (strcmp(keyword, "decay") == 0)
	fin.read((char*)&decay, sizeof(float));
      else if (strcmp(keyword, "angle") == 0)
	fin.read((char*)&angle, sizeof(float));
      else if (strcmp(keyword, "color") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  color = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "opacity") == 0)
	fin.read((char*)&opacity, sizeof(float));
      else if (strcmp(keyword, "segments") == 0)
	fin.read((char*)&segments, sizeof(int));
      else if (strcmp(keyword, "lod") == 0)
	fin.read((char*)&lod, sizeof(int));
      else if (strcmp(keyword, "smooth") == 0)
	fin.read((char*)&smooth, sizeof(int));
    }
}
