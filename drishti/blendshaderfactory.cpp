#include "global.h"
#include "staticfunctions.h"
#include "blendshaderfactory.h"

QString
BlendShaderFactory::generateBlend(QList<CropObject> crops,
				  int nvol)
{
  Vec voxelScaling = Global::voxelScaling();
  QString shader;

  if (nvol < 2) shader +=  "void blend(bool sendTfSet, vec3 otexCoord, vec2 vg, inout vec4 fragColor)\n";
  if (nvol == 2) shader += "void blend(vec3 otexCoord, vec2 vol1, vec2 vol2, float grad1, float grad2, inout vec4 color1, inout vec4 color2)\n";
  if (nvol == 3) shader += "void blend(vec3 otexCoord, vec2 vol1, vec2 vol2, vec2 vol3, float grad1, float grad2, float grad3, inout vec4 color1, inout vec4 color2, inout vec4 color3)\n";
  if (nvol == 4) shader += "void blend(vec3 otexCoord, vec2 vol1, vec2 vol2, vec2 vol3, vec2 vol4, float grad1, float grad2, float grad3, float grad4, inout vec4 color1, inout vec4 color2, inout vec4 color3, inout vec4 color4)\n";
  shader += "{\n";
  shader += "  float viewMix;\n";
  shader += "  vec2 vgc;\n";
  shader += "  vec4 vcol;\n";
  shader += "  vec3 p0, pvec, saxis, taxis;\n";
  shader += "  vec3 op0, opvec, osaxis, otaxis;\n";
  shader += "  float plen, srad1, srad2, trad1, trad2;\n";

  shader += QString("  otexCoord *= vec3(%1,%2,%3);\n").\
    arg(voxelScaling.x).arg(voxelScaling.y).arg(voxelScaling.z);

  if (crops.count() > 0)
    {
      bool clearViewPresent = false;
      if (nvol > 0) // clearView works only with volumes
	{
	  for (int ci=0; ci<crops.count(); ci++)
	    {
	      if (crops[ci].clearView())
		{
		  clearViewPresent = true;
		  break;
		}
	    }
	}

      if (clearViewPresent) // clearView works only with volumes
	{
	  shader += "  vec3 otexCoord1;\n";
	  shader += "  bool infront;\n";
	  shader += "  bool isinFront = false;\n";
	  shader += "  float infrontviewMix = 0.0;\n";
	  shader += "  float viewMix1 = 1.0;\n";
	}

      shader += "  vec4 rgbU = vec4(0.0,0.0,0.0,0.0);\n";
      shader += "  float alphaU = 0.0;\n";

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

	  if (crops[ci].cropType() >= CropObject::View_Tear)
	    {
	      int viewType = crops[ci].cropType();
	      if (crops[ci].clearView() && nvol > 0) // clearView works only with volumes
		viewType = CropObject::View_Ball;

	      if (crops[ci].clearView() && nvol > 0) // clearView works only with volumes
		{ //-- make visible from all views
		  //-- project otexCoord onto central plane defined by dirUp and dirRight vectors
		  // save otexCoord

		  shader += "otexCoord1 = otexCoord;\n";
		  Vec p0 = (pts[0]+pts[1])*0.5f;
		  shader += QString("p0 = vec3(%1, %2, %3);\n").	\
		    arg(p0.x).arg(p0.y).arg(p0.z);
		  shader += "saxis = otexCoord-p0;\n";
		  shader += "infront = dot(saxis, otexCoord-eyepos) < 0.0;\n";
		  shader += "otexCoord = p0 + dot(saxis,dirUp)*dirUp + dot(saxis,dirRight)*dirRight;\n";
		
		  shader += "viewMix = 0.0;\n";
		  shader += applyBlend((pts[0]+pts[1])*0.5f,
				       plen*0.5f,
				       saxis, taxis, pvec,
				       radX[0], radX[1],
				       radY[0], radY[1],
				       viewType,
				       crops[ci].viewMix(),
				       lift[0], lift[1]);
		  
		  shader += "viewMix1 = min(viewMix1, viewMix);\n";
		  shader += "infront = infront && viewMix > 0.0;\n";
		  shader += "isinFront = (isinFront || infront);\n";
		  // restore otexCoord
		  shader += "otexCoord = otexCoord1;\n";
		  //-----
		}

	      shader += "viewMix = 0.0;\n";
	      shader += applyBlend((pts[0]+pts[1])*0.5f,
				   plen*0.5f,
				   saxis, taxis, pvec,
				   radX[0], radX[1],
				   radY[0], radY[1],
				   viewType,
				   crops[ci].viewMix(),
				   lift[0], lift[1]);

	      if (crops[ci].clearView() && nvol>0) // clearView works only with volumes
		shader += "infrontviewMix = max(infrontviewMix, viewMix);\n";

	      if (nvol == 0)
		{
		  // do nothing
		}
	      else if (nvol == 1)
		{
		  shader += "if (sendTfSet && viewMix > 0.0)\n";
		  shader += " {\n";
		  shader += QString("   fragColor = vec4(viewMix,%1,1.0,1.0);\n").arg(crops[ci].tfset());
		  shader += "   return;\n";
		  shader += " }\n";
		  
		  shader += QString("  vgc = vec2(vg.x, vg.y+float(%1));\n"). \
		    arg(float(crops[ci].tfset())/float(Global::lutSize()));
		  shader += "  vcol = texture2D(lutTex, vgc);\n";
		  if (crops[ci].unionBlend())
		    {
		      shader += "  rgbU += vcol*viewMix;\n";
		      shader += "  alphaU = max(alphaU, viewMix*vcol.a);\n";
		    }
		  else
		    shader += "  fragColor = mix(fragColor, vcol, viewMix);\n";
		}
	      else
		{
		  for(int ni=1; ni<=nvol; ni++)
		    {
		      shader += QString("  vgc = vec2(vol%1.x, grad%1*float(%2) + float(%3));\n"). \
			                           arg(ni).arg(1.0/Global::lutSize()). \
			arg(float(crops[ci].tfset()+(ni-1))/float(Global::lutSize()));
		      shader += "  vcol = texture2D(lutTex, vgc);\n";
		      shader += QString("  color%1 = mix(color%1, vcol, viewMix);\n").arg(ni);
		    }
		}
	    }
	}

      if (nvol == 1)
	{
	  shader += "  rgbU += fragColor;\n";
	  shader += "  alphaU = max(alphaU, fragColor.a);\n";
	  shader += "  fragColor.a = alphaU;\n";
	  shader += "  fragColor.rgb = alphaU*rgbU.rgb/rgbU.a;\n";
	}

      if (clearViewPresent)
	{
	  shader += "if (isinFront && infrontviewMix < 0.01) discard;\n";
	  shader += "if (isinFront)\n";
	  if (nvol == 1)
	    {
	      shader += "  fragColor = mix(vec4(0.0,0.0,0.0,0.0), fragColor, infrontviewMix);\n";
	    }
	  else
	    {
	      shader += "{\n";
	      for(int ni=1; ni<=nvol; ni++)
		shader += QString("  color%1 = mix(vec4(0.0,0.0,0.0,0.0), color%1, infrontviewMix);\n"). \
		  arg(ni);
	      shader += "}\n";
	    }
	}
    }

  if (nvol == 1)
    shader += "if (sendTfSet) fragColor = vec4(0.0);\n";

  shader += "}\n\n";
  return shader;
}

QString
BlendShaderFactory::applyBlend(Vec p0, float plen,
			       Vec axis1, Vec axis2, Vec axis3,
			       float srad1, float srad2,
			       float trad1, float trad2,
			       int viewType,
			       float viewMix,
			       int lift1, int lift2)
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
      shader += "v0 = otexCoord-w0;\n";
      shader += "pvlen = dot(pvec, v0);\n";
      shader += "pvlen /= (2.0*plen);\n";
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

  if (viewType == CropObject::View_Tear)
    {
      shader += "  c = smoothstep(0.0, 1.0, abs(c));\n";
      shader += "  c = 0.5*(1.0-c);\n";
      shader += "  c *= (s+1.0);\n";
      shader += "  frc = abs(t);\n";
      shader += "  frc = (c - frc)/c;\n";
      shader += QString("  viewMix = smoothstep(0.0, float(%1), frc);\n").arg(viewMix);
    }
  else if (viewType == CropObject::View_Tube)
    {
      shader += "  frc = min((t2+s2), 1.0);\n";
      if (viewMix > 0.0)
	shader += QString("  viewMix = 1.0-smoothstep(float(%1), 1.0, frc);\n").arg(1.0-viewMix);
      else
	shader += "  viewMix = 1.0-smoothstep(0.95, 1.0, frc);\n";
    }
  else if (viewType == CropObject::View_Ball)
    {
      shader += "  frc = min((c2+t2+s2), 1.0);\n";
      if (viewMix > 0.0)
	shader += QString("  viewMix = 1.0-smoothstep(float(%1), 1.0, frc);\n").arg(1.0-viewMix);
      else
	shader += "  viewMix = 1.0-smoothstep(0.95, 1.0, frc);\n";
    }
  else if (viewType == CropObject::View_Block)
    {
      shader += "  frc = max(c2,max(t2,s2));\n";
      if (viewMix > 0.0)
	shader += QString("  viewMix = (1.0-smoothstep(float(%1), 1.0, frc));\n"). \
	  arg(1.0-viewMix);
      else
	shader += "  viewMix = 1.0-smoothstep(0.95, 1.0, frc);\n";
    }

  shader += "  }\n";  
  shader += "}\n";

  return shader;
}
