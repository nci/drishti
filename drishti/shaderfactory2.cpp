#include "shaderfactory.h"
#include "cropshaderfactory.h"
#include "tearshaderfactory.h"
#include "blendshaderfactory.h"
#include "shaderfactory2.h"
#include "pathshaderfactory.h"
#include "global.h"
#include "prunehandler.h"

QString
ShaderFactory2::tagVolume()
{
  QString shader;
  shader += "  float ptx;\n";
  shader += "  if (!mixTag)\n";
  shader += "    ptx = prunefeather.z;\n";
  shader += "  else\n";
  shader += "    ptx = vg.y;\n";

  //shader += "  vec4 paintColor = texture1D(paintTex, ptx);\n";
  shader += "  vec4 paintColor = texture(paintTex, ptx);\n";
  
  shader += "  ptx *= 255.0;\n";
  shader += "  if (ptx > 0.0) \n";
  shader += "  {\n";
  shader += "    paintColor.rgb *= color1.a;\n";
      // if paintColor is black then change only the transparency
  shader += "    if (paintColor.r+paintColor.g+paintColor.b > 0.01)\n";
  shader += "      color1.rgb = mix(color1.rgb, paintColor.rgb, paintColor.a);\n";
  shader += "    else\n";
  shader += "      color1 *= paintColor.a;\n";
  shader += "  }\n";
  shader += "  else\n";
  shader += "    color1 *= paintColor.a;\n";

  return shader;
}

QString
ShaderFactory2::blendVolume(int nvol)
{
  // use buffer values for blending transfer function
  QString shader;
  shader += "  float ptx = prunefeather.z;\n";
  shader += "  ptx *= 255.0;\n";
  shader += QString("  if (ptx > 0.0 && ptx < float(%1)) \n").	\
    arg(float(Global::lutSize()));
  shader += "  {\n";
  for(int ni=1; ni<=nvol; ni++)
    {
      if (ni==1) shader += "  vec2 vgc;\n";
      shader += QString("  vgc = vec2(vol%1.x, grad%1*float(%2) + (ptx+float(%3))/float(%4));\n"). \
	arg(ni).arg(1.0/Global::lutSize()).				\
	arg(ni-1).arg(float(Global::lutSize()));
      shader += QString("  color%1 = texture2D(lutTex, vgc);\n").arg(ni);
    }
  shader += "  }\n";

  return shader;
}

QString
ShaderFactory2::getNormal(int nvol)
{
  QString shader;

  QString xyzw;
  if (nvol == 2) xyzw = "xw";
  else if (nvol == 3) xyzw = "xyz";
  else if (nvol == 4) xyzw = "xyzw";

  QString vstr;
  if (nvol == 2) vstr = "  vec2";
  else if (nvol == 3) vstr = "  vec3";
  else if (nvol == 4) vstr = "  vec4";

  shader += "  vec2 k = vec2(1.0, -1.0);\n";
  shader += vstr + " g1 = getVal(vtexCoord+k.xyy)[0]."+xyzw+";\n";
  shader += vstr + " g2 = getVal(vtexCoord+k.yyx)[0]."+xyzw+";\n";
  shader += vstr + " g3 = getVal(vtexCoord+k.yxy)[0]."+xyzw+";\n";
  shader += vstr + " g4 = getVal(vtexCoord+k.xxx)[0]."+xyzw+";\n";

  for(int i=1; i<=nvol; i++)
    {
      QString c;
      if (i == 1) c = "x";
      else if (i == 2) c = "y";
      else if (i == 3) c = "z";
      else if (i == 4) c = "w";
      
      shader += QString("  vec3 normal%1 = (k.xyy * g1."+c+" + k.yyx * g2."+c+" + k.yxy * g3."+c+" + k.xxx * g4."+c+");\n").arg(i);
      shader += QString("  normal%1 /= 2.0;\n").arg(i);
      shader += QString("  float grad%1 = length(normal%1);\n").arg(i);
      shader += QString("  grad%1 = clamp(grad%1, 0.0, 0.996);\n").arg(i);
    }

  return shader;
}

//QString
//ShaderFactory2::getNormal(int nvol)
//{
//  QString shader;
//
//  QString xyzw;
//  if (nvol == 2) xyzw = "xw";
//  else if (nvol == 3) xyzw = "xyz";
//  else if (nvol == 4) xyzw = "xyzw";
//
//  QString vstr;
//  if (nvol == 2) vstr = "  vec2";
//  else if (nvol == 3) vstr = "  vec3";
//  else if (nvol == 4) vstr = "  vec4";
//
//  shader += vstr + " samplex1, samplex2;\n";
//  shader += vstr + " sampley1, sampley2;\n";
//  shader += vstr + " samplez1, samplez2;\n";
//  shader += vstr + " tv;\n";
//
//  shader += "  samplex1 = texture2DRect(dataTex, t0+vec2(1,0))."+xyzw+";\n";
//  shader += "        tv = texture2DRect(dataTex, t1+vec2(1,0))."+xyzw+";\n";
//  shader += "  samplex1 = mix(samplex1, tv, slicef);\n";
//
//  shader += "  samplex2 = texture2DRect(dataTex, t0-vec2(1,0))."+xyzw+";\n";
//  shader += "        tv = texture2DRect(dataTex, t1-vec2(1,0))."+xyzw+";\n";
//  shader += "  samplex2 = mix(samplex2, tv, slicef);\n";
//
//  shader += "  sampley1 = texture2DRect(dataTex, t0+vec2(0,1))."+xyzw+";\n";
//  shader += "        tv = texture2DRect(dataTex, t1+vec2(0,1))."+xyzw+";\n";
//  shader += "  sampley1 = mix(sampley1, tv, slicef);\n";
//
//  shader += "  sampley2 = texture2DRect(dataTex, t0-vec2(0,1))."+xyzw+";\n";
//  shader += "        tv = texture2DRect(dataTex, t1-vec2(0,1))."+xyzw+";\n";
//  shader += "  sampley2 = mix(sampley2, tv, slicef);\n";
//
//
//  shader += "  t1 = getTextureCoordinate(slice+2, gridx, tsizex, tsizey, texCoord.xy);";
//  shader += "  tv = (texture2DRect(dataTex, t1))."+xyzw+";\n";
//  shader += "  samplez1 = mix(val1, tv, slicef);\n";
//
//  shader += "  t1 = getTextureCoordinate(slice-1, gridx, tsizex, tsizey, texCoord.xy);\n";
//  shader += "  samplez2 = (texture2DRect(dataTex, t1))."+xyzw+";\n";
//  shader += "  samplez2 = mix(samplez2, val0, slicef);\n";
//      
//  shader += "  vec3 sample1, sample2;\n";
// 
//  for(int i=1; i<=nvol; i++)
//    {
//      QString c;
//      if (i == 1) c = "x";
//      else if (i == 2) c = "y";
//      else if (i == 3) c = "z";
//      else if (i == 4) c = "w";
//
//      shader += QString("  sample1 = vec3(samplex1.%1, sampley1.%1, samplez1.%1);\n").arg(c);
//      shader += QString("  sample2 = vec3(samplex2.%1, sampley2.%1, samplez2.%1);\n").arg(c);
//      shader += QString("  vec3 normal%1 = (sample1-sample2);\n").arg(i);
//      shader += QString("  float grad%1 = length(normal%1);\n").arg(i);
//      shader += QString("  grad%1 = clamp(grad%1, 0.0, 0.996);\n").arg(i);
//    }
//
//  return shader;
//}

QString
ShaderFactory2::addLighting(int nvol)
{
  QString shader;

  for (int i=1; i<=nvol; i++)
    {
      shader += QString("  if (grad%1 > 0.0)\n").arg(i);
      shader += QString("     normal%1 = normalize(normal%1);\n").arg(i);
      shader += "  else\n";
      shader += QString("     normal%1 = vec3(0.0,0.0,0.0);\n").arg(i);
    }

  shader += "  vec3 lightvec = voxpos - lightpos;\n";
  shader += "  lightvec = normalize(lightvec);\n";

  shader += " vec3 reflecvec, Diff, Amb;\n";
  shader += " float DiffMag, Spec;\n";
  shader += "  float litfrac;\n";
  shader += "  float normlen;\n";

  for(int i=1; i<=nvol; i++)
    {
      shader += QString("  reflecvec = reflect(lightvec, normal%1);\n").arg(i);
      shader += QString("  DiffMag = abs(dot(normal%1, lightvec));\n").arg(i);
      shader += QString("  Diff = diffuse*DiffMag*color%1.rgb;\n").arg(i);
      shader += QString("  Amb = ambient*color%1.rgb;\n").arg(i);
      shader += QString("  litfrac = smoothstep(0.05, 0.1, grad%1);\n").arg(i);
      shader += QString("  color%1.rgb = mix(color%1.rgb, Amb, litfrac);\n").arg(i);
      shader += "  if (litfrac > 0.0)\n";
      shader += "   {\n";
      shader += QString("     vec3 frgb = color%1.rgb + litfrac*(Diff);\n").arg(i);
      shader += "     if (any(greaterThan(frgb,vec3(1.0,1.0,1.0)))) \n";
      shader += "        frgb = vec3(1.0,1.0,1.0);\n";
      shader += QString("     if (any(greaterThan(frgb,color%1.aaa))) \n").arg(i);  
      shader += QString("        frgb = color%1.aaa;\n").arg(i);
      shader += QString("     color%1.rgb = frgb;\n").arg(i);
      //shader += QString("     vec3 spec = shadingSpecularGGX(normal%1, I, lightvec, speccoeff*0.2, color%1.rgb);\n").arg(i);
      //shader += QString("     color%1.rgb += specular*spec;\n").arg(i);
      shader += "    float specCoeff = 512.0/mix(0.1, 64.0, speccoeff*speccoeff);\n";
      shader += QString("     float shine = specular * pow(abs(dot(normal%1, I)), specCoeff);\n").arg(i);
      shader += QString("     color%1 = mix(color%1, pow(color%1, vec4(1.0-shine)), vec4(shine));\n").arg(i);
      shader += QString("     color%1 = clamp(color%1, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n").arg(i);
      shader += "   }\n";
      shader += QString("  color%1.rgb *= lightcol;\n").arg(i);
    }

  return shader;
}

QString
ShaderFactory2::mixColorOpacity(int nvol,
				int mixvol, bool mixColor, bool mixOpacity)
{
  QString shader;

  shader += "  float alpha = 0.0;\n";
  shader += "  float totalpha = 0.0;\n";
  shader += "  vec3 rgb = vec3(0.0,0.0,0.0);\n";

  if (!mixColor && !mixOpacity)
    {
      for(int i=1; i<=nvol; i++)
	{
	  shader += QString("  rgb += color%1.rgb;\n").arg(i);
	  shader += QString("  totalpha += color%1.a;\n").arg(i);
	  shader += QString("  alpha = max(alpha, color%1.a);\n").arg(i);
	}
      
      shader += "  glFragColor.a = alpha;\n";
      shader += "  glFragColor.rgb = alpha*rgb/totalpha;\n";
    }
  else
    {
      //      shader += "  rgb = color1.rgb;\n";
      //      shader += "  totalpha = color1.a;\n";
      //      shader += "  alpha = color1.a;\n";

      shader += QString("  rgb = color%1.rgb;\n").arg(mixvol);
      shader += QString("  totalpha = color%1.a;\n").arg(mixvol);
      shader += QString("  alpha = color%1.a;\n").arg(mixvol);

      if (mixColor && mixOpacity)
	{
	  for(int i=mixvol+1; i<=nvol; i++)
	    {	  
	      shader += QString("  rgb = mix(rgb, color%1.rgb, color%1.a);\n").arg(i);
	      shader += QString("  rgb *= color%1.a;\n").arg(i);
	      shader += QString("  totalpha *= color%1.a;\n").arg(i);
	      shader += QString("  alpha *= color%1.a;\n").arg(i);
	    }
	}      
      else if (mixColor)
	{
	  for(int i=mixvol+1; i<=nvol; i++)
	    {	  
	      shader += QString("  rgb = mix(rgb, color%1.rgb, color%1.a);\n").arg(i);
	    }
	}
      else if (mixOpacity)
	{
	  for(int i=mixvol+1; i<=nvol; i++)
	    {	  
	      shader += QString("  rgb *= color%1.a;\n").arg(i);
	      shader += QString("  totalpha *= color%1.a;\n").arg(i);
	      shader += QString("  alpha *= color%1.a;\n").arg(i);
	    }
	}

      for(int i=1; i<mixvol; i++)
	{
	  shader += QString("  rgb += color%1.rgb;\n").arg(i);
	  shader += QString("  totalpha += color%1.a;\n").arg(i);
	  shader += QString("  alpha = max(alpha, color%1.a);\n").arg(i);
	}

      shader += "  glFragColor.a = alpha;\n";
      shader += "  glFragColor.rgb = alpha*rgb/totalpha;\n";
    }

  return shader;
}

QString
ShaderFactory2::genVgx(int nvol)
{
  QString shader;

  QString vstr, xyzw;
  if (nvol == 2)
    {
      xyzw = "xw";
      vstr = "  vec2";
    }
  else if (nvol == 3)
    {
      xyzw = "xyz";
      vstr = "  vec3";
    }
  else if (nvol == 4)
    {
      xyzw = "xyzw";
      vstr = "  vec4";
    }

  shader += "  vec4 voxValues[3] = getVal(vtexCoord);\n"; // interpolated

  shader += "  if (linearInterpolation)\n";
  shader += "    vg."+xyzw+" = voxValues[0]."+xyzw+";\n"; // interpolated
  shader += "  else\n";
  shader += "    vg."+xyzw+" = voxValues[1]."+xyzw+";\n"; // nearest neighbour

  if (nvol == 2)
    shader += "    vg.y = vg.w;\n";

  shader += "  if (mixTag) \n";
  if (nvol == 2)
    shader += "    vg.y = voxValues[1].w;\n";
  else
    shader += "    vg.y = voxValues[1].y;\n";

  return shader;
}

QString
ShaderFactory2::genDefaultSliceShaderString(bool lighting,
					    bool emissive,
					    QList<CropObject> crops,
					    int nvol,
					    bool peel, int peelType,
					    float peelMin, float peelMax, float peelMix,
					    int mixvol, bool mixColor, bool mixOpacity,
					    int interpolateVolumes,
					    bool amrData)
{
  bool cropPresent = false;
  bool tearPresent = false;
  bool viewPresent = false;
  for(int i=0; i<crops.count(); i++)
    if (crops[i].cropType() < CropObject::Tear_Tear)
      cropPresent = true;
    else if (crops[i].cropType() < CropObject::View_Tear)
      tearPresent = true;
    else
      viewPresent = true;

  for(int i=0; i<crops.count(); i++)
    if (crops[i].cropType() >= CropObject::View_Tear &&
	crops[i].cropType() <= CropObject::View_Block &&
	crops[i].magnify() > 1.0)
      tearPresent = true;

  bool pathCropPresent = PathShaderFactory::cropPresent();
  bool pathViewPresent = PathShaderFactory::blendPresent();

  float lastSet[4];

  for(int i=1; i<=nvol; i++)
    {
      float idx = nvol-i+1;
      lastSet[i-1] = (Global::lutSize()-idx)/Global::lutSize();
    }

  QString shader;

  shader = "#version 450 core\n";
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "in vec3 pointpos;\n";
  shader += "in vec3 glTexCoord0;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect amrTex;\n";
  shader += "uniform sampler2DArray dataTexAT;\n";

  shader += "uniform sampler1D paintTex;\n";

  shader += "uniform float tfSet;\n";
  shader += "uniform vec3 delta;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 lightpos;\n";
  shader += "uniform float ambient;\n";
  shader += "uniform float diffuse;\n";
  shader += "uniform float specular;\n";
  shader += "uniform float speccoeff;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int tsizex;\n";
  shader += "uniform int tsizey;\n";
  shader += "uniform float depthcue;\n";

  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform int lightgridx;\n";
  shader += "uniform int lightgridy;\n";
  shader += "uniform int lightgridz;\n";
  shader += "uniform int lightnrows;\n";
  shader += "uniform int lightncols;\n";
  shader += "uniform int lightlod;\n";

  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int prunegridx;\n";
  shader += "uniform int prunetsizex;\n";
  shader += "uniform int prunetsizey;\n";
  shader += "uniform float prunelod;\n";
  shader += "uniform int zoffset;\n";

  shader += "uniform vec2 dataMin;\n";
  shader += "uniform vec2 dataSize;\n";
  shader += "uniform int tminz;\n";

  shader += "uniform int lod;\n";
  shader += "uniform vec3 dirFront;\n";
  shader += "uniform vec3 dirUp;\n";
  shader += "uniform vec3 dirRight;\n";
      
  shader += "uniform float interpVol;\n";
  shader += "uniform bool mixTag;\n";

  shader += "uniform vec3 brickMin;\n";
  shader += "uniform vec3 brickMax;\n";

  shader += "uniform int shdlod;\n";
  shader += "uniform sampler2DRect shdTex;\n";
  shader += "uniform float shdIntensity;\n";

  shader += "uniform float opmod;\n";
  shader += "uniform bool linearInterpolation;\n";

  shader += "uniform float dofscale;\n";

  shader += "uniform vec3 vsize;\n";
  shader += "uniform vec3 vmin;\n";

  shader += "uniform int nclip;\n";
  shader += "uniform vec3 clipPos[10];\n";
  shader += "uniform vec3 clipNormal[10];\n";

  shader += "uniform float gamma;\n";
  
  shader += "out vec4 glFragColor;\n";

  shader += ShaderFactory::genTextureCoordinate();

  //---------------------
  // get voxel value from array texture
  shader += ShaderFactory::getVal();
  shader += ShaderFactory::ggxShader();
  //---------------------

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops, nvol);
  if (pathViewPresent) shader += PathShaderFactory::applyPathBlend(nvol);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();

  shader += "void main(void)\n";
  shader += "{\n";
  
  shader += "  vec3 lightcol = vec3(1.0,1.0,1.0);\n";

  shader += "  vec3 texCoord = glTexCoord0.xyz;\n";

  shader += "if (any(lessThan(texCoord,brickMin)) || ";
  shader += "any(greaterThan(texCoord, brickMax)))\n";
  shader += "  discard;\n";

  //-----------------
  // apply clipping
  shader += " if (nclip > 0)\n";
  shader += "  {\n";
  shader += "    for(int c=0; c<nclip; c++)\n";
  shader += "      {\n";
  shader += "        vec3 cpos = clipPos[c];\n";
  shader += "        vec3 cnorm = clipNormal[c];\n";
  shader += "        if (dot(cnorm,(pointpos-cpos)) <= 0.0) discard;\n";
  shader += "      }\n";
  shader += "  }\n";
  //-----------------


  if (crops.count() > 0)
    {
      shader += "  vec3 otexCoord = texCoord;\n";
      shader += "  float feather = 1.0;\n";
    }
  else
    {
      if (pathCropPresent)
	shader += "  float feather = 1.0;\n";
      if (pathViewPresent)
	shader += "  vec3 otexCoord = texCoord;\n";
    }

  if (tearPresent)
    {
      shader += "vec4 tcf = dissect(texCoord);\n";
      shader += "texCoord = tcf.xyz;\n";
      shader += "feather *= tcf.w;\n";
    }
  if (cropPresent) shader += "feather *= crop(texCoord, true);\n";
  if (pathCropPresent) shader += "feather *= pathcrop(texCoord, true);\n";

  //--------------------------
  // light texture coordinates
  shader += "vec3 ltexCoord = (texCoord-vmin)/lod;\n";
  shader += "ltexCoord.xy = vec2(tsizex,tsizey)*(ltexCoord.xy/vsize.xy);\n";
  //--------------------------

  if (amrData)
    {
      shader += "  float amrX = texture2DRect(amrTex, vec2(glTexCoord0.x,0)).x;\n";
      shader += "  float amrY = texture2DRect(amrTex, vec2(glTexCoord0.y,0)).y;\n";
      shader += "  float amrZ = texture2DRect(amrTex, vec2(glTexCoord0.z,0)).z;\n";
      
      shader += "  texCoord = vec3(amrX, amrY, amrZ);\n";
    }

  //------
  shader += "vec3 vtexCoord = (texCoord-vmin)/lod;\n";
  // for nearest neighbour interpolation
  //shader += "if (!linearInterpolation || mixTag)\n";
  shader += "if (!linearInterpolation)\n";
  shader += "{\n";
  shader += "  vtexCoord = vec3(floor(vtexCoord)+vec3(0.5));\n";
  shader += "}\n";
  //------

  shader += "texCoord.xy = vec2(tsizex,tsizey)*(vtexCoord.xy/vsize.xy);\n";
  shader += "texCoord.z = vtexCoord.z;\n";

  shader += ShaderFactory::genPreVgx();  
  shader += genVgx(nvol);

  //----------------------------------
  shader += "if (lightlod > 0)\n";
  shader += " {\n"; // calculate light color
  shader += "   float llod = prunelod*float(lightlod);\n";
  shader += "   vec2 pvg = ltexCoord.xy/llod;\n";
  shader += "   int lbZslc = int((zoffset+ltexCoord.z)/llod);\n";
  shader += "   float lbZslcf = fract((zoffset+ltexCoord.z)/llod);\n";
  shader += "   vec2 pvg0 = getTextureCoordinate(lbZslc, ";
  shader += "                 lightncols, lightgridx, lightgridy, pvg);\n";
  shader += "   vec2 pvg1 = getTextureCoordinate(lbZslc+1, ";
  shader += "                 lightncols, lightgridx, lightgridy, pvg);\n";	       
  shader += "   vec3 lc0 = texture2DRect(lightTex, pvg0).xyz;\n";
  shader += "   vec3 lc1 = texture2DRect(lightTex, pvg1).xyz;\n";
  shader += "   lightcol = mix(lc0, lc1, lbZslcf);\n";	       
  shader += "   lightcol = 1.0-pow((vec3(1,1,1)-lightcol),vec3(lod,lod,lod));\n";
  shader += " }\n";
  shader += "else\n";
  shader += " lightcol = vec3(1.0,1.0,1.0);\n";

  shader += "if (shdlod > 0)\n";
  shader += "  {\n";
  shader += "     float sa = texture2DRect(shdTex, gl_FragCoord.xy*vec2(dofscale)/vec2(shdlod)).a;\n";
  shader += "     sa = 1.0-smoothstep(0.0, shdIntensity, sa);\n";
  shader += "     sa = clamp(0.1, sa, 1.0);\n";
  shader += "     lightcol *= sa;\n";
  shader += "  }\n";
  //----------------------------------

  if (peel || lighting || !Global::use1D())
    shader += getNormal(nvol);

  if (interpolateVolumes == 2 && nvol == 2)
    {
      shader += "  vg.x = mix(vg.x, vg.y, interpVol);\n";
      shader += "  grad1 = mix(grad1, grad2, interpVol);\n";
      shader += "  normal1 = mix(normal1, normal2, interpVol);\n";
      shader += "  vg.y = vg.x;\n";
      shader += "  grad2 = grad1;\n";
      shader += "  normal2 = normal1;\n";
    }

  shader += "  int h0, h1;\n";

  for(int i=1; i<=nvol; i++)
    {
      QString c;
      if (i == 1) c = "x";
      else if (i == 2) c = "y";
      else if (i == 3) c = "z";
      else if (i == 4) c = "w";
      
      if (Global::use1D())
	shader += QString("  vec2 vol%1 = vec2(vg.%2, 0.0);\n").arg(i).arg(c);
      else
	shader += QString("  vec2 vol%1 = vec2(vg.%2, grad%1/float(%3));\n").	\
	  arg(i).arg(c).arg(Global::lutSize());
      
      if (emissive)
	shader += QString("  vec2 emis%1 = vol%1;\n").arg(i);
      
      // interpolate values between the volumes
      if (interpolateVolumes == 2 && nvol == 2)
	shader += QString("  vol%1.y += tfSet;\n").arg(i);
      else
	shader += QString("  vol%1.y += tfSet+float(%2);\n").		\
	  arg(i).arg((i-1)*1.0/Global::lutSize());
    }

  for(int i=1; i<=nvol; i++)
    shader += QString("  vec4 color%1 = texture2D(lutTex, vol%1);\n").arg(i);



  if (Global::emptySpaceSkip())
    {
      if (PruneHandler::blend())
	shader += blendVolume(nvol);
      else
	shader += tagVolume();
    }

  // incidence vector moved here from addLighting
  shader += "  vec3 voxpos = pointpos;\n";
  shader += "  vec3 I = voxpos - eyepos;\n";
  shader += "  I = normalize(I);\n";
  //------------------
  
  //------------------
  if (peel)
    {
      if (!lighting) 
	{
	  shader += QString("  if (grad1 > 0.0)\n");
	  shader += QString("     normal1 = normalize(normal1);\n");
	  shader += "  else\n";
	  shader += QString("     normal1 = vec3(0.0,0.0,0.0);\n");

	  shader += "  normal1 = mix(vec3(0.0,0.0,0.0), normal1, step(0.0, grad1));"; 
	  //shader += "  vec3 voxpos = pointpos;\n";
	  //shader += "  vec3 I = voxpos - eyepos;\n";
	  //shader += "  I = normalize(I);\n";
	}
      shader += "  float IdotN = dot(I, normal1);\n";

	  if(nvol == 2) 
	  {
	  	  shader += QString("  vec2 valpeel = vec2(0.0, 0.0);\n");
	  	  shader += QString("  vec2 one = vec2(1.0, 1.0);\n");
	  	  shader += QString("  vec2 vIdotN = vec2(IdotN, IdotN);\n");	  	  
		  shader += QString("  vec2 peelMix = vec2(float(%1), float(%1));\n").arg(peelMix);
		  shader += QString("  vec2 peelMin = vec2(float(%1), float(%1));\n").arg(peelMin);
		  shader += QString("  vec2 peelMinSub = vec2(float(%1), float(%1));\n").arg(peelMin-0.1);
		  shader += QString("  vec2 peelMax = vec2(float(%1), float(%1));\n").arg(peelMax);
		  shader += QString("  vec2 peelMaxPlus = vec2(float(%1), float(%1));\n").arg(peelMax+0.1);
	  }
	  else if(nvol == 3)
	  {
	  	  shader += QString("  vec3 valpeel = vec3(0.0, 0.0, 0.0);\n");
	  	  shader += QString("  vec3 one = vec3(1.0, 1.0, 1.0);\n");
	  	  shader += QString("  vec3 vIdotN = vec3(IdotN, IdotN, IdotN);\n");	  	  
		  shader += QString("  vec3 peelMix = vec3(float(%1), float(%1), float(%1));\n").arg(peelMix);
		  shader += QString("  vec3 peelMin = vec3(float(%1), float(%1), float(%1));\n").arg(peelMin);
		  shader += QString("  vec3 peelMinSub = vec3(float(%1), float(%1), float(%1));\n").arg(peelMin-0.1);
		  shader += QString("  vec3 peelMax = vec3(float(%1), float(%1), float(%1));\n").arg(peelMax);
		  shader += QString("  vec3 peelMaxPlus = vec3(float(%1), float(%1), float(%1));\n").arg(peelMax+0.1);
	  }
	  else if(nvol == 4)
	  {
	  	  shader += QString("  vec4 valpeel = vec4(0.0, 0.0, 0.0, 0.0);\n");
	  	  shader += QString("  vec4 one = vec4(1.0, 1.0, 1.0, 1.0);\n");
	  	  shader += QString("  vec4 vIdotN = vec4(IdotN, IdotN, IdotN, IdotN);\n");
		  shader += QString("  vec4 peelMix = vec4(float(%1), float(%1), float(%1), float(%1));\n").arg(peelMix);
		  shader += QString("  vec4 peelMin = vec4(float(%1), float(%1), float(%1), float(%1));\n").arg(peelMin);
		  shader += QString("  vec4 peelMinSub = vec4(float(%1), float(%1), float(%1), float(%1));\n").arg(peelMin-0.1);
		  shader += QString("  vec4 peelMax = vec4(float(%1), float(%1), float(%1), float(%1));\n").arg(peelMax);
		  shader += QString("  vec4 peelMaxPlus = vec4(float(%1), float(%1), float(%1), float(%1));\n").arg(peelMax+0.1);	  
	  }

      if (peelType == 0)
	{
	  if (peelMin < peelMax)
	    shader += QString("  valpeel = smoothstep(peelMin, peelMax, vIdotN);\n");
	  else
	    shader += QString("  valpeel = smoothstep(peelMax, peelMin, -vIdotN);\n");
	}
      else if (peelType == 1)
	{ //---- keep inside
	  shader += QString("  valpeel = one-smoothstep(peelMinSub, peelMin, vIdotN);\n");
	  shader += QString("  valpeel *= smoothstep(peelMax, peelMaxPlus, vIdotN);\n");
	}

      shader += QString("  vIdotN = mix(valpeel, one, peelMix);\n").arg(peelMix);

	  for(int i=1; i<=nvol; i++)
		{

		  QString c;
		  if (i == 1) c = "x";
		  else if (i == 2) c = "y";
		  else if (i == 3) c = "z";
		  else if (i == 4) c = "w";
      
		  shader += QString("  color%2.rgba *= vIdotN.").arg(i);
		  shader += c + ";\n";
		}
    }
  //------------------


  if (viewPresent)
    {
      if (nvol == 2) shader += "blend(otexCoord, vol1, vol2, grad1, grad2, color1, color2);\n";
      if (nvol == 3) shader += "blend(otexCoord, vol1, vol2, vol3, grad1, grad2, grad3, color1, color2, color3);\n";
      if (nvol == 4) shader += "blend(otexCoord, vol1, vol2, vol3, vol4, grad1, grad2, grad3, grad4, color1, color2, color3, color4);\n";
    }
  if (pathViewPresent)
    {
      if (nvol == 2) shader += "pathblend(otexCoord, vol1, vol2, grad1, grad2, color1, color2);\n";
      if (nvol == 3) shader += "pathblend(otexCoord, vol1, vol2, vol3, grad1, grad2, grad3, color1, color2, color3);\n";
      if (nvol == 4) shader += "pathblend(otexCoord, vol1, vol2, vol3, vol4, grad1, grad2, grad3, grad4, color1, color2, color3, color4);\n";
    }

  if (lighting)
    shader += addLighting(nvol);
  
  if (interpolateVolumes > 0 && nvol == 2)
    {
      if (interpolateVolumes == 2) // interpolate values between the volumes
	shader += "  glFragColor = color1;\n";
      else                         // interpolate colors between the volumes
	shader += "  glFragColor = mix(color1, color2, interpVol);\n";
    }
  else
    shader += mixColorOpacity(nvol, mixvol, mixColor, mixOpacity);

  if (Global::emptySpaceSkip())
    shader += "  glFragColor.rgba = mix(vec4(0.0,0.0,0.0,0.0), glFragColor.rgba, prunefeather.x);\n";

  // -- apply feather
  if (tearPresent || cropPresent || pathCropPresent)
    shader += "  glFragColor.rgba = mix(glFragColor.rgba, vec4(0.0,0.0,0.0,0.0), feather);\n";


  //---------------------------------
  if (Global::emptySpaceSkip())
    {
      shader += "if (delta.x > 1.0)\n";
      shader += "{\n";
      shader += "  float v1 = vol1.x*step(0.001, color1.a);\n";
      shader += "  float v2 = vol2.x*step(0.001, color2.a);\n";
      shader += "  float r = v1;\n";
      shader += "  float a1 = color1.a;\n";
      shader += "  float a2 = color2.a;\n";
      shader += "  float a = a1;\n";
      shader += "  if (delta.x < 2.0) { r = max(0.0,v1-v2); a = max(0.0,a1-a2); }\n"; // A-B
      shader += "  else if (delta.x < 3.0) { r = abs(v1-v2); a = abs(a1-a2); }\n"; // ||A-B||
      shader += "  else if (delta.x < 4.0) { r = min(1.0,v1+v2); a = min(1.0,a1+a2); }\n"; // A+B
      shader += "  else if (delta.x < 5.0) { r = max(v1,v2); a = max(a1,a2); }\n"; // max(A,B)
      shader += "  else if (delta.x < 6.0) { r = min(v1,v2); a = min(a1,a2); }\n"; // min(A,B)
      shader += "  else if (delta.x < 7.0) { r = min(1.0,v1/v2); a = min(1.0,a1/a2); }\n"; // A/B
      shader += "  else if (delta.x < 8.0) { r = v1*v2; a = a1*a2; }\n"; // A*B
      shader += "  r *= step(0.001,glFragColor.a);\n";
      shader += "  glFragColor = vec4(r, a, prunefeather.z, 1.0);\n";
      shader += "  return;\n";
      shader += "}\n";
    }
  else
    {
      shader += "if (delta.x > 1.0)\n";
      shader += "{\n";
      shader += "  float v1 = vol1.x*step(0.001, color1.a);\n";
      shader += "  float v2 = vol2.x*step(0.001, color2.a);\n";
      shader += "  float r = v1;\n";
      shader += "  float a1 = color1.a;\n";
      shader += "  float a2 = color2.a;\n";
      shader += "  float a = a1;\n";
      shader += "  if (delta.x < 2.0) { r = max(0.0,v1-v2); a = max(0.0,a1-a2); }\n"; // A-B
      shader += "  else if (delta.x < 3.0) { r = abs(v1-v2); a = abs(a1-a2); }\n"; // ||A-B||
      shader += "  else if (delta.x < 4.0) { r = min(1.0,v1+v2); a = min(1.0,a1+a2); }\n"; // A+B
      shader += "  else if (delta.x < 5.0) { r = max(v1,v2); a = max(a1,a2); }\n"; // max(A,B)
      shader += "  else if (delta.x < 6.0) { r = min(v1,v2); a = min(a1,a2); }\n"; // min(A,B)
      shader += "  else if (delta.x < 7.0) { r = min(1.0,v1/v2); a = min(1.0,a1/a2); }\n"; // A/B
      shader += "  else if (delta.x < 8.0) { r = v1*v2; a = a1*a2; }\n"; // A*B
      shader += "  r *= step(0.001,glFragColor.a);\n";
      shader += "  glFragColor = vec4(r, a, 0.0, 1.0);\n";
      shader += "  return;\n";
      shader += "}\n";
    }
//---------------------------------

//------------------------------------
  shader += "glFragColor = 1.0-pow((vec4(1,1,1,1)-glFragColor),";
  shader += "vec4(lod,lod,lod,lod));\n";
//------------------------------------

  shader += "  if (glFragColor.a < 0.005)\n";
  shader += "	discard;\n";
  
  shader += "\n";

  if (emissive)
    {
      for(int i=1; i<=nvol; i++)
	{
	  shader += QString("  emis%1.y += %2;\n").arg(i).arg(lastSet[i-1]);
	  shader += QString("  glFragColor.rgb += step(0.01, color%1.a)*texture2D(lutTex, emis%1).rgb;\n").arg(i);
	}
    }

  // -- depth cueing
  shader += "  glFragColor.rgb *= depthcue;\n";

  shader += "  glFragColor *= opmod;\n";


  shader += "  glFragColor = clamp(glFragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";

  shader += "  glFragColor.rgb = pow(glFragColor.rgb, vec3(gamma));\n";

  shader += "}\n";

  return shader;
}
