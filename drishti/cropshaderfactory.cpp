#include "staticfunctions.h"
#include "cropshaderfactory.h"

QString
CropShaderFactory::generateCropping(QList<CropObject> crops)
{
  int ncrops = 0;
  for (int ci=0; ci<crops.count(); ci++)
    {
      if (crops[ci].cropType() < CropObject::Tear_Tear)
	ncrops ++;
    }

  QString shader;

  shader += "float crop(vec3 vpos, bool reject)\n";
  shader += "{\n";
  shader += "  bool isCropped = false;\n";
  shader += "  bool isNotCropped = false;\n";
  shader += "  float feather = 1.0;\n";
  shader += "  float myfeather;\n";
  shader += "  vec3 p0, pvec, saxis, taxis;\n";
  shader += "  vec3 op0, opvec, osaxis, otaxis;\n";
  shader += "  float plen, srad1, srad2, trad1, trad2;\n";
  shader += "  bool hatch, hatchGrid;\n";
  shader += "  int xn, xd, yn, yd, zn, zd;\n";

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
	  shader += "  isCropped = false;\n";

	  if (crops[ci].cropType() < CropObject::Tear_Tear)
	    {
	      bool hatch, hg;
	      int hxn, hyn, hzn, hxd, hyd, hzd;
	      hatch = crops[ci].hatch();
	      crops[ci].hatchParameters(hg, hxn, hxd, hyn, hyd, hzn, hzd);
	      if (!hatch)
		shader += "hatch = false;\n";
	      else
		{
		  shader += "hatch = true;\n";
		  if (hg)
		    shader += "hatchGrid = true;\n";
		  else
		    shader += "hatchGrid = false;\n";
		  shader += QString("xn = %1;\n").arg(hxn);
		  shader += QString("yn = %1;\n").arg(hyn);
		  shader += QString("zn = %1;\n").arg(hzn);
		  shader += QString("xd = %1;\n").arg(hxd);
		  shader += QString("yd = %1;\n").arg(hyd);
		  shader += QString("zd = %1;\n").arg(hzd);
		}
	    }

	  if (crops[ci].cropType() == CropObject::Crop_Tube)
	    {
	      shader += applyTubeCropping(crops[ci].keepInside(),
					  crops[ci].keepEnds(),
					  crops[ci].halfSection(),
					  pts[0], pvec, plen,
					  saxis, taxis,
					  radX[0], radX[1],
					  radY[0], radY[1],
					  lift[0], lift[1],
					  ncrops);
	    }
	  else if (crops[ci].cropType() == CropObject::Crop_Box)
	    {
	      shader += applyBoxCropping(crops[ci].keepInside(),
					 crops[ci].keepEnds(),
					 crops[ci].halfSection(),
					 (pts[0]+pts[1])*0.5,
					 plen*0.5,
					 saxis, taxis, pvec,
					 radX[0], radX[1],
					 radY[0], radY[1],
					 lift[0], lift[1],
					 ncrops);
	    }
	  else if (crops[ci].cropType() == CropObject::Crop_Ellipsoid)
	    {
	      shader += applyEllipsoidCropping(crops[ci].keepInside(),
					       crops[ci].keepEnds(),
					       crops[ci].halfSection(),
					       (pts[0]+pts[1])*0.5,
					       plen*0.5,
					       saxis, taxis, pvec,
					       (radX[0]+radX[1])*0.5,
					       (radY[0]+radY[1])*0.5,
					       lift[0], lift[1],
					       ncrops);
	    }

	  if (crops[ci].cropType() < CropObject::Tear_Tear)
	    shader += "feather *= myfeather;\n";

	  shader += "isNotCropped = isNotCropped || !isCropped;\n";
	}
    }
  shader += "  if (!isNotCropped) return 1.0;\n";

  shader += "  return feather;\n";
  shader += "}\n\n";

  return shader;
}

#define ShaderRejectString()						\
  shader += "  {\n";							\
  shader  += "  if (!hatch)\n";						\
  shader += "   {\n";							\
  if (ncrops == 1)							\
    {									\
      shader  += "    if (reject) discard;\n";				\
      shader  += "    else return 1.0;\n";				\
    }									\
  else									\
    {									\
      shader += "isCropped = true;\n";					\
    }									\
  shader += "   }\n";							\
  shader += "  else\n";							\
  shader += "   {\n";							\
  shader += "    if (hatchGrid)\n";					\
  shader += "      myfeather = 1.0;\n";					\
  shader += "    else\n";						\
  shader += "      myfeather = 0.0;\n";					\
  shader += "    int oz = int(floor(0.5*(c+1.0)*100.0));\n";		\
  shader += "    int oy = int(floor(0.5*(t+1.0)*100.0));\n";		\
  shader += "    int ox = int(floor(0.5*(s+1.0)*100.0));\n";		\
  shader += "    float hfe = 1.0;\n";					\
  shader += "    if (xn > 0 && xd > 0)\n";				\
  shader += "      hfe = hfe * smoothstep(float(xd-1), float(xd+1), mod(float(ox),float(xn)));\n"; \
  shader += "    if (yn > 0 && yd > 0)\n";				\
  shader += "      hfe = hfe * smoothstep(float(yd-1), float(yd+1), mod(float(oy),float(yn)));\n"; \
  shader += "    if (zn > 0 && zd > 0)\n";				\
  shader += "      hfe = hfe * smoothstep(float(zd-1), float(zd+1), mod(float(oz),float(zn)));\n"; \
  shader += "    if (hatchGrid)\n";					\
  shader += "      myfeather = hfe;\n";					\
  shader += "    else\n";						\
  shader += "      myfeather = 1.0 - hfe;\n";				\
  shader += "    if (myfeather > 0.0999) isCropped = true;\n";		\
  shader += "   }\n";							\
  shader += "  }\n";

  
QString
CropShaderFactory::applyTubeCropping(bool keepInside,
				     bool keepEnds,
				     bool halfSection,
				     Vec p0, Vec pvec, float plen,
				     Vec saxis, Vec taxis,
				     float srad1, float srad2,
				     float trad1, float trad2,
				     int lift1, int lift2,
				     int ncrops)
{
  float feather = 0.05f;
  bool sradEqual = (fabs(srad1-srad2) < 0.001f);
  bool tradEqual = (fabs(trad1-trad2) < 0.001f);


  QString shader;  
  shader += QString("p0 = vec3(%1, %2, %3);\n").\
                     arg(p0.x).arg(p0.y).arg(p0.z);
  shader += QString("pvec = vec3(%1, %2, %3);\n").\
                     arg(pvec.x).arg(pvec.y).arg(pvec.z);
  shader += QString("plen = float(%1);\n").arg(plen);
  shader += QString("saxis = vec3(%1, %2, %3);\n").\
                     arg(saxis.x).arg(saxis.y).arg(saxis.z);
  shader += QString("taxis = vec3(%1, %2, %3);\n").\
                     arg(taxis.x).arg(taxis.y).arg(taxis.z);

  shader += QString("srad1 = float(%1);\n").arg(srad1);
  shader += QString("trad1 = float(%1);\n").arg(trad1);

  if (!sradEqual)
    shader += QString("srad2 = float(%1);\n").arg(srad2);
  if (!tradEqual)
    shader += QString("trad2 = float(%1);\n").arg(trad2);

  shader += "myfeather = 0.0;\n";

  shader += "{\n";

  shader += "vec3 v0 = vpos-p0;\n";
  shader += "float pvlen = dot(pvec, v0);\n";

  shader += "float c = 2.0*(pvlen/plen - 0.5);\n";
  shader += "float c2 = c*c;\n";

  shader += "vec3 pv = vpos - (p0 + pvlen*pvec);\n";

  shader += "pvlen /= plen;\n";

  if (lift1 != 0 || lift2 != 0)
    {
      shader += "vec3 sc2 = (1.0-c2)*saxis;\n";
      shader += QString("pv -= mix(%1*sc2, %2*sc2, pvlen);\n").arg(lift1).arg(lift2);
    }


  if (!keepEnds)
    {
      shader += QString("myfeather = smoothstep(0.0, %1, pvlen);\n").arg(feather);
      shader += QString("myfeather *= (1.0-smoothstep(1.0-float(%1), 1.0, pvlen));\n").arg(feather);
    }
  else
    {
      shader += QString("myfeather = smoothstep(-%1, 0.0, pvlen);\n").arg(feather);
      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), pvlen));\n").arg(feather);
    }
  
  shader += "float s = dot(pv, saxis);\n"; 
  shader += "float t = dot(pv, taxis);\n"; 

  if (sradEqual)
    shader += "float sr = s/srad1;\n";
  else
    shader += "float sr = s/((1.0-pvlen)*srad1 + pvlen*srad2);\n";
  
  if (tradEqual)
    shader += "float tr = t/trad1;\n";
  else
    shader += "float tr = t/((1.0-pvlen)*trad1 + pvlen*trad2);\n";
  
  shader += "pvlen = sr*sr + tr*tr;\n";

  shader += " s = sr; t = tr;\n";
  
  if (!keepEnds)
    {
      shader += "bool ends = (pvlen < 0.0 || pvlen > plen);\n";
      shader += "if (ends) \n";
      ShaderRejectString()
    }

  if (keepEnds)
    {
      if (keepInside)
	{
	  if (halfSection)
	    {
	      shader += "if (c2<1.0 && t>0.0 && pvlen > 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = smoothstep(1.0-float(%1), 1.0, pvlen);\n").arg(feather);
	      shader += QString("myfeather = min(myfeather, (smoothstep(-%1, 0.0, tr)));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	  else
	    {
	      shader += "if (c2<1.0 && pvlen > 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = smoothstep(1.0-float(%1), 1.0, pvlen);\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	}
      else // keep outside
	{
	  if (halfSection)
	    {
	      shader += "if (c2<1.0 && (t<0.0 || pvlen < 1.0))\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = (1.0-smoothstep(1.0, 1.0+float(%1), pvlen));\n").arg(feather);
	      shader += QString("myfeather = max(myfeather, (1.0-smoothstep(0.0, %1, tr)));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	  else
	    {
	      shader += "if (c2<1.0 && pvlen < 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = (1.0-smoothstep(1.0, 1.0+float(%1), pvlen));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	}
     }
  else // do not keepends
    {
      if (keepInside)
	{
 	  if (halfSection)
	    {
	      shader += "  if (t > 0.0 && pvlen > 1.0)\n";
	      ShaderRejectString()

	      shader += "else\n";
	      shader += "{\n";
	      shader += QString("float myf = smoothstep(1.0-float(%1), 1.0, pvlen);\n").arg(feather);
	      shader += QString("myf *= smoothstep(-%1, 0.0, tr);\n").arg(feather);
	      shader += "myfeather *= (1.0-myf);\n";
	      shader += "myfeather = 1.0-myfeather;\n";
	      shader += "}\n";
	    }
	  else
	    {
	      shader += "if (pvlen > 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather *= (1.0-smoothstep(1.0-float(%1), 1.0, pvlen));\n").arg(feather);
	      shader += "myfeather = 1.0-myfeather;\n";
	      shader += "  }\n";
	    }
	}
      else // keep outside
	{
 	  if (halfSection)
	    {
	      shader += "  if (t<0.0 || (t > 0.0 && pvlen < 1.0))\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather *= smoothstep(1.0, 1.0+float(%1), pvlen);\n").arg(feather);
	      shader += QString("myfeather *= smoothstep(0.0, %1, tr);\n").arg(feather);
	      shader += "myfeather = 1.0-myfeather;\n";
	      shader += "  }\n";
	    }
	  else
	    {
	      shader += "if (pvlen < 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), pvlen));\n").arg(feather);
	      shader += "  }\n";
	    }
	}
    }

  shader += "}\n";

  return shader;
}

QString
CropShaderFactory::applyEllipsoidCropping(bool keepInside,
					  bool keepEnds,
					  bool halfSection,
					  Vec p0, float plen,
					  Vec axis1, Vec axis2, Vec axis3,
					  float srad1,
					  float trad1,
					  int lift1, int lift2,
					  int ncrops)
{
  float feather = 0.1f;

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

  shader += "{\n";

  shader += "vec3 v0 = vpos-p0;\n";
  shader += "float c = dot(v0, pvec)/plen;\n"; 
  shader += "float c2 = c*c;\n";
  
  shader += "float pvlen;\n";
  if (lift1 != 0 || lift2 != 0)
    {
      shader += "pvlen = 0.5*(c + 1.0);\n";
      shader += "vec3 scplen = (1.0-c2)*saxis;\n";
      shader += QString("v0 -= mix(%1*scplen, %2*scplen, pvlen);\n").arg(lift1).arg(lift2);
    }

  shader += "float a = dot(v0, saxis)/srad1;\n"; 
  shader += "float b = dot(v0, taxis)/trad1;\n"; 
  shader += "float a2 = a*a;\n";
  shader += "float b2 = b*b;\n";
  shader += "pvlen = a2 + b2 + c2;\n";
  shader += "float s = a;\n"; 
  shader += "float t = b;\n"; 

  if (!keepEnds)
    { 
      shader += "if (c2 > 1.0)\n";
      ShaderRejectString()

      shader += "  else\n";
      shader += "  {\n";
      if (keepInside)
	{ // keep inside
	  if (halfSection)
	    {
	      shader += "if (b>0.0 && pvlen > 1.0)\n";	      
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = smoothstep(1.0-float(%1), 1.0, pvlen);\n").arg(feather);
	      shader += QString("myfeather = min(myfeather, (smoothstep(-%1, 0.0, b)));\n").arg(feather);
	      shader += QString("myfeather = max(myfeather, smoothstep(1.0-float(%1), 1.0, c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	  else
	    {
	      shader += "if (pvlen > 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = (1.0-smoothstep(1.0-float(%1), 1.0, pvlen));\n").arg(feather);
	      shader += "myfeather = 1.0-myfeather;\n";
	      shader += "  }\n";
	    }
	}
      else
	{ // keep outside
	  if (halfSection)
	    {
	      shader += "if (b<0.0 || pvlen < 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = (1.0-smoothstep(1.0, 1.0+float(%1), pvlen));\n").arg(feather);
	      shader += QString("myfeather = max(myfeather, (1.0-smoothstep(0.0, %1, b)));\n").arg(feather);
	      shader += QString("myfeather = max(myfeather, smoothstep(1.0-float(%1), 1.0, c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	  else
	    {
	      shader += "if (pvlen < 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = (1.0-smoothstep(1.0, 1.0+float(%1), pvlen));\n").arg(feather);
	      shader += "  }\n";
	    }
	}
      shader += "  }\n";
    }
  else
    { // keep ends
      if (keepInside)
	{
	  if (halfSection)
	    {
	      shader += "if (b>0.0 && pvlen > 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = smoothstep(1.0-float(%1), 1.0, pvlen);\n").arg(feather);
	      shader += QString("myfeather = min(myfeather, (smoothstep(-%1, 0.0, b)));\n").arg(feather);
	      shader += "  }\n";
	    }
	  else
	    {
	      shader += "if (c2<1.0 && pvlen > 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = (smoothstep(1.0-float(%1), 1.0, pvlen));\n").arg(feather);
	      shader += QString("myfeather = min(myfeather, 1.0-smoothstep(1.0, 1.0+float(%1), c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	}
      else
	{ // keep outside
	  if (halfSection)
	    {
	      shader += "if (b<0.0 || pvlen < 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = (1.0-smoothstep(1.0, 1.0+float(%1), pvlen));\n").arg(feather);
	      shader += QString("myfeather = max(myfeather, (1.0-smoothstep(0.0, %1, b)));\n").arg(feather);
	      shader += "  }\n";
	    }
	  else
	    {
	      shader += "if (pvlen < 1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = (1.0-smoothstep(1.0, 1.0+float(%1), pvlen));\n").arg(feather);
	      shader += "  }\n";
	    }
	}
    }


  shader += "}\n";

  return shader;
}

QString
CropShaderFactory::applyBoxCropping(bool keepInside,
				    bool keepEnds,
				    bool halfSection,
				    Vec p0, float plen,
				    Vec axis1, Vec axis2, Vec axis3,
				    float srad1, float srad2,
				    float trad1, float trad2,
				    int lift1, int lift2,
				    int ncrops)
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
      shader += "vec3 w0 = p0-plen*pvec;\n";
      shader += "v0 = vpos-w0;\n";
      shader += "pvlen = dot(pvec, v0);\n";
      shader += "pvlen /= (2*plen);\n";
    }

  shader += "v0 = vpos-p0;\n";

  if (sradEqual)
    shader += "float sr = srad1;\n";
  else
    shader += "float sr = (1.0-pvlen)*srad1 + pvlen*srad2;\n";
  
  if (tradEqual)
    shader += "float tr = trad1;\n";
  else
    shader += "float tr = (1.0-pvlen)*trad1 + pvlen*trad2;\n";


  shader += "float c = dot(v0, pvec)/plen;\n"; 

  if (lift1 != 0 || lift2 != 0)
    {
      shader += "float cplen = c;\n"; 
      shader += "pvlen = 0.5*(cplen + 1.0);\n";
      shader += "vec3 scplen = (1.0-cplen*cplen)*saxis;\n";
      shader += QString("v0 -= mix(%1*scplen, %2*scplen, pvlen);\n").arg(lift1).arg(lift2);
    }

  shader += "float s = dot(v0, saxis)/sr;\n"; 
  shader += "float t = dot(v0, taxis)/tr;\n"; 

  shader += "float c2 = c*c;\n";
  shader += "float s2 = s*s;\n";
  shader += "float t2 = t*t;\n";

  if (!keepEnds)
    {
      shader += "if (c2 > 1.0)\n";
      ShaderRejectString()

      shader += "  else\n";
      shader += "  {\n";

      if (keepInside)
	{ // keep inside

	  if (halfSection)
	    {
	      shader += "if (t>0.0 && (s2>1.0 || t2>1.0))\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather  = (smoothstep(1.0-float(%1), 1.0, max(s2, t2)));\n").arg(feather);
	      shader += QString("myfeather *= (smoothstep(-%1, 0.0, t));\n").arg(feather);
	      shader += QString("myfeather = max(myfeather, smoothstep(1.0-float(%1), 1.0, c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	  else
	    {
	      shader += "if (s2>1.0 || t2>1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather  = (1.0-smoothstep(1.0-float(%1), 1.0, s2));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0-float(%1), 1.0, t2));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0-float(%1), 1.0, c2));\n").arg(feather);
	      shader += "myfeather = 1.0-myfeather;\n";
	      shader += "  }\n";
	    }
	}
      else
	{ // keep outside
	  if (halfSection)
	    {
	      shader += "if (t<0.0 || (s2<1.0 && t2<1.0))\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = (1.0-smoothstep(0.0, %1, t));\n").arg(feather);
	      shader += QString("myfeather = max(myfeather, 1.0-smoothstep(1.0, 1.0+float(%1), max(s2, t2)));\n").arg(feather);
	      shader += QString("myfeather = max(myfeather, smoothstep(1.0-float(%1), 1.0, c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	  else
	    {
	      shader += "if (s2<1.0 && t2<1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather  = (1.0-smoothstep(1.0, 1.0+float(%1), s2));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), t2));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0-float(%1), 1.0, c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	}
      shader += "  }\n";
    }


  if (keepEnds)
    {
      if (keepInside)
	{ // keep inside
	  if (halfSection)
	    {
	      shader += "if (c2<=1.0 && (t>0.0 && (s2>1.0 || t2>1.0)))\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather  = (smoothstep(1.0-float(%1), 1.0, max(s2, t2)));\n").arg(feather);
	      shader += QString("myfeather *= (smoothstep(-%1, 0.0, t));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	  else
	    {
	      shader += "if (c2 <= 1.0 && (s2>1.0 || t2>1.0))\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather  = (1.0-smoothstep(1.0-float(%1), 1.0, s2));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0-float(%1), 1.0, t2));\n").arg(feather);
	      shader += "myfeather = 1.0-myfeather;\n";
	      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	}
      else
	{ // keep outside
	  if (halfSection)
	    {
	      shader += "if (c2 <= 1.0 && (t<0.0 || (s2<1.0 && t2<1.0)))\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather = (1.0-smoothstep(0.0, %1, t));\n").arg(feather);
	      shader += QString("myfeather = max(myfeather, 1.0-smoothstep(1.0, 1.0+float(%1), max(s2, t2)));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	  else
	    {
	      shader += "if (c2<=1.0 && s2<1.0 && t2<1.0)\n";
	      ShaderRejectString()

	      shader += "  else\n";
	      shader += "  {\n";
	      shader += QString("myfeather  = (1.0-smoothstep(1.0, 1.0+float(%1), s2));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), t2));\n").arg(feather);
	      shader += QString("myfeather *= (1.0-smoothstep(1.0, 1.0+float(%1), c2));\n").arg(feather);
	      shader += "  }\n";
	    }
	}
    }

  shader += "}\n";

  return shader;
}
