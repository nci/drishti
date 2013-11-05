#include "global.h"
#include "pathshaderfactory.h"
#include "geometryobjects.h"

bool
PathShaderFactory::cropPresent()
{
  int pc = GeometryObjects::paths()->count();
  if (pc == 0) return false;

  QList<PathObject> paths = GeometryObjects::paths()->paths();
  for (int i=0; i<paths.count(); i++)
    if (paths[i].crop()) return true;

  return false;
}

bool
PathShaderFactory::blendPresent()
{
  int pc = GeometryObjects::paths()->count();
  if (pc == 0) return false;

  QList<PathObject> paths = GeometryObjects::paths()->paths();
  for (int i=0; i<paths.count(); i++)
    if (paths[i].blend()) return true;

  return false;
}

QString
PathShaderFactory::applyPathCrop()
{
  int pc = GeometryObjects::paths()->count();
  if (pc == 0)
    return QString();

  QList<PathObject> paths1 = GeometryObjects::paths()->paths();
  QList<PathObject> paths;
  for (int i=0; i<paths1.count(); i++)
    {
      if (paths1[i].crop())
	paths << paths1[i];
    }
  paths1.clear();

  if (paths.count() == 0)
    return QString();

  QString shader;

  shader += "float pathcrop(vec3 texCoord, bool reject)\n";
  shader += "{\n";
  shader += "float feather = 1.0;\n";
  shader += "float myfeather, op;\n";
  shader += "vec3 p0[20];\n";
  shader += "vec3 tgP[20];\n";
  shader += "vec3 tgPu0, tgPu1;\n";
  shader += "vec3 saxis[20];\n";
  shader += "vec3 taxis[20];\n";
  shader += "float rad[20];\n";
  shader += "int npts;\n";
  shader += "bool keepInside = false;\n";

  for (int i=0; i<paths.count(); i++)
    {
      QList<Vec> points = paths[i].points();
      QList<Vec> tgP = paths[i].tangents();
      QList<Vec> saxis = paths[i].saxis();
      QList<Vec> taxis = paths[i].taxis();
      QList<float> rad = paths[i].radX();
      int npt = qMin(20, points.count());

      shader += QString(" npts = %1;\n").arg(npt);

      shader += QString(" keepInside = bool(%1);\n").arg(paths[i].keepInside());

      for (int j=0; j<npt; j++)
	shader += QString(" p0[%1] = vec3(%2,%3,%4);\n").arg(j).	\
	  arg(points[j].x).arg(points[j].y).arg(points[j].z);

      for (int j=0; j<npt; j++)
	shader += QString(" tgP[%1] = vec3(%2,%3,%4);\n").arg(j).	\
	  arg(tgP[j].x).arg(tgP[j].y).arg(tgP[j].z);

      Vec t = tgP[0].unit();
      shader += QString(" tgPu0 = vec3(%1,%2,%3);\n").	\
	arg(t.x).arg(t.y).arg(t.z);
      t = tgP[npt-1].unit();
      shader += QString(" tgPu1 = vec3(%1,%2,%3);\n").	\
	arg(t.x).arg(t.y).arg(t.z);


      for (int j=0; j<npt; j++)
	shader += QString(" saxis[%1] = vec3(%2,%3,%4);\n").arg(j).	\
	  arg(saxis[j].x).arg(saxis[j].y).arg(saxis[j].z);

      for (int j=0; j<npt; j++)
	shader += QString(" taxis[%1] = vec3(%2,%3,%4);\n").arg(j).	\
	  arg(taxis[j].x).arg(taxis[j].y).arg(taxis[j].z);

      for (int j=0; j<npt; j++)
	shader += QString(" rad[%1] = float(%2);\n").arg(j).arg(rad[j]);

      shader += "{\n";

      // find nearest point
      shader += " int ptn = 0;\n";
      shader += " float d = dot((texCoord-p0[0]),(texCoord-p0[0]));\n";
      shader += " vec3 proj = p0[0];\n";
      shader += " vec3 xaxis = saxis[0];\n";
      shader += " vec3 yaxis = taxis[0];\n";
      shader += " float rd = rad[0];\n";
      shader += " for(int i=0; i<npts; i++)\n";
      shader += "  {\n";
      shader += "    vec3 v0 = texCoord - p0[i];\n";
      shader += "    float dd = dot(v0,v0);\n";
      shader += "    if (dd < d)\n";
      shader += "     {\n";
      shader += "       ptn = i;\n";
      shader += "       d = dd;\n";
      shader += "       proj = p0[i];\n";
      shader += "       xaxis = saxis[i];\n";
      shader += "       yaxis = taxis[i];\n";
      shader += "       rd = rad[i];\n";
      shader += "     }\n";
      shader += "    vec3 tang, tp0, tp1;\n";
      shader += "    float ltang;\n";
      shader += "    int i0, i1;\n";
      shader += "    if (i < npts-1)\n";
      shader += "      { i0 = i; i1 = i+1; }\n";
      shader += "    else\n";
      shader += "      { i0 = i-1; i1 = i; }\n";
      shader += "    tang = p0[i1]-p0[i0];\n";
      shader += "    ltang = length(tang);\n";
      shader += "    tang = normalize(tang);\n";
      shader += "    tp0 = texCoord - p0[i0];\n";
      shader += "    float d0 = dot(tp0, tang);\n";
      shader += "    if (d0 >= 0.0 && d0 <= ltang)\n";
      shader += "      {\n";
      shader += "        float frc = d0/ltang;\n";
      shader += "        vec3 diff = p0[i1] - p0[i0];\n";
      shader += "        vec3 pp = p0[i0];\n";
      shader += "        float len = length(diff);\n";
      shader += "        if (len > 0.1)\n";
      shader += "         {\n";
      shader += "           vec3 v1 = 3.0*diff - 2.0*tgP[i0] - tgP[i1];\n";
      shader += "           vec3 v2 = -2.0*diff + tgP[i0] + tgP[i1];\n";      
      shader += "           pp += frc*(tgP[i0] + frc*(v1+frc*v2));\n";
      shader += "         }\n";
      shader += "        float dd = dot((texCoord-pp),(texCoord-pp));\n";
      shader += "        if (dd < d)\n";
      shader += "         {\n";
      shader += "           ptn = -1;\n";
      shader += "           d = dd;\n";
      shader += "           proj = pp;\n";
      shader += "           xaxis = mix(saxis[i0], saxis[i1], frc);\n";
      shader += "           yaxis = mix(taxis[i0], taxis[i1], frc);\n";
      shader += "           rd = mix(rad[i0], rad[i1], frc);\n";
      shader += "         }\n";
      shader += "      }\n";
      shader += "  }\n";


      if (paths[i].useType() == 1)
	shader += "  op = smoothstep(max(0.0,rd-3.0), rd, distance(texCoord, proj));\n";
      else
	{
	  shader += "  float a = dot((texCoord-proj),xaxis);\n";
	  if (paths[i].useType() == 2)
	    {
	      // mystery : why is this slow !!!!
	      //shader += "  op = smoothstep(0.0, 3.0, a);\n";

	      // this is faster ?????
	      shader += "  op = smoothstep(max(0.0,rd-300.0), 0.0, abs(a));\n";
	      shader += "  float b = dot((texCoord-proj),yaxis);\n";
	      shader += "  if (b < 0.0)\n";
	      shader += "    op *= smoothstep(-3.0, 0.0, b);\n";
	    }
	  else
	    {
	      shader += "  op = smoothstep(max(0.0,rd-3.0), rd, abs(a));\n";
	      if (paths[i].useType() == 4)
		{
		  shader += "  float b = dot((texCoord-proj),yaxis);\n";
		  shader += "  if (b < 0.0)\n";
		  shader += "    op *= smoothstep(-3.0, 0.0, b);\n";
		}
	    }
	}

      shader += "  myfeather = op;\n";
      shader += " if (keepInside) myfeather = 1.0-myfeather;\n";

      if (!paths[i].keepEnds())
	{
	  shader += " if (ptn == 0)\n";
	  shader += "   myfeather *= smoothstep(-3.0, 0.0, dot(tgPu0, texCoord-proj));\n";

	  shader += " if (ptn == npts-1)\n";
	  shader += "   myfeather *= (1.0-smoothstep(0.0, 3.0, dot(tgPu1, texCoord-proj)));\n";
	}
      else
	{
	  shader += " if (ptn == 0)\n";
	  shader += "   myfeather = max(myfeather, (1.0-smoothstep(-3.0, 0.0, dot(tgPu0, texCoord-proj))));\n";
	  shader += " if (ptn == npts-1)\n";
	  shader += "   myfeather = max(myfeather, smoothstep(0.0, 3.0, dot(tgPu1, texCoord-proj)));\n";
	}

      if (paths.count() == 1)
	{
	  shader += " if (myfeather <= 0.0)\n";
	  shader += "  {\n";
	  shader += "    if (reject) discard;\n";
	  shader += "    else return 1.0;\n";
	  shader += "  }\n";
	}
      // otherwise have union

      shader += " feather *= (1.0 - myfeather);\n";
      shader += "}\n";
    }

  shader += "return feather;\n";
  shader += "}\n";

  return shader;
}

QString
PathShaderFactory::applyPathBlend(int nvol)
{
  int pc = GeometryObjects::paths()->count();
  if (pc == 0)
    return QString();

  QList<PathObject> paths1 = GeometryObjects::paths()->paths();
  QList<PathObject> paths;
  for (int i=0; i<paths1.count(); i++)
    {
      if (paths1[i].blend())
	paths << paths1[i];
    }
  paths1.clear();

  if (paths.count() == 0)
    return QString();

  QString shader;

  if (nvol < 2) shader +=  "void pathblend(vec3 otexCoord, vec2 vg, inout vec4 fragColor)\n";
  if (nvol == 2) shader += "void pathblend(vec3 otexCoord, vec2 vol1, vec2 vol2, float grad1, float grad2, inout vec4 color1, inout vec4 color2)\n";
  if (nvol == 3) shader += "void pathblend(vec3 otexCoord, vec2 vol1, vec2 vol2, vec2 vol3, float grad1, float grad2, float grad3, inout vec4 color1, inout vec4 color2, inout vec4 color3)\n";
  if (nvol == 4) shader += "void pathblend(vec3 otexCoord, vec2 vol1, vec2 vol2, vec2 vol3, vec2 vol4, float grad1, float grad2, float grad3, float grad4, inout vec4 color1, inout vec4 color2, inout vec4 color3, inout vec4 color4)\n";
  shader += "{\n";
  shader += "float viewMix;\n";
  shader += "vec2 vgc;\n";
  shader += "vec4 vcol;\n";
  shader += "float op;\n";
  shader += "vec3 p0[20];\n";
  shader += "vec3 tgP[20];\n";
  shader += "vec3 tgPu0, tgPu1;\n";
  shader += "vec3 saxis[20];\n";
  shader += "vec3 taxis[20];\n";
  shader += "float rad[20];\n";
  shader += "int npts;\n";
  shader += "bool keepInside = false;\n";

  for (int i=0; i<paths.count(); i++)
    {
      QList<Vec> points = paths[i].points();
      QList<Vec> tgP = paths[i].tangents();
      QList<Vec> saxis = paths[i].saxis();
      QList<Vec> taxis = paths[i].taxis();
      QList<float> rad = paths[i].radX();
      int npt = qMin(20, points.count());

      shader += QString(" npts = %1;\n").arg(npt);

      shader += QString(" keepInside = bool(%1);\n").arg(paths[i].keepInside());

      for (int j=0; j<npt; j++)
	shader += QString(" p0[%1] = vec3(%2,%3,%4);\n").arg(j).	\
	  arg(points[j].x).arg(points[j].y).arg(points[j].z);

      for (int j=0; j<npt; j++)
	shader += QString(" tgP[%1] = vec3(%2,%3,%4);\n").arg(j).	\
	  arg(tgP[j].x).arg(tgP[j].y).arg(tgP[j].z);

      Vec t = tgP[0].unit();
      shader += QString(" tgPu0 = vec3(%1,%2,%3);\n").	\
	arg(t.x).arg(t.y).arg(t.z);
      t = tgP[npt-1].unit();
      shader += QString(" tgPu1 = vec3(%1,%2,%3);\n").	\
	arg(t.x).arg(t.y).arg(t.z);

      for (int j=0; j<npt; j++)
	shader += QString(" saxis[%1] = vec3(%2,%3,%4);\n").arg(j).	\
	  arg(saxis[j].x).arg(saxis[j].y).arg(saxis[j].z);

      for (int j=0; j<npt; j++)
	shader += QString(" taxis[%1] = vec3(%2,%3,%4);\n").arg(j).	\
	  arg(taxis[j].x).arg(taxis[j].y).arg(taxis[j].z);

      for (int j=0; j<npt; j++)
	shader += QString(" rad[%1] = float(%2);\n").arg(j).arg(rad[j]);

      shader += "{\n";

      // find nearest point
      shader += " int ptn = 0;\n";
      shader += " float d = dot((otexCoord-p0[0]),(otexCoord-p0[0]));\n";
      shader += " vec3 proj = p0[0];\n";
      shader += " vec3 xaxis = saxis[0];\n";
      shader += " vec3 yaxis = taxis[0];\n";
      shader += " float rd = rad[0];\n";
      shader += " for(int i=0; i<npts; i++)\n";
      shader += "  {\n";
      shader += "    vec3 v0 = otexCoord - p0[i];\n";
      shader += "    float dd = dot(v0,v0);\n";
      shader += "    if (dd < d)\n";
      shader += "     {\n";
      shader += "       ptn = i;\n";
      shader += "       d = dd;\n";
      shader += "       proj = p0[i];\n";
      shader += "       xaxis = saxis[i];\n";
      shader += "       yaxis = taxis[i];\n";
      shader += "       rd = rad[i];\n";
      shader += "     }\n";
      shader += "    vec3 tang, tp0, tp1;\n";
      shader += "    float ltang;\n";
      shader += "    int i0, i1;\n";
      shader += "    if (i < npts-1)\n";
      shader += "      { i0 = i; i1 = i+1; }\n";
      shader += "    else\n";
      shader += "      { i0 = i-1; i1 = i; }\n";
      shader += "    tang = p0[i1]-p0[i0];\n";
      shader += "    ltang = length(tang);\n";
      shader += "    tang = normalize(tang);\n";
      shader += "    tp0 = otexCoord - p0[i0];\n";
      shader += "    float d0 = dot(tp0, tang);\n";
      shader += "    if (d0 >= 0.0 && d0 <= ltang)\n";
      shader += "      {\n";
      shader += "        float frc = d0/ltang;\n";
      shader += "        vec3 diff = p0[i1] - p0[i0];\n";
      shader += "        vec3 pp = p0[i0];\n";
      shader += "        float len = length(diff);\n";
      shader += "        if (len > 0.1)\n";
      shader += "         {\n";
      shader += "           vec3 v1 = 3.0*diff - 2.0*tgP[i0] - tgP[i1];\n";
      shader += "           vec3 v2 = -2.0*diff + tgP[i0] + tgP[i1];\n";      
      shader += "           pp += frc*(tgP[i0] + frc*(v1+frc*v2));\n";
      shader += "         }\n";
      shader += "        float dd = dot((otexCoord-pp),(otexCoord-pp));\n";
      shader += "        if (dd < d)\n";
      shader += "         {\n";
      shader += "           ptn = -1;\n";
      shader += "           d = dd;\n";
      shader += "           proj = pp;\n";
      shader += "           xaxis = mix(saxis[i0], saxis[i1], frc);\n";
      shader += "           yaxis = mix(taxis[i0], taxis[i1], frc);\n";
      shader += "           rd = mix(rad[i0], rad[i1], frc);\n";
      shader += "         }\n";
      shader += "      }\n";
      shader += "  }\n";


      if (paths[i].useType() == 5)
	shader += "  op = smoothstep(max(0.0,rd-3.0), rd, distance(otexCoord, proj));\n";
      else
	{
	  shader += "  float a = dot((otexCoord-proj),xaxis);\n";
	  if (paths[i].useType() == 6)
	    {
	      // don't know why this is slow !!!!
	      //shader += "  op = smoothstep(0.0, 3.0, a);\n";

	      // this is faster ?????
	      shader += "  op = smoothstep(max(0.0,rd-300.0), 0.0, abs(a));\n";
	      shader += "  float b = dot((otexCoord-proj),yaxis);\n";
	      shader += "  if (b < 0.0)\n";
	      shader += "    op *= smoothstep(-3.0, 0.0, b);\n";
	    }
	  else
	    {
	      shader += "  op = smoothstep(max(0.0,rd-3.0), rd, abs(a));\n";
	      if (paths[i].useType() == 8)
		{
		  shader += "  float b = dot((otexCoord-proj),yaxis);\n";
		  shader += "  if (b < 0.0)\n";
		  shader += "    op *= smoothstep(-3.0, 0.0, b);\n";
		}
	    }
	}

      shader += " viewMix = 1.0-op;\n";
      shader += " if (keepInside) viewMix = 1.0-viewMix;\n";

      if (!paths[i].keepEnds())
	{
	  shader += " if (ptn == 0)\n";
	  shader += "   viewMix = max(viewMix, (1.0-smoothstep(-3.0, 0.0, dot(tgPu0, otexCoord-proj))));\n";

	  shader += " if (ptn == npts-1)\n";
	  shader += "   viewMix = max(viewMix, smoothstep(0.0, 3.0, dot(tgPu1, otexCoord-proj)));\n";
	}
      else
	{
	  shader += " if (ptn == 0)\n";
	  shader += "   viewMix = min(viewMix, smoothstep(-3.0, 0.0, dot(tgPu0, otexCoord-proj)));\n";
	  shader += " if (ptn == npts-1)\n";
	  shader += "   viewMix = min(viewMix, 1.0-smoothstep(0.0, 3.0, dot(tgPu1, otexCoord-proj)));\n";
	}


      if (nvol == 0)
	{
	  // do nothing
	}
      else if (nvol == 1)
	{
	  shader += QString("  vgc = vec2(vg.x, vg.y+float(%1));\n").	\
	    arg(float(paths[i].blendTF())/float(Global::lutSize()));
	  shader += "  vcol = texture2D(lutTex, vgc);\n";
	  shader += "  fragColor = mix(fragColor, vcol, viewMix);\n";
	}
      else
	{
	  for(int ni=1; ni<=nvol; ni++)
	    {
	      shader += QString("  vgc = vec2(vol%1.x, grad%1*float(%2) + %3);\n"). \
		arg(ni).arg(1.0/Global::lutSize()).			\
		arg(float(paths[i].blendTF()+(ni-1))/float(Global::lutSize()));
	      shader += "  vcol = texture2D(lutTex, vgc);\n";
	      shader += QString("  color%1 = mix(color%1, vcol, viewMix);\n").arg(ni);
	    }
	}
      
      shader += "}\n";

    }
  shader += "} //-------------------------\n";

  return shader;
}
