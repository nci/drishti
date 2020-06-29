#include "global.h"
#include "trisetinformation.h"
#include <QMessageBox>
#include <QDir>

TrisetInformation::TrisetInformation() { clear(); }

void
TrisetInformation::clear()
{
  show = true;
  clip = true;
  filename.clear();
  position = Vec(0,0,0);
  scale = Vec(1,1,1);
  q = Quaternion();
  color = Vec(1,1,1);
  cropcolor = Vec(0,0,0);
  roughness = 0.3;
  ambient = 0;
  diffuse = 1;
  specular = 1;
  reveal = 0.0;
  outline = 0.0;
  glow = 0.0;
  dark = 0.5;
  captionText = "";
  captionColor = Qt::white;
  captionFont = QFont("Helvetica");
  captionFont.setPointSize(16);
  captionPosition = Vec(0,0,0);
  cpDx = 0;
  cpDy = -100;
  pattern = Vec(0,10,0.5);
  opacity = 0.7;
}

TrisetInformation&
TrisetInformation::operator=(const TrisetInformation& ti)
{
  show = ti.show;
  clip = ti.clip;
  filename = ti.filename;
  position = ti.position;
  scale = ti.scale;
  q = ti.q;
  color = ti.color;
  cropcolor = ti.cropcolor;
  roughness = ti.roughness;
  ambient = ti.ambient;
  diffuse = ti.diffuse;
  specular = ti.specular;
  reveal = ti.reveal;
  outline = ti.outline;
  glow = ti.glow;
  dark = ti.dark;
  captionText = ti.captionText;
  captionColor = ti.captionColor;
  captionFont = ti.captionFont;
  captionPosition = ti.captionPosition;
  cpDx = ti.cpDx;
  cpDy = ti.cpDy;
  pattern = ti.pattern;
  opacity = ti.opacity;
  
  return *this;
}

TrisetInformation
TrisetInformation::interpolate(const TrisetInformation tinfo1,
			       const TrisetInformation tinfo2,
			       float frc)
{
  TrisetInformation tinfo;
  tinfo.show = tinfo1.show;
  tinfo.clip = tinfo1.clip;
  tinfo.filename = tinfo1.filename;
  tinfo.roughness = (1-frc)*tinfo1.roughness + frc*tinfo2.roughness;
  tinfo.position = (1-frc)*tinfo1.position + frc*tinfo2.position;
  tinfo.scale = (1-frc)*tinfo1.scale + frc*tinfo2.scale;
  tinfo.q = Quaternion::slerp(tinfo1.q, tinfo2.q, frc);
  tinfo.color = (1-frc)*tinfo1.color + frc*tinfo2.color;
  tinfo.cropcolor = (1-frc)*tinfo1.cropcolor + frc*tinfo2.cropcolor;
  tinfo.ambient = (1-frc)*tinfo1.ambient + frc*tinfo2.ambient;
  tinfo.diffuse = (1-frc)*tinfo1.diffuse + frc*tinfo2.diffuse;
  tinfo.specular = (1-frc)*tinfo1.specular + frc*tinfo2.specular;
  tinfo.reveal = (1-frc)*tinfo1.reveal + frc*tinfo2.reveal;
  tinfo.outline = (1-frc)*tinfo1.outline + frc*tinfo2.outline;
  tinfo.glow = (1-frc)*tinfo1.glow + frc*tinfo2.glow;
  tinfo.dark = (1-frc)*tinfo1.dark + frc*tinfo2.dark;
  tinfo.captionText = tinfo1.captionText;
  tinfo.captionColor = tinfo1.captionColor;
  tinfo.captionFont = tinfo1.captionFont;
  tinfo.captionPosition = (1-frc)*tinfo1.captionPosition + frc*tinfo2.captionPosition;
  tinfo.cpDx = (1-frc)*tinfo1.cpDx + frc*tinfo2.cpDx;
  tinfo.cpDy = (1-frc)*tinfo1.cpDy + frc*tinfo2.cpDy;
  tinfo.pattern = (1-frc)*tinfo1.pattern + frc*tinfo2.pattern;
  tinfo.opacity = (1-frc)*tinfo1.opacity + frc*tinfo2.opacity;
  
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


  QDir direc(Global::previousDirectory());
  QString relFile = direc.relativeFilePath(filename);
  memset(keyword, 0, 100);
  sprintf(keyword, "filename");
  fout.write((char*)keyword, strlen(keyword)+1);
  int len = relFile.size()+1;
  fout.write((char*)&len, sizeof(int));
  if (len > 0)
    fout.write((char*)relFile.toLatin1().data(), len*sizeof(char));
    
//  memset(keyword, 0, 100);
//  sprintf(keyword, "filename");
//  fout.write((char*)keyword, strlen(keyword)+1);
//  int len = filename.size()+1;
//  fout.write((char*)&len, sizeof(int));
//  if (len > 0)
//    fout.write((char*)filename.toLatin1().data(), len*sizeof(char));


  
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
  sprintf(keyword, "roughness");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&roughness, sizeof(float));
  
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
  sprintf(keyword, "show");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&show, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "clip");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&clip, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "rotation");
  fout.write((char*)keyword, strlen(keyword)+1);
  Vec axis;
  qreal ang;
  q.getAxisAngle(axis, ang);
  f[0] = axis.x;
  f[1] = axis.y;
  f[2] = axis.z;
  fout.write((char*)&f, 3*sizeof(float));
  float angle = ang;
  fout.write((char*)&angle, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "reveal");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&reveal, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "outline");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&outline, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "glow");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&glow, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "dark");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&dark, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "text");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = captionText.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)captionText.toLatin1().data(), len*sizeof(char));

  QString fontStr = captionFont.toString();
  memset(keyword, 0, 100);
  sprintf(keyword, "font");
  fout.write((char*)keyword, strlen(keyword)+1);
  len = fontStr.size()+1;
  fout.write((char*)&len, sizeof(int));
  fout.write((char*)fontStr.toLatin1().data(), len*sizeof(char));

  unsigned char r = captionColor.red();
  unsigned char g = captionColor.green();
  unsigned char b = captionColor.blue();
  memset(keyword, 0, 100);
  sprintf(keyword, "captioncolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&r, sizeof(unsigned char));
  fout.write((char*)&g, sizeof(unsigned char));
  fout.write((char*)&b, sizeof(unsigned char));

  memset(keyword, 0, 100);
  sprintf(keyword, "captionpos");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = captionPosition.x;
  f[1] = captionPosition.y;
  f[2] = captionPosition.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "captionoffset");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&cpDx, sizeof(int));
  fout.write((char*)&cpDy, sizeof(int));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "pattern");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = pattern.x;
  f[1] = pattern.y;
  f[2] = pattern.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "opacity");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&opacity, sizeof(float));

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
  int len;

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
	      //filename = QString(str);
	      QDir direc(Global::previousDirectory());
	      filename = direc.absoluteFilePath(QString(str));
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
      else if (strcmp(keyword, "roughness") == 0)
	fin.read((char*)&roughness, sizeof(float));
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
      else if (strcmp(keyword, "reveal") == 0)
	fin.read((char*)&reveal, sizeof(float));
      else if (strcmp(keyword, "outline") == 0)
	fin.read((char*)&outline, sizeof(float));
      else if (strcmp(keyword, "glow") == 0)
	fin.read((char*)&glow, sizeof(float));
      else if (strcmp(keyword, "dark") == 0)
	fin.read((char*)&dark, sizeof(float));
      else if (strcmp(keyword, "show") == 0)
	fin.read((char*)&show, sizeof(bool));
      else if (strcmp(keyword, "clip") == 0)
	fin.read((char*)&clip, sizeof(bool));
      else if (strcmp(keyword, "rotation") == 0)
	{
	  Vec axis;
	  float angle;
	  fin.read((char*)&f, 3*sizeof(float));	      
	  axis = Vec(f[0],f[1],f[2]);
	  fin.read((char*)&angle, sizeof(float));
	  q = Quaternion(axis, angle);
	}
      else if (strcmp(keyword, "text") == 0)
	{
	  fin.read((char*)&len, sizeof(int));
	  char *str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  captionText = QString(str);
	  delete [] str;
	}
      else if (strcmp(keyword, "font") == 0)
	{
	  fin.read((char*)&len, sizeof(int));
	  char *str = new char[len];
	  fin.read((char*)str, len*sizeof(char));
	  QString fontStr = QString(str);
	  captionFont.fromString(fontStr); 
	  delete [] str;
	}
      else if (strcmp(keyword, "captioncolor") == 0)
	{
	  unsigned char r, g, b;
	  fin.read((char*)&r, sizeof(unsigned char));
	  fin.read((char*)&g, sizeof(unsigned char));
	  fin.read((char*)&b, sizeof(unsigned char));
	  captionColor = QColor(r,g,b);
	}
      else if (strcmp(keyword, "captionpos") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  captionPosition = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "captionoffset") == 0)
	{
	  fin.read((char*)&cpDx, sizeof(float));
	  fin.read((char*)&cpDy, sizeof(float));
	}
      else if (strcmp(keyword, "pattern") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  pattern = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "opacity") == 0)
	fin.read((char*)&opacity, sizeof(float));
    }
}
