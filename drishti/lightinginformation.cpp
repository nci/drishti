#include "lightinginformation.h"
#include "staticfunctions.h"

Highlights::Highlights()
{
  ambient = 1;
  diffuse = 0;
  specular = 1;
  specularCoefficient = 7;
}
Highlights::Highlights(const Highlights& hl)
{
  ambient = hl.ambient;
  diffuse = hl.diffuse;
  specular = hl.specular;
  specularCoefficient = hl.specularCoefficient;
}
Highlights&
Highlights::operator=(const Highlights& hl)
{
  ambient = hl.ambient;
  diffuse = hl.diffuse;
  specular = hl.specular;
  specularCoefficient = hl.specularCoefficient;
  return *this;
}
Highlights
Highlights::interpolate(const Highlights highlights1,
			const Highlights highlights2,
			float frc)
{
  Highlights highlights;

  highlights.ambient = highlights1.ambient + frc*(highlights2.ambient-highlights1.ambient);
  highlights.diffuse = highlights1.diffuse + frc*(highlights2.diffuse-highlights1.diffuse);
  highlights.specular = highlights1.specular + frc*(highlights2.specular-highlights1.specular);
  highlights.specularCoefficient = highlights1.specularCoefficient + frc*(highlights2.specularCoefficient-highlights1.specularCoefficient);

  return highlights;
}


LightingInformation::LightingInformation()
{
  applyEmissive = false;
  applyLighting = true;
  applyShadows = true;
  applyColoredShadows = false;
  applyBackplane = false;
  colorAttenuation = Vec(1,1,1);
  userLightVector = Vec(0,0,1);
  shadowBlur = 1.9f;
  shadowScale = 0.7f;
  shadowIntensity = 0.5f;
  shadowFovOffset = 0.0f;
  lightDistanceOffset = 0.0f;
  backplaneShadowScale = 1.0f;
  backplaneIntensity = 1.0f;

  peel = false;
  peelType = 0;
  peelMin = 0.3f;
  peelMax = -0.3f;
  peelMix = 0.0;
}

void
LightingInformation::clear()
{
  applyEmissive = false;
  applyLighting = true;
  applyShadows = true;
  applyColoredShadows = false;
  applyBackplane = false;
  colorAttenuation = Vec(1,1,1);
  userLightVector = Vec(0,0,1);
  shadowBlur = 1.9f;
  shadowScale = 0.7f;
  shadowIntensity = 0.5f;
  shadowFovOffset = 0.0f;
  lightDistanceOffset = 0.0f;
  backplaneShadowScale = 1.0f;
  backplaneIntensity = 1.0f;

  peel = false;
  peelType = 0;
  peelMin = -0.3f;
  peelMax = 0.3f;
  peelMix = 0.0;
}

LightingInformation::LightingInformation(const LightingInformation& li)
{
  applyEmissive = li.applyEmissive;
  applyLighting = li.applyLighting;
  applyShadows = li.applyShadows;
  applyColoredShadows = li.applyColoredShadows;
  applyBackplane = li.applyBackplane;
  colorAttenuation = li.colorAttenuation;
  userLightVector = li.userLightVector;
  shadowBlur = li.shadowBlur;
  shadowScale = li.shadowScale;
  shadowIntensity = li.shadowIntensity;
  shadowFovOffset = li.shadowFovOffset;
  lightDistanceOffset = li.lightDistanceOffset;
  backplaneShadowScale = li.backplaneShadowScale;
  backplaneIntensity = li.backplaneIntensity;
  highlights = li.highlights;

  peel = li.peel;
  peelType = li.peelType;
  peelMin = li.peelMin;
  peelMax = li.peelMax;
  peelMix = li.peelMix;
}
LightingInformation&
LightingInformation::operator=(const LightingInformation& li)
{
  applyEmissive = li.applyEmissive;
  applyLighting = li.applyLighting;
  applyShadows = li.applyShadows;
  applyColoredShadows = li.applyColoredShadows;
  applyBackplane = li.applyBackplane;
  colorAttenuation = li.colorAttenuation;
  userLightVector = li.userLightVector;
  shadowBlur = li.shadowBlur;
  shadowScale = li.shadowScale;
  shadowIntensity = li.shadowIntensity;
  shadowFovOffset = li.shadowFovOffset;
  lightDistanceOffset = li.lightDistanceOffset;
  backplaneShadowScale = li.backplaneShadowScale;
  backplaneIntensity = li.backplaneIntensity;
  highlights = li.highlights;

  peel = li.peel;
  peelType = li.peelType;
  peelMin = li.peelMin;
  peelMax = li.peelMax;
  peelMix = li.peelMix;

  return *this;
}

LightingInformation
LightingInformation::interpolate(const LightingInformation lightInfo1,
				 const LightingInformation lightInfo2,
				 float frc)
{
  LightingInformation lightInfo;

  if (frc <= 0.5)
    {
      lightInfo.applyEmissive = lightInfo1.applyEmissive;
      lightInfo.applyLighting = lightInfo1.applyLighting;
      lightInfo.applyShadows = lightInfo1.applyShadows;
      lightInfo.applyColoredShadows = lightInfo1.applyColoredShadows;
      lightInfo.applyBackplane = lightInfo1.applyBackplane;
      lightInfo.peel = lightInfo1.peel;
      lightInfo.peelType = lightInfo1.peelType;
    }
  else
    {
      lightInfo.applyEmissive = lightInfo2.applyEmissive;
      lightInfo.applyLighting = lightInfo2.applyLighting;
      lightInfo.applyShadows = lightInfo2.applyShadows;
      lightInfo.applyColoredShadows = lightInfo2.applyColoredShadows;
      lightInfo.applyBackplane = lightInfo2.applyBackplane;
      lightInfo.peel = lightInfo2.peel;
      lightInfo.peelType = lightInfo2.peelType;
    }

  lightInfo.colorAttenuation = lightInfo1.colorAttenuation + frc*(lightInfo2.colorAttenuation -
								  lightInfo1.colorAttenuation);

  lightInfo.shadowBlur = lightInfo1.shadowBlur + frc*(lightInfo2.shadowBlur -
						      lightInfo1.shadowBlur);

  lightInfo.shadowScale = lightInfo1.shadowScale + frc*(lightInfo2.shadowScale -
							lightInfo1.shadowScale);

  lightInfo.shadowIntensity = lightInfo1.shadowIntensity + frc*(lightInfo2.shadowIntensity -
								lightInfo1.shadowIntensity);

  lightInfo.shadowFovOffset = lightInfo1.shadowFovOffset + frc*(lightInfo2.shadowFovOffset -
								lightInfo1.shadowFovOffset);

  lightInfo.peelMin = lightInfo1.peelMin + frc*(lightInfo2.peelMin - lightInfo1.peelMin);
  lightInfo.peelMax = lightInfo1.peelMax + frc*(lightInfo2.peelMax - lightInfo1.peelMax);
  lightInfo.peelMix = lightInfo1.peelMix + frc*(lightInfo2.peelMix - lightInfo1.peelMix);

  if ((lightInfo1.userLightVector-
       lightInfo2.userLightVector).squaredNorm() > 0.001)
    {
      Vec axis;
      float angle;
      StaticFunctions::getRotationBetweenVectors(lightInfo1.userLightVector,
						 lightInfo2.userLightVector,
						 axis, angle);
      Quaternion q(axis, angle*frc);
      lightInfo.userLightVector = q.rotate(lightInfo1.userLightVector);
    }
  else
    lightInfo.userLightVector = lightInfo1.userLightVector;


  lightInfo.lightDistanceOffset = lightInfo1.lightDistanceOffset +
    frc*(lightInfo2.lightDistanceOffset - lightInfo1.lightDistanceOffset);

  lightInfo.backplaneShadowScale = lightInfo1.backplaneShadowScale +
    frc*(lightInfo2.backplaneShadowScale - lightInfo1.backplaneShadowScale);

  lightInfo.backplaneIntensity = lightInfo1.backplaneIntensity +
    frc*(lightInfo2.backplaneIntensity - lightInfo1.backplaneIntensity);

  lightInfo.highlights = Highlights::interpolate(lightInfo1.highlights,
						 lightInfo2.highlights,
						 frc);

  return lightInfo;
}
void
LightingInformation::load(fstream &fin)
{
  bool done = false;
  char keyword[100];
  float f[3];
  
  while(!done)
    { 
      fin.getline(keyword, 100, 0);
      if (strcmp(keyword, "end") == 0)
	done = true;
      else if (strcmp(keyword, "applyemissive") == 0)
	fin.read((char*)&applyEmissive, sizeof(bool));
      else if (strcmp(keyword, "applylighting") == 0)
	fin.read((char*)&applyLighting, sizeof(bool));
      else if (strcmp(keyword, "applyshadows") == 0)
	fin.read((char*)&applyShadows, sizeof(bool));
      else if (strcmp(keyword, "applycoloredshadows") == 0)
	fin.read((char*)&applyColoredShadows, sizeof(bool));
      else if (strcmp(keyword, "applybackplane") == 0)
	fin.read((char*)&applyBackplane, sizeof(bool));
      else if (strcmp(keyword, "peel") == 0)
	fin.read((char*)&peel, sizeof(bool));
      else if (strcmp(keyword, "colorattenuation") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  colorAttenuation = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "userlightvector") == 0)
	{
	  fin.read((char*)&f, 3*sizeof(float));
	  userLightVector = Vec(f[0], f[1], f[2]);
	}
      else if (strcmp(keyword, "shadowblur") == 0)
	fin.read((char*)&shadowBlur, sizeof(float));
      else if (strcmp(keyword, "shadowscale") == 0)
	fin.read((char*)&shadowScale, sizeof(float));
      else if (strcmp(keyword, "shadowintensity") == 0)
	fin.read((char*)&shadowIntensity, sizeof(float));
      else if (strcmp(keyword, "shadowfovoffset") == 0)
	fin.read((char*)&shadowFovOffset, sizeof(float));
      else if (strcmp(keyword, "lightdistanceoffset") == 0)
	fin.read((char*)&lightDistanceOffset, sizeof(float));
      else if (strcmp(keyword, "backplaneshadowscale") == 0)
	fin.read((char*)&backplaneShadowScale, sizeof(float));
      else if (strcmp(keyword, "backplaneintensity") == 0)
	fin.read((char*)&backplaneIntensity, sizeof(float));
      else if (strcmp(keyword, "ambient") == 0)
	fin.read((char*)&highlights.ambient, sizeof(float));
      else if (strcmp(keyword, "diffuse") == 0)
	fin.read((char*)&highlights.diffuse, sizeof(float));
      else if (strcmp(keyword, "specular") == 0)
	fin.read((char*)&highlights.specular, sizeof(float));
      else if (strcmp(keyword, "specularcoefficient") == 0)
	fin.read((char*)&highlights.specularCoefficient, sizeof(int));
      else if (strcmp(keyword, "peeltype") == 0)
	fin.read((char*)&peelType, sizeof(int));
      else if (strcmp(keyword, "peelmin") == 0)
	fin.read((char*)&peelMin, sizeof(float));
      else if (strcmp(keyword, "peelmax") == 0)
	fin.read((char*)&peelMax, sizeof(float));
      else if (strcmp(keyword, "peelmix") == 0)
	fin.read((char*)&peelMix, sizeof(float));
    }

}

void
LightingInformation::save(fstream &fout)
{
  char keyword[100];
  float f[3];

  memset(keyword, 0, 100);
  sprintf(keyword, "lightinginformation");
  fout.write((char*)keyword, strlen(keyword)+1);

  memset(keyword, 0, 100);
  sprintf(keyword, "applyemissive");
  fout.write((char*)keyword, strlen(keyword)+1);  
  fout.write((char*)&applyEmissive, sizeof(bool));


  memset(keyword, 0, 100);
  sprintf(keyword, "applylighting");
  fout.write((char*)keyword, strlen(keyword)+1);  
  fout.write((char*)&applyLighting, sizeof(bool));


  memset(keyword, 0, 100);
  sprintf(keyword, "applyshadows");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&applyShadows, sizeof(bool));


  memset(keyword, 0, 100);
  sprintf(keyword, "applycoloredshadows");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&applyColoredShadows, sizeof(bool));


  memset(keyword, 0, 100);
  sprintf(keyword, "applybackplane");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&applyBackplane, sizeof(bool));


  memset(keyword, 0, 100);
  sprintf(keyword, "colorattenuation");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = colorAttenuation.x;
  f[1] = colorAttenuation.y;
  f[2] = colorAttenuation.z;
  fout.write((char*)&f, 3*sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "userlightvector");
  fout.write((char*)keyword, strlen(keyword)+1);
  f[0] = userLightVector.x;
  f[1] = userLightVector.y;
  f[2] = userLightVector.z;
  fout.write((char*)&f, 3*sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "shadowblur");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&shadowBlur, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "shadowscale");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&shadowScale, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "shadowintensity");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&shadowIntensity, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "shadowfovoffset");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&shadowFovOffset, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "lightdistanceoffset");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&lightDistanceOffset, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "backplaneshadowscale");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&backplaneShadowScale, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "backplaneintensity");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&backplaneIntensity, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "ambient");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&highlights.ambient, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "diffuse");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&highlights.diffuse, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "specular");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&highlights.specular, sizeof(float));


  memset(keyword, 0, 100);
  sprintf(keyword, "specularcoefficient");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&highlights.specularCoefficient, sizeof(int));


  memset(keyword, 0, 100);
  sprintf(keyword, "peel");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&peel, sizeof(bool));

  memset(keyword, 0, 100);
  sprintf(keyword, "peeltype");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&peelType, sizeof(int));

  memset(keyword, 0, 100);
  sprintf(keyword, "peelmin");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&peelMin, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "peelmax");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&peelMax, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "peelmix");
  fout.write((char*)keyword, strlen(keyword)+1);
  fout.write((char*)&peelMix, sizeof(float));

  memset(keyword, 0, 100);
  sprintf(keyword, "end");
  fout.write((char*)keyword, strlen(keyword)+1);
}
