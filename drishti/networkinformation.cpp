#include "networkinformation.h"

NetworkInformation::NetworkInformation() { clear(); }

void
NetworkInformation::clear()
{
  filename.clear();
  Vopacity = 1;
  Eopacity = 1;
  Vatt = 0;
  Eatt = 0;
  userVmin = userVmax = 0;
  userEmin = userEmax = 0;
  Vstops.clear();
  Estops.clear();
  scalee = scalev = 1.0;
}

NetworkInformation&
NetworkInformation::operator=(const NetworkInformation& ti)
{
  filename = ti.filename;
  Vopacity = ti.Vopacity;
  Eopacity = ti.Eopacity;
  Vatt = ti.Vatt;
  Eatt = ti.Eatt;
  userVmin = ti.userVmin;
  userVmax = ti.userVmax;
  userEmin = ti.userEmin;
  userEmax = ti.userEmax;
  Vstops = ti.Vstops;
  Estops = ti.Estops;
  scalee = ti.scalee;
  scalev = ti.scalev;
  return *this;
}

NetworkInformation
NetworkInformation::interpolate(const NetworkInformation tinfo1,
				const NetworkInformation tinfo2,
				float frc)
{
  NetworkInformation tinfo;
  tinfo.filename = tinfo1.filename;

  tinfo.Vopacity = (1-frc)*tinfo1.Vopacity + frc*tinfo2.Vopacity;
  tinfo.Eopacity = (1-frc)*tinfo1.Eopacity + frc*tinfo2.Eopacity;

  tinfo.scalee = (1-frc)*tinfo1.scalee + frc*tinfo2.scalee;
  tinfo.scalev = (1-frc)*tinfo1.scalev + frc*tinfo2.scalev;

  if (frc < 0.5)
    {
      tinfo.Vstops = tinfo1.Vstops;
      tinfo.Estops = tinfo1.Estops;
    }
  else
    {
      tinfo.Vstops = tinfo2.Vstops;
      tinfo.Estops = tinfo2.Estops;
    }

  if (tinfo1.Vatt == tinfo2.Vatt)
    {
      tinfo.Vatt = tinfo1.Vatt;
      tinfo.userVmin = (1-frc)*tinfo1.userVmin + frc*tinfo2.userVmin;
      tinfo.userVmax = (1-frc)*tinfo1.userVmax + frc*tinfo2.userVmax;
    }
  else
    {
      // modulate opacity & scale near 0.5 if the attributes are changing
      float opf = qAbs(frc - 0.5f);
      opf = qMin(opf*10, 1.0f);
      tinfo.Vopacity *= opf;
      tinfo.scalev *= opf;

      if (frc < 0.5)
	{
	  tinfo.Vatt = tinfo1.Vatt;
	  tinfo.userVmin = tinfo1.userVmin;
	  tinfo.userVmax = tinfo1.userVmax;
	}
      else
	{
	  tinfo.Vatt = tinfo2.Vatt;
	  tinfo.userVmin = tinfo2.userVmin;
	  tinfo.userVmax = tinfo2.userVmax;
	}
    }

  if (tinfo1.Eatt == tinfo2.Eatt)
    {
      tinfo.Eatt = tinfo1.Eatt;
      tinfo.userEmin = (1-frc)*tinfo1.userEmin + frc*tinfo2.userEmin;
      tinfo.userEmax = (1-frc)*tinfo1.userEmax + frc*tinfo2.userEmax;
    }
  else
    {
      // modulate opacity & scale near 0.5 if the attributes are changing
      float opf = qAbs(frc - 0.5f);
      opf = qMin(opf*10, 1.0f);
      tinfo.Eopacity *= opf;
      tinfo.scalee *= opf;

      if (frc < 0.5)
	{
	  tinfo.Eatt = tinfo1.Eatt;
	  tinfo.userEmin = tinfo1.userEmin;
	  tinfo.userEmax = tinfo1.userEmax;
	}
      else
	{
	  tinfo.Eatt = tinfo2.Eatt;
	  tinfo.userEmin = tinfo2.userEmin;
	  tinfo.userEmax = tinfo2.userEmax;
	}
    }



  return tinfo;
}

QList<NetworkInformation>
NetworkInformation::interpolate(const QList<NetworkInformation> tinfo1,
				const QList<NetworkInformation> tinfo2,
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

  QList<NetworkInformation> tinfo;
  for(int i=0; i<tinfo1.count(); i++)
    {
      if (present[i] >= 0)
	{
	  NetworkInformation ti;
	  ti = interpolate(tinfo1[i], tinfo2[present[i]], frc);
	  tinfo.append(ti);
	}
      else
	tinfo.append(tinfo1[i]);
    }

  return tinfo;
}

void
NetworkInformation::save(fstream &fout)
{
  char keyword[100];

  memset(keyword, 0, 100);
  sprintf(keyword, "networkinformation");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "filename");
  fout.write((char*)keyword, strlen(keyword)+1);
  int len = filename.size()+1;
  fout.write((char*)&len, sizeof(int));
  if (len > 0)
    fout.write((char*)filename.toLatin1().data(), len*sizeof(char));

  memset(keyword, 0, 100);
  sprintf(keyword, "vopacity");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&Vopacity, sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "eopacity");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&Eopacity, sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "scalee");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&scalee, sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "scalev");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&scalev, sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "vstops");
  fout.write((char*)keyword, strlen(keyword)+1);
  int nstops = Vstops.count();
  fout.write((char*)&nstops, sizeof(int));
  for(int s=0; s<nstops; s++)
    {
      float pos = Vstops[s].first;
      QColor color = Vstops[s].second;
      int r = color.red();
      int g = color.green();
      int b = color.blue();
      int a = color.alpha();
      fout.write((char*)&pos, sizeof(float));
      fout.write((char*)&r, sizeof(int));
      fout.write((char*)&g, sizeof(int));
      fout.write((char*)&b, sizeof(int));
      fout.write((char*)&a, sizeof(int));
    }

  memset(keyword, 0, 100);
  sprintf(keyword, "estops");
  fout.write((char*)keyword, strlen(keyword)+1);
  nstops = Estops.count();
  fout.write((char*)&nstops, sizeof(int));
  for(int s=0; s<nstops; s++)
    {
      float pos = Estops[s].first;
      QColor color = Estops[s].second;
      int r = color.red();
      int g = color.green();
      int b = color.blue();
      int a = color.alpha();
      fout.write((char*)&pos, sizeof(float));
      fout.write((char*)&r, sizeof(int));
      fout.write((char*)&g, sizeof(int));
      fout.write((char*)&b, sizeof(int));
      fout.write((char*)&a, sizeof(int));
    }


  memset(keyword, 0, 100);
  sprintf(keyword, "vatt");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&Vatt, sizeof(int));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "uservmin");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&userVmin, sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "uservmax");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&userVmax, sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "eatt");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&Eatt, sizeof(int));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "useremin");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&userEmin, sizeof(float));
  
  memset(keyword, 0, 100);
  sprintf(keyword, "useremax");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&userEmax, sizeof(float));
  

  memset(keyword, 0, 100);
  sprintf(keyword, "end");
  fout.write((char*)keyword, strlen(keyword)+1);
}

void
NetworkInformation::load(fstream &fin)
{
  clear();

  bool done = false;
  char keyword[100];
  

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
      else if (strcmp(keyword, "vopacity") == 0)
	fin.read((char*)&Vopacity, sizeof(float));
      else if (strcmp(keyword, "eopacity") == 0)
	fin.read((char*)&Eopacity, sizeof(float));
      else if (strcmp(keyword, "scalee") == 0)
	fin.read((char*)&scalee, sizeof(float));
      else if (strcmp(keyword, "scalev") == 0)
	fin.read((char*)&scalev, sizeof(float));
      else if (strcmp(keyword, "vstops") == 0)
	{
	  int nstops;
	  fin.read((char*)&nstops, sizeof(int));
	  for(int s=0; s<nstops; s++)
	    {
	      float pos;
	      int r,g,b,a;
	      fin.read((char*)&pos, sizeof(float));
	      fin.read((char*)&r, sizeof(int));
	      fin.read((char*)&g, sizeof(int));
	      fin.read((char*)&b, sizeof(int));
	      fin.read((char*)&a, sizeof(int));
	      Vstops << QGradientStop(pos, QColor(r,g,b,a));
	    }
	}
      else if (strcmp(keyword, "estops") == 0)
	{
	  int nstops;
	  fin.read((char*)&nstops, sizeof(int));
	  for(int s=0; s<nstops; s++)
	    {
	      float pos;
	      int r,g,b,a;
	      fin.read((char*)&pos, sizeof(float));
	      fin.read((char*)&r, sizeof(int));
	      fin.read((char*)&g, sizeof(int));
	      fin.read((char*)&b, sizeof(int));
	      fin.read((char*)&a, sizeof(int));
	      Estops << QGradientStop(pos, QColor(r,g,b,a));
	    }
	}
      else if (strcmp(keyword, "vatt") == 0)
	fin.read((char*)&Vatt, sizeof(int));
      else if (strcmp(keyword, "uservmin") == 0)
	fin.read((char*)&userVmin, sizeof(float));
      else if (strcmp(keyword, "uservmax") == 0)
	fin.read((char*)&userVmax, sizeof(float));
      else if (strcmp(keyword, "eatt") == 0)
	fin.read((char*)&Eatt, sizeof(int));
      else if (strcmp(keyword, "useremin") == 0)
	fin.read((char*)&userEmin, sizeof(float));
      else if (strcmp(keyword, "useremax") == 0)
	fin.read((char*)&userEmax, sizeof(float));
    }
}
