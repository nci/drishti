#include "trisetinformation.h"

TrisetInformation::TrisetInformation() { clear(); }

void
TrisetInformation::clear()
{
  filename.clear();
  position = Vec(0,0,0);
  scale = Vec(1,1,1);
  color = Vec(1,1,1);
  cropcolor = Vec(0,0,0);
  opacity = 1;
  ambient = 0;
  diffuse = 1;
  specular = 1;
  pointMode = true;
  pointSize = 10;
  pointStep = 1;
  blendMode = false;
  shadows = false;
  screenDoor = false;
  flipNormals = false;
}

TrisetInformation&
TrisetInformation::operator=(const TrisetInformation& ti)
{
  filename = ti.filename;
  position = ti.position;
  scale = ti.scale;
  color = ti.color;
  cropcolor = ti.cropcolor;
  opacity = ti.opacity;
  ambient = ti.ambient;
  diffuse = ti.diffuse;
  specular = ti.specular;
  pointMode = ti.pointMode;
  pointStep = ti.pointStep;
  pointSize = ti.pointSize;
  blendMode = ti.blendMode;
  shadows = ti.shadows;
  screenDoor = ti.screenDoor;
  flipNormals = ti.flipNormals;

  return *this;
}

TrisetInformation
TrisetInformation::interpolate(const TrisetInformation tinfo1,
			       const TrisetInformation tinfo2,
			       float frc)
{
  TrisetInformation tinfo;
  tinfo.filename = tinfo1.filename;
  tinfo.opacity = (1-frc)*tinfo1.opacity + frc*tinfo2.opacity;
  tinfo.position = (1-frc)*tinfo1.position + frc*tinfo2.position;
  tinfo.scale = (1-frc)*tinfo1.scale + frc*tinfo2.scale;
  tinfo.color = (1-frc)*tinfo1.color + frc*tinfo2.color;
  tinfo.cropcolor = (1-frc)*tinfo1.cropcolor + frc*tinfo2.cropcolor;
  tinfo.ambient = (1-frc)*tinfo1.ambient + frc*tinfo2.ambient;
  tinfo.diffuse = (1-frc)*tinfo1.diffuse + frc*tinfo2.diffuse;
  tinfo.specular = (1-frc)*tinfo1.specular + frc*tinfo2.specular;
  tinfo.pointMode = tinfo1.pointMode;
  tinfo.pointStep = (1-frc)*tinfo1.pointStep + frc*tinfo2.pointStep;
  tinfo.pointSize = (1-frc)*tinfo1.pointSize + frc*tinfo2.pointSize;
  tinfo.blendMode = tinfo1.blendMode;
  tinfo.shadows = tinfo1.shadows;
  tinfo.screenDoor = tinfo1.screenDoor;
  tinfo.flipNormals = tinfo1.flipNormals;

  return tinfo;
}

QList<TrisetInformation>
TrisetInformation::interpolate(const QList<TrisetInformation> tinfo1,
			       const QList<TrisetInformation> tinfo2,
			       float frc)
{
  QVector<int> present;
  present.resize(tinfo1.count());
  for(int i=0; i<tinfo1.count(); i++)
    {
      present[i] = -1;
      for(int j=0; j<tinfo2.count(); j++)
	{
	  if (tinfo1[i].filename == tinfo2[j].filename)
	    {
	      present[i] = j;
	      break;
	    }
	}
    }

  QList<TrisetInformation> tinfo;
  for(int i=0; i<tinfo1.count(); i++)
    {
      if (present[i] >= 0)
	{
	  TrisetInformation ti;
	  ti = interpolate(tinfo1[i], tinfo2[present[i]], frc);
	  tinfo.append(ti);
	}
      else
	tinfo.append(tinfo1[i]);
    }

  return tinfo;
}

void
TrisetInformation::save(fstream &fout)
{
  char keyword[100];
  float f[3];

  memset(keyword, 0, 100);
  sprintf(keyword, "trisetinformation");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "filename");
  fout.write((char*)keyword, strlen(keyword)+1);
  int len = filename.size()+1;
  fout.write((char*)&len, sizeof(int));
  if (len > 0)
    fout.write((char*)filename.toLatin1().data(), len*sizeof(char));

  memset(keyword, 0, 100);
  sprintf(keyword, "position");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = position.x;
  f[1] = position.y;
  f[2] = position.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "scale");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = scale.x;
  f[1] = scale.y;
  f[2] = scale.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "opacity");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&opacity, sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "color");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = color.x;
  f[1] = color.y;
  f[2] = color.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "cropcolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = cropcolor.x;
  f[1] = cropcolor.y;
  f[2] = cropcolor.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "ambient");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&ambient, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "diffuse");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&diffuse, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "specular");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&specular, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "pointmode");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&pointMode, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "pointstep");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&pointStep, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "pointsize");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&pointSize, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "blendmode");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&blendMode, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "shadows");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&shadows, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "screendoor");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&screenDoor, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "flipnormals");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&flipNormals, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "end");
  fout.write((char*)keyword, strlen(keyword)+1);
}

void
TrisetInformation::load(fstream &fin)
{
  clear();

  bool done = false;
  char keyword[100];
  float f[3];

  while(!done)
    { 
      fin.getline(keyword, 100, 0);
      if (strcmp(keyword, "end") == 0)
	done = true;
      else if (strcmp(keyword, "filename") == 0)
	{
	  int len;
	  fin.read((char*)&len, sizeof(int));
	  if (len > 0)
	    {
	      char *str = new char[len];
	      fin.read((char*)str, len*sizeof(char));
	      filename = QString(str);
	      delete [] str;
	    }
	}
      else if (strcmp(keyword, "position") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  position = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "scale") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  scale = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "opacity") == 0)
	fin.read((char*)&opacity, sizeof(float));
      else if (strcmp(keyword, "color") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  color = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "cropcolor") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  cropcolor = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "ambient") == 0)
	fin.read((char*)&ambient, sizeof(float));
      else if (strcmp(keyword, "diffuse") == 0)
	fin.read((char*)&diffuse, sizeof(float));
      else if (strcmp(keyword, "specular") == 0)
	fin.read((char*)&specular, sizeof(float));
      else if (strcmp(keyword, "pointmode") == 0)
	fin.read((char*)&pointMode, sizeof(bool));
      else if (strcmp(keyword, "pointstep") == 0)
	fin.read((char*)&pointStep, sizeof(int));
      else if (strcmp(keyword, "pointsize") == 0)
	fin.read((char*)&pointSize, sizeof(int));
      else if (strcmp(keyword, "blendmode") == 0)
	fin.read((char*)&blendMode, sizeof(bool));
      else if (strcmp(keyword, "shadows") == 0)
	fin.read((char*)&shadows, sizeof(bool));
      else if (strcmp(keyword, "screendoor") == 0)
	fin.read((char*)&screenDoor, sizeof(bool));
      else if (strcmp(keyword, "flipnormals") == 0)
	fin.read((char*)&flipNormals, sizeof(bool));
    }
}
