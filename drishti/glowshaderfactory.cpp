#include "global.h"
#include "staticfunctions.h"
#include "glowshaderfactory.h"

QString
GlowShaderFactory::generateGlow(QList<CropObject> crops,
				int nvol,
				QString fragColor)
{
  QString shader;

  shader += "vec3 glow(vec3 otexCoord)\n";
  shader += "{\n";
  shader += "  float glowMix;\n";
  shader += "  vec3 glowColor = vec3(0,0,0);\n";
  shader += "  vec3 p0, pvec, saxis, taxis;\n";
  shader += "  vec3 op0, opvec, osaxis, otaxis;\n";
  shader += "  float plen, srad1, srad2, trad1, trad2;\n";

  if (crops.count() > 0)
    {
      for (int ci=0; ci<crops.count(); ci++)
	{
	  
	  QList<Vec> pts;
	  QList<float> radX;
	  QList<float> radY;
	  QList<int> lift;
	  Vec pvec, saxis, taxis;
	  float plen;
	  
	  pts = crops[ci].points();
	  radX = crops[ci].radX();
	  radY = crops[ci].radY();
	  lift = crops[ci].lift();

	  pvec = pts[1]-pts[0];
	  plen = pvec.norm();

	  pvec = crops[ci].m_tang;
	  saxis = crops[ci].m_xaxis;
	  taxis = crops[ci].m_yaxis;

	  if (crops[ci].cropType() >= CropObject::Glow_Ball)
	    {
	      shader += "glowMix = 0.0;\n";
	      shader += applyGlow(crops[ci].keepInside(),
				  crops[ci].keepEnds(),
				  crops[ci].halfSection(),
				  (pts[0]+pts[1])*0.5f,
				  plen*0.5f,
				  saxis, taxis, pvec,
				  radX[0], radX[1],
				  radY[0], radY[1],
				  crops[ci].cropType(),
				  crops[ci].viewMix(),
				  lift[0], lift[1]);

	      Vec cropColor = crops[ci].color();
	      shader += QString("glowColor += glowMix*%1*vec3(%2, %3, %4);\n").	\
		                               arg(crops[ci].opacity()*0.3).\
		                               arg(cropColor.x).\
		                               arg(cropColor.y).\
		                               arg(cropColor.z);
	    }
	}
    }

  shader += "  return glowColor;\n";
  shader += "}\n\n";

  return shader;
}

QString
GlowShaderFactory::applyGlow(bool keepInside,
			     bool keepEnds,
			     bool halfSection,
			     Vec p0, float plen,
			     Vec axis1, Vec axis2, Vec axis3,
			     float srad1, float srad2,
			     float trad1, float trad2,
			     int viewType,
			     float glowMix,
			     int lift1, int lift2)
{
  float feather = 0.1f;
  bool sradEqual = (fabs(srad1-srad2) < 0.001f);
  bool tradEqual = (fabs(trad1-trad2) < 0.001f);

  QString shader;
  shader += QString("p0 = vec3(%1, %2, %3);\n").\
                     arg(p0.x).arg(p0.y).arg(p0.z);
  shader += QString("saxis = vec3(%1, %2, %3);\n").\
                     arg(axis1.x).arg(axis1.y).arg(axis1.z);
  shader += QString("taxis = vec3(%1, %2, %3);\n").\
                     arg(axis2.x).arg(axis2.y).arg(axis2.z);
  shader += QString("pvec = vec3(%1, %2, %3);\n").\
                     arg(axis3.x).arg(axis3.y).arg(axis3.z);

  shader += QString("plen = float(%1);\n").arg(plen);
  shader += QString("srad1 = float(%1);\n").arg(srad1);
  shader += QString("trad1 = float(%1);\n").arg(trad1);

  if (!sradEqual)
    shader += QString("srad2 = float(%1);\n").arg(srad2);
  if (!tradEqual)
    shader += QString("trad2 = float(%1);\n").arg(trad2);


  shader += "{\n";

  shader += "vec3 v0;\n";
  shader += "float pvlen;\n";
  if (!sradEqual || !tradEqual)
    {
      shader += "vec3 w0 = p0-plen*pvec;\n"; // we are given center p0 - now get bottom point
      shader += "v0 = otexCoord-w0;\n";
      shader += "pvlen = dot(pvec, v0);\n";
      shader += "pvlen /= (2*plen);\n";
    }

  shader += "v0 = otexCoord-p0;\n";

  if (sradEqual)
    shader += "float sr = srad1;\n";
  else
    shader += "float sr = (1.0-pvlen)*srad1 + pvlen*srad2;\n";
  
  if (tradEqual)
    shader += "float tr = trad1;\n";
  else
    shader += "float tr = (1.0-pvlen)*trad1 + pvlen*trad2;\n";


  shader += "float z = dot(v0, pvec);\n";
  shader += "float c = z/plen;\n"; 
  shader += "float c2 = c*c;\n";

  if (lift1 != 0 || lift2 != 0)
    {
      shader += "pvlen = 0.5*(c + 1.0);\n";
      shader += "vec3 scplen = (1.0-c2)*saxis;\n";
      shader += QString("v0 -= mix(%1*scplen, %2*scplen, pvlen);\n").arg(lift1).arg(lift2);
    }

  shader += "float x = dot(v0, saxis);\n";
  shader += "float s = x/sr;\n"; 

  shader += "float y = dot(v0, taxis);\n";
  shader += "float t = y/tr;\n"; 

  shader += "float s2 = s*s;\n";
  shader += "float t2 = t*t;\n";

  shader += "float frc;\n";
  shader += "if (c2 <= 1.0 && s2<=1.0 && t2<=1.0)\n";
  shader += "{\n";

  if (viewType == CropObject::Glow_Ball)
    shader += "  frc = min((c2+t2+s2), 1.0);\n";
  else if (viewType == CropObject::Glow_Block)
    shader += "  frc = max(c2,max(t2,s2));\n";
  else if (viewType == CropObject::Glow_Tube)
    shader += "  frc = min((t2+s2), 1.0);\n";

  if (glowMix > 0.0)
    shader += QString("  glowMix = 1.0 - smoothstep(float(%1), 1.0, frc);\n").arg(1.0-glowMix);
  else
    shader += "  glowMix = 1.0 - smoothstep(0.95, 1.0, frc);\n";

  shader += "  }\n";  
  shader += "}\n";

  return shader;
}
