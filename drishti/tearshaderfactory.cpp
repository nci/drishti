#include "staticfunctions.h"
#include "tearshaderfactory.h"

QString
TearShaderFactory::generateTear(QList<CropObject> crops)
{
  QString shader;

  shader += "vec4 dissect(vec3 texCoord)\n";
  shader += "{\n";
  shader += "  float feather = 1.0;\n";
  shader += "  float myfeather;\n";
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

	  shader += "  myfeather = 0.0;\n";

	  if ( (crops[ci].cropType() >= CropObject::Tear_Tear &&
		crops[ci].cropType() <= CropObject::Tear_Curl) ||
	       (crops[ci].cropType() >= CropObject::View_Tear &&
		crops[ci].cropType() <= CropObject::View_Block &&
		crops[ci].magnify() > 1.0) )
	    {
	      float magnify = 1.0;
	      if (crops[ci].cropType() >= CropObject::View_Tear &&
		  crops[ci].cropType() <= CropObject::View_Block &&
		  crops[ci].magnify() > 1.0)
		magnify = 1.0/crops[ci].magnify();

	      shader += applyTear((pts[0]+pts[1])*0.5,
				  plen*0.5,
				  saxis, taxis, pvec,
				  radX[0], radX[1],
				  radY[0], radY[1],
				  crops[ci].cropType(),
				  lift[0], lift[1],
				  magnify);

	      shader += "feather *= myfeather;\n";
	    }
	  else if (crops[ci].cropType() >= CropObject::Displace_Displace)
	    {
	      shader += applyDisplace((pts[0]+pts[1])*0.5,
				      plen*0.5,
				      saxis, taxis, pvec,
				      radX[0], radX[1],
				      radY[0], radY[1],
				      crops[ci].cropType(),
				      crops[ci].dtranslate(),
				      crops[ci].dpivot(),
				      crops[ci].drotaxis(),
				      crops[ci].drotangle(),
				      lift[0], lift[1]);
	      shader += "feather *= myfeather;\n";
	    }
	}
    }

  shader += "  return vec4(texCoord, feather);\n";
  shader += "}\n\n";

  return shader;
}

QString
TearShaderFactory::applyTear(Vec p0, float plen,
			     Vec axis1, Vec axis2, Vec axis3,
			     float srad1, float srad2,
			     float trad1, float trad2,
			     int tearType,
			     int lift1, int lift2,
			     float magnify)
{
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
      shader += "v0 = texCoord-w0;\n";
      shader += "pvlen = dot(pvec, v0);\n";
      shader += "pvlen /= (2.0*plen);\n";
    }

  shader += "v0 = texCoord-p0;\n";

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
      shader += QString("v0 -= mix(float(%1)*scplen, float(%2)*scplen, pvlen);\n").arg(lift1).arg(lift2);
    }

  shader += "float x = dot(v0, saxis);\n";
  shader += "float s = x/sr;\n"; 

  shader += "float y = dot(v0, taxis);\n";
  shader += "float t = y/tr;\n"; 

  shader += "float s2 = s*s;\n";
  shader += "float t2 = t*t;\n";

  shader += "float frc;\n";
  if (tearType == CropObject::Tear_Tear || 
      tearType == CropObject::Tear_Wedge)
    {
      shader += "if (c2 <= 1.0 && s2<=1.0 && t2<=1.0)\n";
      shader += "{\n";
      if (tearType == CropObject::Tear_Wedge)
	{
	  shader += "  c = 0.5*(1.0+c);\n";
	  shader += "  c = smoothstep(0.4, 1.0, c);\n";
	}
      else
	shader += "  c = smoothstep(0.0, 0.9, abs(c));\n";

      shader += "  c = 0.5*(1.0-c);\n";
      shader += "  c *= (s+1.0);\n";
      shader += "  frc = abs(t);\n";
      shader += "  if (frc < c) return vec4(texCoord, feather);\n";
      shader += "  frc = (frc - c)/(1.0 - c);\n";
      shader += "  texCoord = p0 + z*pvec + x*saxis + frc*sign(t)*tr*taxis;\n";
      shader += "  myfeather = 1.0-smoothstep(0.0, 0.05, frc);\n";
      shader += "}\n";
    }
  else if (tearType == CropObject::Tear_Hole)
    {
      shader += "if (c2 <= 1.0 && s2<=1.0 && t2<=1.0)\n";
      shader += "{\n";
      shader += "  s = max(0.0, s);\n";
      shader += "  s = s*s;\n";
      shader += "  c = 0.4 + 0.4*s;\n";
      shader += "  frc = min((c2+t2), 1.0);\n";
      shader += "  if (frc < c) return vec4(texCoord, feather);\n";
      shader += "  vec3 apt0 = p0 + x*saxis;\n";
      shader += "  frc = (frc - c)/(1.0 - c);\n";
      shader += "  texCoord = mix(apt0, texCoord, frc);\n";
      shader += "  myfeather = 1.0-smoothstep(0.0, 0.05, frc);\n";
      shader += "}\n";
    }
  else if (tearType >= CropObject::View_Tear &&
	   tearType <= CropObject::View_Block)
    {      
      shader += "if (c2 <= 1.0 && s2<=1.0 && t2<=1.0)\n";
      shader += "{\n";
      shader += "  if (c2+s2+t2 <= 1.0)\n";
      shader += "  {\n";
      shader += QString("    float frc = float(%1) + float(%2)*smoothstep(0.5, 1.0, c2+s2+t2);\n").\
	arg(magnify).arg(1.0-magnify);
      shader += "    s*=frc; c*=frc; t*=frc;\n";
      shader += "    texCoord = p0 + c*plen*pvec + s*sr*saxis + t*tr*taxis;\n";
      shader += "  }\n";
      shader += "}\n";
    }
  else if (tearType == CropObject::Tear_Curl)
    {
      shader += "if (c2 <= 1.0 && s2<=1.0 && t2<=1.0)\n";
      shader += "{\n";
      shader += "  vec3 oc = p0 + z*pvec + sr/2.0*saxis - tr*taxis;\n";

      shader += "  vec3 vdir = texCoord-oc;\n";
      shader += "  float vlen = length(vdir);\n";
      shader += "  vdir = normalize(vdir);\n";

      shader += "  float theta = acos(dot(vdir, -saxis));\n";

      shader += "  frc = theta/radians(180.0);\n";

      shader += "  float r0 = 0.5*sr;\n";
      shader += "  float r1 = 1.5*sr;\n";
      shader += "  r0 = mix(r0, 0.1*r0, frc);\n";
      shader += "  r1 = mix(r1, 0.1*r1, frc);\n";

      shader += "  if (vlen < r0 || vlen > r1)\n";
      shader += "  {\n";
      shader += "    myfeather = 1.0 - smoothstep(0.9, 1.0, c2);\n";
      shader += "    myfeather = min(myfeather, 1.0-smoothstep(0.9, 1.0, s2));\n";
      shader += "    myfeather = min(myfeather, 1.0-smoothstep(0.9, 1.0, t2));\n";
      shader += "  }\n";
      shader += "  else\n";
      shader += "  {\n";
      shader += "    s = (vlen-r0)/(r1-r0);\n";
      shader += "    myfeather = 1.0 - smoothstep(0.0, 0.1, s);\n";
      shader += "    myfeather = max(myfeather, smoothstep(0.7, 1.0, s));\n";
      shader += "    s = 2.0*s;\n";
      shader += "    texCoord = p0 + z*pvec - (-tr+2.0*frc*tr)*taxis - s*sr*saxis;\n";
      shader += "  }\n";

      shader += "}\n";
    }

  shader += "}\n";

  return shader;
}

QString
TearShaderFactory::applyDisplace(Vec p0, float plen,
				 Vec axis1, Vec axis2, Vec axis3,
				 float srad1, float srad2,
				 float trad1, float trad2,
				 int tearType,
				 Vec dtranslate,
				 Vec dpivot,
				 Vec drotaxis, float drotangle,
				 int lift1, int lift2)
{
  // check if displacement is needed
  if (fabs(drotangle) < 0.001f  &&
      dtranslate.squaredNorm() < 0.001f)
    return "";


  bool sradEqual = (fabs(srad1-srad2) < 0.001f);
  bool tradEqual = (fabs(trad1-trad2) < 0.001f);

  QString shader;

  shader += QString("plen = float(%1);\n").arg(plen);
  shader += QString("srad1 = float(%1);\n").arg(srad1);
  shader += QString("trad1 = float(%1);\n").arg(trad1);

  if (!sradEqual)
    shader += QString("srad2 = float(%1);\n").arg(srad2);
  if (!tradEqual)
    shader += QString("trad2 = float(%1);\n").arg(trad2);


  Vec newp0 = p0;

  Vec translate = dtranslate.x*2*srad1*axis1 +
	          dtranslate.y*2*trad1*axis2 +
	           dtranslate.z*2*plen*axis3;

  Vec rotaxis = drotaxis.x*axis1 +
                drotaxis.y*axis2 +
                drotaxis.z*axis3;
  rotaxis = rotaxis.unit();

  Vec pivot = p0 - srad1*axis1 - trad1*axis2 - plen*axis3 +
              dpivot.x*2*srad1*axis1 +
              dpivot.y*2*trad1*axis2 +
               dpivot.z*2*plen*axis3;

  Vec p0vec = p0 - pivot;

  Quaternion rot = Quaternion(rotaxis, DEG2RAD(drotangle));
  Vec saxis = rot.rotate(axis1);
  Vec taxis = rot.rotate(axis2);
  Vec pvec = rot.rotate(axis3);
  p0vec = rot.rotate(p0vec);
  newp0 = pivot + p0vec;
  newp0 += translate;
  
  shader += QString("p0 = vec3(%1, %2, %3);\n").		\
                     arg(newp0.x).arg(newp0.y).arg(newp0.z);
  shader += QString("saxis = vec3(%1, %2, %3);\n").		\
                     arg(saxis.x).arg(saxis.y).arg(saxis.z);
  shader += QString("taxis = vec3(%1, %2, %3);\n").		\
                     arg(taxis.x).arg(taxis.y).arg(taxis.z);
  shader += QString("pvec = vec3(%1, %2, %3);\n").		\
                     arg(pvec.x).arg(pvec.y).arg(pvec.z);


  shader += QString("op0 = vec3(%1, %2, %3);\n").	\
                     arg(p0.x).arg(p0.y).arg(p0.z);
  shader += QString("osaxis = vec3(%1, %2, %3);\n").	\
                     arg(axis1.x).arg(axis1.y).arg(axis1.z);
  shader += QString("otaxis = vec3(%1, %2, %3);\n").	\
                     arg(axis2.x).arg(axis2.y).arg(axis2.z);
  shader += QString("opvec = vec3(%1, %2, %3);\n").	\
                     arg(axis3.x).arg(axis3.y).arg(axis3.z);


  shader += "{\n";

  shader += "vec3 v0;\n";
  shader += "float pvlen;\n";
  if (!sradEqual || !tradEqual)
    {
      shader += "vec3 w0 = p0-plen*pvec;\n"; // we are given center p0 - now get bottom point
      shader += "v0 = texCoord-w0;\n";
      shader += "pvlen = dot(pvec, v0);\n";
      shader += "pvlen /= (2.0*plen);\n";
    }

  shader += "v0 = texCoord-p0;\n";

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
      shader += QString("v0 -= mix(float(%1)*scplen, float(%2)*scplen, pvlen);\n").arg(lift1).arg(lift2);
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
  shader += "  texCoord = op0 + z*opvec + x*osaxis + y*otaxis;\n";
  shader += "  myfeather = smoothstep(0.95, 1.0, c2);\n";
  shader += "  myfeather = max(myfeather, smoothstep(0.95, 1.0, s2));\n";
  shader += "  myfeather = max(myfeather, smoothstep(0.95, 1.0, t2));\n";
  shader += "}\n";
  shader += "else\n";
  shader += "{\n";
  if (!sradEqual || !tradEqual)
    {
      shader += "w0 = op0-plen*opvec;\n";
      shader += "v0 = texCoord-w0;\n";
      shader += "pvlen = dot(opvec, v0);\n";
      shader += "pvlen /= (2.0*plen);\n";
      shader += "v0 = texCoord-op0;\n";
    }
  shader += "v0 = texCoord-op0;\n";
  if (sradEqual)
    shader += "sr = srad1;\n";
  else
    shader += "sr = (1.0-pvlen)*srad1 + pvlen*srad2;\n";
  if (tradEqual)
    shader += "tr = trad1;\n";
  else
    shader += "tr = (1.0-pvlen)*trad1 + pvlen*trad2;\n";
  
  shader += "x = dot(v0, osaxis);\n";
  shader += "s = x/sr;\n"; 
  
  shader += "y = dot(v0, otaxis);\n";
  shader += "t = y/tr;\n"; 
  
  shader += "z = dot(v0, opvec);\n";
  shader += "c = z/plen;\n"; 
  
  shader += "s2 = s*s;\n";
  shader += "t2 = t*t;\n";
  shader += "c2 = c*c;\n";
  
  shader += "if (c2<=0.95 && s2<=0.95 && t2<=0.95)\n";
  shader += "{\n";
  shader += "  return vec4(texCoord, feather);\n";
  shader += "  myfeather = (1.0-smoothstep(0.95, 1.0, c2));\n";
  shader += "  myfeather *= (1.0-smoothstep(0.95, 1.0, s2));\n";
  shader += "  myfeather *= (1.0-smoothstep(0.95, 1.0, t2));\n";
  shader += "}\n";
  
  shader += "}\n";

  shader += "}\n";

  return shader;
}
