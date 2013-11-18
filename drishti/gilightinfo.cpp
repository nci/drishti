#include "gilightinfo.h"

GiLightInfo::GiLightInfo() { clear(); }

GiLightInfo::~GiLightInfo() { clear(); }

void
GiLightInfo::clear()
{
  gloInfo.clear();

  basicLight = false;
  applyClip = false;
  applyCrop = false;
  onlyAOLight = false;
  lightLod = 2;
  lightDiffuse = 1;

  aoLightColor = Vec(1,1,1);
  aoRad = 2;
  aoTimes = 5;
  aoFrac = 0.7;
  aoDensity1 = 0.3;
  aoDensity2 = 1.0;
  
  emisTF = -1;
  emisDecay = 1.0;
  emisBoost = 1;
  emisTimes = 1;
}

GiLightInfo&
GiLightInfo::operator=(const GiLightInfo& gi)
{
  gloInfo = gi.gloInfo;
  
  basicLight = gi.basicLight;
  applyClip = gi.applyClip;
  applyCrop = gi.applyCrop;
  onlyAOLight = gi.onlyAOLight;
  lightLod = gi.lightLod;
  lightDiffuse = gi.lightDiffuse;

  aoLightColor = gi.aoLightColor;
  aoRad = gi.aoRad;
  aoTimes = gi.aoTimes;
  aoFrac = gi.aoFrac;
  aoDensity1 = gi.aoDensity1;
  aoDensity2 = gi.aoDensity2;
  
  emisTF = gi.emisTF;
  emisDecay = gi.emisDecay;
  emisBoost = gi.emisBoost;
  emisTimes = gi.emisTimes;

  return *this;
}

GiLightInfo
GiLightInfo::interpolate(const GiLightInfo gi1,
			 const GiLightInfo gi2,
			 float frc)
{
  GiLightInfo gi;

  gi.gloInfo = GiLightObjectInfo::interpolate(gi1.gloInfo,
					      gi2.gloInfo,
					      frc);

  gi.basicLight = gi1.basicLight;
  gi.applyClip = gi1.applyClip;
  gi.applyCrop = gi1.applyCrop;
  gi.onlyAOLight = gi1.onlyAOLight;

  gi.lightLod = (1.0-frc)*gi1.lightLod + frc*gi2.lightLod;
  gi.lightDiffuse = (1.0-frc)*gi1.lightDiffuse + frc*gi2.lightDiffuse;

  gi.aoLightColor = (1.0-frc)*gi1.aoLightColor + frc*gi2.aoLightColor;
  gi.aoRad = (1.0-frc)*gi1.aoRad + frc*gi2.aoRad;
  gi.aoTimes = (1.0-frc)*gi1.aoTimes + frc*gi2.aoTimes;
  gi.aoFrac = (1.0-frc)*gi1.aoFrac + frc*gi2.aoFrac;
  gi.aoDensity1 = (1.0-frc)*gi1.aoDensity1 + frc*gi2.aoDensity1;
  gi.aoDensity2 = (1.0-frc)*gi1.aoDensity2 + frc*gi2.aoDensity2;

  gi.emisTF = gi1.emisTF;
  gi.emisDecay = (1.0-frc)*gi1.emisDecay + frc*gi2.emisDecay;
  gi.emisBoost = (1.0-frc)*gi1.emisBoost + frc*gi2.emisBoost;
  gi.emisTimes = (1.0-frc)*gi1.emisTimes + frc*gi2.emisTimes;

  return gi;
}

void
GiLightInfo::save(fstream &fout)
{
  char keyword[100];
  float f[3];

  memset(keyword, 0, 100);
  sprintf(keyword, "gilightinfo");
  fout.write((char*)keyword, strlen(keyword)+1);

  for(int i=0; i<gloInfo.count(); i++)
    gloInfo[i].save(fout);

  memset(keyword, 0, 100);
  sprintf(keyword, "basiclight");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&basicLight, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "applyclip");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&applyClip, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "applycrop");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&applyCrop, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "onlyaolight");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&onlyAOLight, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "lightlod");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&lightLod, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "lightdiffuse");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&lightDiffuse, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "aolightcolor");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = aoLightColor.x;
  f[1] = aoLightColor.y;
  f[2] = aoLightColor.z;
  fout.write((char*)&f, 3*sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "aorad");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&aoRad, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "aotimes");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&aoTimes, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "aofrac");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&aoFrac, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "aodensity1");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&aoDensity1, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "aodensity2");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&aoDensity2, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "emistf");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&emisTF, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "emisdecay");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&emisDecay, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "emisboost");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&emisBoost, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "emistimes");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&emisTimes, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "end");
  fout.write((char*)keyword, strlen(keyword)+1);
}

void
GiLightInfo::load(fstream &fin)
{
  bool done = false;
  char keyword[100];
  float f[3];
  
  gloInfo.clear();

  while(!done)
    { 
      fin.getline(keyword, 100, 0);
      if (strcmp(keyword, "end") == 0)
	done = true;
      else if (strcmp(keyword, "gloinfo") == 0)
	{
	  GiLightObjectInfo glo;
	  glo.load(fin);
	  gloInfo << glo;
	}
      else if (strcmp(keyword, "basiclight") == 0)
	fin.read((char*)&basicLight, sizeof(bool));
      else if (strcmp(keyword, "applyclip") == 0)
	fin.read((char*)&applyClip, sizeof(bool));
      else if (strcmp(keyword, "applycrop") == 0)
	fin.read((char*)&applyCrop, sizeof(bool));
      else if (strcmp(keyword, "onlyaolight") == 0)
	fin.read((char*)&onlyAOLight, sizeof(bool));
      else if (strcmp(keyword, "lightlod") == 0)
	fin.read((char*)&lightLod, sizeof(int));
      else if (strcmp(keyword, "lightdiffuse") == 0)
	fin.read((char*)&lightDiffuse, sizeof(int));
      else if (strcmp(keyword, "aolightcolor") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  aoLightColor = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "aorad") == 0)
	fin.read((char*)&aoRad, sizeof(int));
      else if (strcmp(keyword, "aotimes") == 0)
	fin.read((char*)&aoTimes, sizeof(int));
      else if (strcmp(keyword, "aofrac") == 0)
	fin.read((char*)&aoFrac, sizeof(float));
      else if (strcmp(keyword, "aodensity1") == 0)
	fin.read((char*)&aoDensity1, sizeof(float));
      else if (strcmp(keyword, "aodensity2") == 0)
	fin.read((char*)&aoDensity2, sizeof(float));
      else if (strcmp(keyword, "emistf") == 0)
	fin.read((char*)&emisTF, sizeof(int));
      else if (strcmp(keyword, "emisdecay") == 0)
	fin.read((char*)&emisDecay, sizeof(float));
      else if (strcmp(keyword, "emisboost") == 0)
	fin.read((char*)&emisBoost, sizeof(int));
      else if (strcmp(keyword, "emistimes") == 0)
	fin.read((char*)&emisTimes, sizeof(int));
    }
}
