#include "landmarkinformation.h"

#include <QMessageBox>

LandmarkInformation::LandmarkInformation()
{
  clear();
}

LandmarkInformation::~LandmarkInformation()
{
  clear();
}

void
LandmarkInformation::clear()
{
  points.clear();
  text.clear();

  showCoordinates = false;
  showText = true;
  showNumber = true;

  pointColor = Vec(0.5,1.0,0.5);
  pointSize = 10;

  textColor = Vec(1.0,1.0,1.0);
  textSize = 10;

  distance.clear();
  projectline.clear();
  projectplane.clear();
}

LandmarkInformation&
LandmarkInformation::operator=(const LandmarkInformation& li)
{
  textColor = li.textColor;
  textSize = li.textSize;
  pointColor = li.pointColor;
  pointSize = li.pointSize;

  points = li.points;
  text = li.text;

  showCoordinates = li.showCoordinates;
  showText = li.showText;
  showNumber = li.showNumber;
  
  distance = li.distance;
  projectline = li.projectline;
  projectplane = li.projectplane;

  return *this;
}

void
LandmarkInformation::save(fstream& fout)
{
  char keyword[100];
  int len;
  float f[3];

  memset(keyword, 0, 100);
  sprintf(keyword, "landmarks");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "showcoord");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&showCoordinates, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "showtext");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&showText, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "shownumber");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&showNumber, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "textcolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = textColor.x;
  f[1] = textColor.y;
  f[2] = textColor.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "textsize");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&textSize, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "pointcolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = pointColor.x;
  f[1] = pointColor.y;
  f[2] = pointColor.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "pointsize");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&pointSize, sizeof(int));

  int npts = points.count();
  memset(keyword, 0, 100);
  sprintf(keyword, "points");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<npts; i++)
    {
      f[0] = points[i].x;
      f[1] = points[i].y;
      f[2] = points[i].z;
      fout.write((char*)&f, 3*sizeof(float));
    }

  npts = text.count();
  memset(keyword, 0, 100);
  sprintf(keyword, "pointtext");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&npts, sizeof(int));
  for(int i=0; i<npts; i++)
    fout.write((char*)text[i].toLatin1().data(), text[i].length()+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "distancetext");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)distance.toLatin1().data(), distance.length()+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "projectlinetext");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)projectline.toLatin1().data(), projectline.length()+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "projectplanetext");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)projectplane.toLatin1().data(), projectplane.length()+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "landmarksend");
  fout.write((char*)keyword, strlen(keyword)+1);
}

void
LandmarkInformation::load(fstream& fin)
{
  clear();

  bool done = false;
  char keyword[1000];
  float f[5];
  while (!done)
    {
      fin.getline(keyword, 100, 0);

      if (strcmp(keyword, "landmarksend") == 0)
	done = true;
      else if (strcmp(keyword, "showcoord") == 0)
	fin.read((char*)&showCoordinates, sizeof(bool));
      else if (strcmp(keyword, "showtext") == 0)
	fin.read((char*)&showText, sizeof(bool));
      else if (strcmp(keyword, "shownumber") == 0)
	fin.read((char*)&showNumber, sizeof(bool));
      else if (strcmp(keyword, "textcolor") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  textColor = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "textsize") == 0)
	fin.read((char*)&textSize, sizeof(int));
      else if (strcmp(keyword, "pointcolor") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  pointColor = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "pointsize") == 0)
	fin.read((char*)&pointSize, sizeof(int));
      else if (strcmp(keyword, "points") == 0)
	{
	  int npts;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {
	      fin.read((char*)&f, 3*sizeof(float));
	      points.append(Vec(f[0], f[1], f[2]));
	    }
	}
      else if (strcmp(keyword, "pointtext") == 0)
	{
	  int npts;
	  fin.read((char*)&npts, sizeof(int));
	  for(int i=0; i<npts; i++)
	    {	      
	      fin.getline(keyword, 100, 0);
	      text.append(keyword);
	    }
	}
      else if (strcmp(keyword, "distancetext") == 0)
	{
	  fin.getline(keyword, 100, 0);	  
	  distance = QString(keyword);
	}
      else if (strcmp(keyword, "projectlinetext") == 0)
	{
	  fin.getline(keyword, 100, 0);
	  projectline = QString(keyword);
	}
      else if (strcmp(keyword, "projectplanetext") == 0)
	{
	  fin.getline(keyword, 100, 0);
	  projectplane = QString(keyword);
	}
    }

}
