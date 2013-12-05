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

  shader += "  vec4 paintColor = texture1D(paintTex, ptx);\n";
  
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

  shader += vstr + " samplex1, samplex2;\n";
  shader += vstr + " sampley1, sampley2;\n";
  shader += vstr + " samplez1, samplez2;\n";
  shader += vstr + " tv;\n";

  shader += "  samplex1 = texture2DRect(dataTex, t0+vec2(1,0))."+xyzw+";\n";
  shader += "        tv = texture2DRect(dataTex, t1+vec2(1,0))."+xyzw+";\n";
  shader += "  samplex1 = mix(samplex1, tv, slicef);\n";

  shader += "  samplex2 = texture2DRect(dataTex, t0-vec2(1,0))."+xyzw+";\n";
  shader += "        tv = texture2DRect(dataTex, t1-vec2(1,0))."+xyzw+";\n";
  shader += "  samplex2 = mix(samplex2, tv, slicef);\n";

  shader += "  sampley1 = texture2DRect(dataTex, t0+vec2(0,1))."+xyzw+";\n";
  shader += "        tv = texture2DRect(dataTex, t1+vec2(0,1))."+xyzw+";\n";
  shader += "  sampley1 = mix(sampley1, tv, slicef);\n";

  shader += "  sampley2 = texture2DRect(dataTex, t0-vec2(0,1))."+xyzw+";\n";
  shader += "        tv = texture2DRect(dataTex, t1-vec2(0,1))."+xyzw+";\n";
  shader += "  sampley2 = mix(sampley2, tv, slicef);\n";


  shader += "  t1 = getTextureCoordinate(slice+2, gridx, tsizex, tsizey, texCoord.xy);";
  shader += "  tv = (texture2DRect(dataTex, t1))."+xyzw+";\n";
  shader += "  samplez1 = mix(val1, tv, slicef);\n";

  shader += "  t1 = getTextureCoordinate(slice-1, gridx, tsizex, tsizey, texCoord.xy);\n";
  shader += "  samplez2 = (texture2DRect(dataTex, t1))."+xyzw+";\n";
  shader += "  samplez2 = mix(samplez2, val0, slicef);\n";
      
  shader += "  vec3 sample1, sample2;\n";
 
  for(int i=1; i<=nvol; i++)
    {
      QString c;
      if (i == 1) c = "x";
      else if (i == 2) c = "y";
      else if (i == 3) c = "z";
      else if (i == 4) c = "w";

      shader += QString("  sample1 = vec3(samplex1.%1, sampley1.%1, samplez1.%1);\n").arg(c);
      shader += QString("  sample2 = vec3(samplex2.%1, sampley2.%1, samplez2.%1);\n").arg(c);
      shader += QString("  vec3 normal%1 = (sample1-sample2);\n").arg(i);
      shader += QString("  float grad%1 = length(normal%1);\n").arg(i);
      shader += QString("  grad%1 = clamp(grad%1, 0.0, 0.996);\n").arg(i);
    }

  return shader;
}
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

  //shader += "  vec3 lightcol = vec3(1.0,1.0,1.0);\n";
  shader += "  vec3 voxpos = pointpos;\n";
  shader += "  vec3 I = voxpos - eyepos;\n";
  shader += "  vec3 lightvec = voxpos - lightpos;\n";
  shader += "  I = normalize(I);\n";
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
      shader += QString("  Spec = pow(abs(dot(normal%1, reflecvec)), speccoeff);\n").arg(i);
      shader += QString("  Spec *= specular*color%1.a;\n").arg(i);
      shader += QString("  Amb = ambient*color%1.rgb;\n").arg(i);
      shader += QString("  litfrac = smoothstep(0.05, 0.1, grad%1);\n").arg(i);
      shader += QString("  color%1.rgb = mix(color%1.rgb, Amb, litfrac);\n").arg(i);
      shader += "  if (litfrac > 0.005)\n";
      //shader += QString("     color%1.rgb += litfrac*(Diff + Spec);\n").arg(i);
      shader += "  if (litfrac > 0.0)\n";
      shader += "   {\n";
      shader += QString("     vec3 frgb = color%1.rgb + litfrac*(Diff + Spec);\n").arg(i);
      shader += "     if (any(greaterThan(frgb,vec3(1.0,1.0,1.0)))) \n";
      shader += "        frgb = vec3(1.0,1.0,1.0);\n";
      shader += QString("     if (any(greaterThan(frgb,color%1.aaa))) \n").arg(i);  
      shader += QString("        frgb = color%1.aaa;\n").arg(i);
      shader += QString("     color%1.rgb = frgb;\n").arg(i);
      shader += "   }\n";
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
      
      shader += "  gl_FragColor.a = alpha;\n";
      shader += "  gl_FragColor.rgb = alpha*rgb/totalpha;\n";
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

      shader += "  gl_FragColor.a = alpha;\n";
      shader += "  gl_FragColor.rgb = alpha*rgb/totalpha;\n";
    }

  return shader;
}

QString
ShaderFactory2::genVgx(int nvol)
{
  QString shader;

  shader += "  int row, col, slice;\n";
  shader += "  vec2 t0, t1;\n";
  shader += "  float zcoord, slicef;\n";
  shader += "  zcoord = max(1.0, texCoord.z);\n";
  shader += "  slice = int(floor(zcoord));\n";
  shader += "  slicef = fract(zcoord);\n";

  //---------------------------------------------------------------------
  if (Global::emptySpaceSkip())
    {      
      shader += "  vec2 pvg = texCoord.xy / prunelod;\n";

      shader += "  vec2 pvg0 = getTextureCoordinate(int(float(zoffset+slice)/prunelod), ";
      shader += "              prunegridx, prunetsizex, prunetsizey, pvg);\n";
      shader += "  vec4 pf0 = texture2DRect(pruneTex, pvg0);\n";
      
      shader += "  vec2 pvg1 = getTextureCoordinate(int(float(zoffset+slice+1)/prunelod), ";
      shader += "              prunegridx, prunetsizex, prunetsizey, pvg);\n";
      shader += "  vec4 pf1 = texture2DRect(pruneTex, pvg1);\n";

      shader += "  pf0 = mix(pf0, pf1, slicef);\n";
      shader += "  vec4 prunefeather = pf0;\n";
      shader += "  if ( prunefeather.x < 0.005) discard;\n";

      // tag values should be non interpolated - nearest neighbour
      shader += "  prunefeather.z = texture2DRect(pruneTex, vec2(floor(pvg0.x)+0.5,floor(pvg0.y)+0.5)).z;\n";
    }
  //---------------------------------------------------------------------


  shader += "  t0 = getTextureCoordinate(slice, gridx, tsizex, tsizey, texCoord.xy);\n";
  shader += "  t1 = getTextureCoordinate(slice+1, gridx, tsizex, tsizey, texCoord.xy);\n";

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

  shader += vstr + "  vg;\n";
  shader += vstr + "  val0, val1;\n";

  shader += "  val0 = (texture2DRect(dataTex, t0))."+xyzw+";\n";
  shader += "  val1 = (texture2DRect(dataTex, t1))."+xyzw+";\n";
  shader += "  vg = mix(val0, val1, slicef);\n";

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
					    int interpolateVolumes)
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

  bool pathCropPresent = PathShaderFactory::cropPresent();
  bool pathViewPresent = PathShaderFactory::blendPresent();

  float lastSet[4];

  for(int i=1; i<=nvol; i++)
    {
      float idx = nvol-i+1;
      lastSet[i-1] = (Global::lutSize()-idx)/Global::lutSize();
    }

  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointpos;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect dataTex;\n";

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

  shader += ShaderFactory::genTextureCoordinate();

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops, nvol);
  if (pathViewPresent) shader += PathShaderFactory::applyPathBlend(nvol);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();

  shader += "void main(void)\n";
  shader += "{\n";
  
  shader += "  vec3 lightcol = vec3(1.0,1.0,1.0);\n";

  shader += "  vec3 texCoord = gl_TexCoord[0].xyz;\n";

  shader += "if (any(lessThan(texCoord,brickMin)) || any(greaterThan(texCoord, brickMax)))\n";
  shader += "  discard;\n";

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


  shader += "texCoord.x = 1.0 + float(tsizex-2)*(texCoord.x-dataMin.x)/dataSize.x;\n";
  shader += "texCoord.y = 1.0 + float(tsizey-2)*(texCoord.y-dataMin.y)/dataSize.y;\n";
  shader += "texCoord.z = 1.0 + (texCoord.z-float(tminz))/float(lod);\n";

  shader += genVgx(nvol);

  //----------------------------------
  shader += " if (lightlod > 0)\n";
  shader += "   {\n"; // calculate light color
  shader += "     vec3 lc;\n";
  shader += "     vec2 pvg = texCoord.xy / prunelod;\n";
  shader += "     pvg /= vec2(lightlod,lightlod);\n";
  shader += "     vec2 pvg0 = getTextureCoordinate(int(float(zoffset+slice)/prunelod)/lightlod, ";
  shader += "            lightncols, lightgridx, lightgridy, pvg);\n";
  shader += "     vec2 pvg1 = getTextureCoordinate(int(float(zoffset+slice+1)/prunelod)/lightlod, ";
  shader += "            lightncols, lightgridx, lightgridy, pvg);\n";
  shader += "     vec3 lc0 = texture2DRect(lightTex, pvg0).xyz;\n";
  shader += "     vec3 lc1 = texture2DRect(lightTex, pvg1).xyz;\n";
  shader += "     lightcol = mix(lc0, lc1, slicef);\n";
  shader += "     lightcol = 1.0-pow((vec3(1,1,1)-lightcol),vec3(lod,lod,lod));\n";
  shader += "   }\n";
  shader += " else\n";
  shader += "   lightcol = vec3(1.0,1.0,1.0);\n";
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



  if (lighting)
    shader += addLighting(nvol);

  //------------------
  if (peel && peelType!=2)
    {
      if (!lighting) 
	{
	  shader += QString("  if (grad1 > 0.0)\n");
	  shader += QString("     normal1 = normalize(normal1);\n");
	  shader += "  else\n";
	  shader += QString("     normal1 = vec3(0.0,0.0,0.0);\n");

	  shader += "  normal1 = mix(vec3(0.0,0.0,0.0), normal1, step(0.0, grad1));"; 
	  shader += "  vec3 voxpos = pointpos;\n";
	  shader += "  vec3 I = voxpos - eyepos;\n";
	  shader += "  I = normalize(I);\n";
	}
      shader += "  float IdotN = dot(I, normal1);\n";

	  if(nvol == 2) 
	  {
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
	    shader += QString("  val1 = smoothstep(peelMin, peelMax, vIdotN);\n");
	  else
	    shader += QString("  val1 = smoothstep(peelMax, peelMin, -vIdotN);\n");
	}
      else if (peelType == 1)
	{ //---- keep inside
	  shader += QString("  val1 = one-smoothstep(peelMinSub, peelMin, vIdotN);\n");
	  shader += QString("  val1 *= smoothstep(peelMax, peelMaxPlus, vIdotN);\n");
	}
//      else
//	{ //---- keep outside
//	  shader += QString("  val1 = smoothstep(peelMinSub, peelMin, vIdotN);\n");
//	  shader += QString("  val1 = max(val1, (one - smoothstep(peelMax, peelMaxPlus, vIdotN)));\n");
//	}

      shader += QString("  vIdotN = mix(val1, one, peelMix);\n").arg(peelMix);

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

  if (Global::emptySpaceSkip())
    {
      if (PruneHandler::blend())
	shader += blendVolume(nvol);
      else
	shader += tagVolume();
    }
  
  if (interpolateVolumes > 0 && nvol == 2)
    {
      if (interpolateVolumes == 2) // interpolate values between the volumes
	shader += "  gl_FragColor = color1;\n";
      else                         // interpolate colors between the volumes
	shader += "  gl_FragColor = mix(color1, color2, interpVol);\n";
    }
  else
    shader += mixColorOpacity(nvol, mixvol, mixColor, mixOpacity);

  if (Global::emptySpaceSkip())
    shader += "  gl_FragColor.rgba = mix(vec4(0.0,0.0,0.0,0.0), gl_FragColor.rgba, prunefeather.x);\n";

  // -- apply feather
  if (tearPresent || cropPresent || pathCropPresent)
    shader += "  gl_FragColor.rgba = mix(gl_FragColor.rgba, vec4(0.0,0.0,0.0,0.0), feather);\n";


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
      shader += "  r *= step(0.001,gl_FragColor.a);\n";
      shader += "  gl_FragColor = vec4(r, a, prunefeather.z, 1.0);\n";
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
      shader += "  r *= step(0.001,gl_FragColor.a);\n";
      shader += "  gl_FragColor = vec4(r, a, 0.0, 1.0);\n";
      shader += "  return;\n";
      shader += "}\n";
    }
//---------------------------------

//------------------------------------
  shader += "gl_FragColor = 1.0-pow((vec4(1,1,1,1)-gl_FragColor),";
  shader += "vec4(lod,lod,lod,lod));\n";
//------------------------------------

  shader += "  if (gl_FragColor.a < 0.005)\n";
  shader += "	discard;\n";
  
  shader += "\n";

  if (emissive)
    {
      for(int i=1; i<=nvol; i++)
	{
	  shader += QString("  emis%1.y += %2;\n").arg(i).arg(lastSet[i-1]);
	  shader += QString("  gl_FragColor.rgb += step(0.01, color%1.a)*texture2D(lutTex, emis%1).rgb;\n").arg(i);
	}
    }

  // -- depth cueing
  shader += "  gl_FragColor.rgb *= depthcue;\n";

  shader += "}\n";

  return shader;
}

QString
ShaderFactory2::genHighQualitySliceShaderString(bool lighting,
						bool emissive,
						bool shadows,
						QList<CropObject> crops,
						int nvol,
						bool peel, int peelType,
						float peelMin, float peelMax, float peelMix,
						int mixvol, bool mixColor, bool mixOpacity,
						int interpolateVolumes)
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

  bool pathCropPresent = PathShaderFactory::cropPresent();
  bool pathViewPresent = PathShaderFactory::blendPresent();

  float lastSet[4];
  for(int i=1; i<=nvol; i++)
    {
      float idx = nvol-i+1;
      lastSet[i-1] = (Global::lutSize()-idx)/Global::lutSize();
    }

  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointpos;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect dataTex;\n";
  shader += "uniform sampler2DRect shadowTex;\n";

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
  shader += "\n";

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

  shader += ShaderFactory::genTextureCoordinate();

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops, nvol);
  if (pathViewPresent) shader += PathShaderFactory::applyPathBlend(nvol);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec3 lightcol = vec3(1.0,1.0,1.0);\n";

  shader += "  vec3 texCoord = gl_TexCoord[0].xyz;\n";

  shader += "if (any(lessThan(texCoord,brickMin)) || any(greaterThan(texCoord, brickMax)))\n";
  shader += "  discard;\n";

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

  shader += "texCoord.x = 1.0 + float(tsizex-2)*(texCoord.x-dataMin.x)/dataSize.x;\n";
  shader += "texCoord.y = 1.0 + float(tsizey-2)*(texCoord.y-dataMin.y)/dataSize.y;\n";
  shader += "texCoord.z = 1.0 + (texCoord.z-float(tminz))/float(lod);\n";

  shader += genVgx(nvol);

  //----------------------------------
  shader += " if (lightlod > 0)\n";
  shader += "   {\n"; // calculate light color
  shader += "     vec3 lc;\n";
  shader += "     vec2 pvg = texCoord.xy / prunelod;\n";
  shader += "     pvg /= vec2(lightlod,lightlod);\n";
  shader += "     vec2 pvg0 = getTextureCoordinate(int(float(zoffset+slice)/prunelod)/lightlod, ";
  shader += "            lightncols, lightgridx, lightgridy, pvg);\n";
  shader += "     vec2 pvg1 = getTextureCoordinate(int(float(zoffset+slice+1)/prunelod)/lightlod, ";
  shader += "            lightncols, lightgridx, lightgridy, pvg);\n";
  shader += "     vec3 lc0 = texture2DRect(lightTex, pvg0).xyz;\n";
  shader += "     vec3 lc1 = texture2DRect(lightTex, pvg1).xyz;\n";
  shader += "     lightcol = mix(lc0, lc1, slicef);\n";
  shader += "     lightcol = 1.0-pow((vec3(1,1,1)-lightcol),vec3(lod,lod,lod));\n";
  shader += "   }\n";
  shader += " else\n";
  shader += "   lightcol = vec3(1.0,1.0,1.0);\n";
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
	shader += QString("  vec2 vol%1 = vec2(vg.%2, grad%1*float(%3));\n").	\
	                                    arg(i).arg(c).arg(1.0/Global::lutSize());

      if (emissive)
	shader += QString("  vec2 emis%1 = vol%1;\n").arg(i);

      // interpolate values between the volumes
      if (interpolateVolumes == 2 && nvol == 2)
	shader += QString("  vol%1.y += tfSet;\n").arg(i);
      else
	shader += QString("  vol%1.y += tfSet+float(%2);\n").\
	  arg(i).arg((i-1)*1.0/Global::lutSize());
    }

  for(int i=1; i<=nvol; i++)
    shader += QString("  vec4 color%1 = texture2D(lutTex, vol%1);\n").arg(i);


  if (lighting)
    shader += addLighting(nvol);

  //------------------
  if (peel && peelType!=2)
    {
      if (!lighting) 
	{
	  shader += QString("  if (grad1 > 0.0)\n");
	  shader += QString("     normal1 = normalize(normal1);\n");
	  shader += "  else\n";
	  shader += QString("     normal1 = vec3(0.0,0.0,0.0);\n");

	  shader += "  normal1 = mix(vec3(0.0,0.0,0.0), normal1, step(0.0, grad1));"; 
	  shader += "  vec3 voxpos = pointpos;\n";
	  shader += "  vec3 I = voxpos - eyepos;\n";
	  shader += "  I = normalize(I);\n";
	}
      shader += "  float IdotN = dot(I, normal1);\n";

	  if(nvol == 2) 
	  {
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
	    shader += QString("  val1 = smoothstep(peelMin, peelMax, vIdotN);\n");
	  else
	    shader += QString("  val1 = smoothstep(peelMax, peelMin, -vIdotN);\n");
	}
      else if (peelType == 1)
	{ //---- keep inside
	  shader += QString("  val1 = one-smoothstep(peelMinSub, peelMin, vIdotN);\n");
	  shader += QString("  val1 *= smoothstep(peelMax, peelMaxPlus, vIdotN);\n");
	}
//      else
//	{ //---- keep outside
//	  shader += QString("  val1 = smoothstep(peelMinSub, peelMin, vIdotN);\n");
//	  shader += QString("  val1 = max(val1, (one - smoothstep(peelMax, peelMaxPlus, vIdotN)));\n");
//	}

      shader += QString("  vIdotN = mix(val1, one, peelMix);\n").arg(peelMix);

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


  if (Global::emptySpaceSkip())
    {
      if (PruneHandler::blend())
	shader += blendVolume(nvol);
      else
	shader += tagVolume();
    }

  if (interpolateVolumes > 0 && nvol == 2)
    {
      if (interpolateVolumes == 2) // interpolate values between the volumes
	shader += "  gl_FragColor = color1;\n";
      else                         // interpolate colors between the volumes
	shader += "  gl_FragColor = mix(color1, color2, interpVol);\n";
    }
  else
    shader += mixColorOpacity(nvol, mixvol, mixColor, mixOpacity);


  if (Global::emptySpaceSkip())
    shader += "  gl_FragColor.rgba = mix(vec4(0.0,0.0,0.0,0.0), gl_FragColor.rgba, prunefeather.x);\n";

  // -- apply feather
  if (tearPresent || cropPresent || pathCropPresent)
    shader += "  gl_FragColor.rgba = mix(gl_FragColor.rgba, vec4(0.0,0.0,0.0,0.0), feather);\n";

  if (shadows && peel && peelType == 2)
    shader += "  gl_FragColor.rgba *= peelfeather;\n";

//------------------------------------
  shader += "gl_FragColor = 1.0-pow((vec4(1,1,1,1)-gl_FragColor),";
  shader += "vec4(lod,lod,lod,lod));\n";
//------------------------------------

  shader += "  if (gl_FragColor.a < 0.005)\n";
  shader += "	discard;\n";
  
  shader += "\n";

  if (shadows)
    {
      //----------------------
      // shadows
      shader += "  vec4 incident_light = texture2DRect(shadowTex, gl_TexCoord[1].xy);\n";
      shader += "  float maxval = 1.0 - incident_light.a;\n";
      shader += "  gl_FragColor.rgb *= maxval*(incident_light.rgb + maxval);\n";
      //----------------------
    }

  if (emissive)
    {
      for(int i=1; i<=nvol; i++)
	{
	  shader += QString("  emis%1.y += %2;\n").arg(i).arg(lastSet[i-1]);
	  shader += QString("  gl_FragColor.rgb += step(0.01, color%1.a)*texture2D(lutTex, emis%1).rgb;\n").arg(i);
	}
    }

  // -- depth cueing
  shader += "  gl_FragColor.rgb *= depthcue;\n";

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";
  
  shader += "}\n";

  return shader;
}
 
QString
ShaderFactory2::genSliceShadowShaderString(float shadowintensity,
					   float r, float g, float b,
					   QList<CropObject> crops,
					   int nvol,
					   bool peel, int peelType,
					   float peelMin, float peelMax, float peelMix,
					   int mixvol, bool mixColor, bool mixOpacity,
					   int interpolateVolumes)
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

  bool pathCropPresent = PathShaderFactory::cropPresent();
  bool pathViewPresent = PathShaderFactory::blendPresent();

  QString shader;
  float maxrgb = qMax(r, qMax(g, b));
  if (maxrgb > 1)
    maxrgb = 1.0/maxrgb;
  else
    maxrgb = 1.0;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointpos;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect dataTex;\n";

  shader += "uniform sampler1D paintTex;\n";

  shader += "uniform float tfSet;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int tsizex;\n";
  shader += "uniform int tsizey;\n";
  shader += "\n";

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

  shader += ShaderFactory::genTextureCoordinate();

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops, nvol);
  if (pathViewPresent) shader += PathShaderFactory::applyPathBlend(nvol);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec3 texCoord = gl_TexCoord[0].xyz;\n";

  shader += "if (any(lessThan(texCoord,brickMin)) || any(greaterThan(texCoord, brickMax)))\n";
  shader += "  discard;\n";

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

  shader += "texCoord.x = 1.0 + float(tsizex-2)*(texCoord.x-dataMin.x)/dataSize.x;\n";
  shader += "texCoord.y = 1.0 + float(tsizey-2)*(texCoord.y-dataMin.y)/dataSize.y;\n";
  shader += "texCoord.z = 1.0 + (texCoord.z-float(tminz))/float(lod);\n";

  shader += genVgx(nvol);

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
	shader += QString("  vec2 vol%1 = vec2(vg.%2, grad%1*float(%3));\n"). \
	                                    arg(i).arg(c).arg(1.0/Global::lutSize());
					    
      // interpolate values between the volumes
      if (interpolateVolumes == 2 && nvol == 2)
	shader += QString("  vol%1.y += tfSet;\n").arg(i);
      else
	shader += QString("  vol%1.y += tfSet+float(%2);\n").		\
	  arg(i).arg((i-1)*1.0/Global::lutSize());
    }

  for(int i=1; i<=nvol; i++)
    shader += QString("  vec4 color%1 = texture2D(lutTex, vol%1);\n").arg(i);


  //------------------
  if (peel && peelType!=2)
    {

      shader += QString("  if (grad1 > 0.0)\n");
      shader += QString("     normal1 = normalize(normal1);\n");
      shader += "  else\n";
      shader += QString("     normal1 = vec3(0.0,0.0,0.0);\n");

      shader += "  normal1 = mix(vec3(0.0,0.0,0.0), normal1, step(0.0, grad1));"; 
      shader += "  vec3 voxpos = pointpos;\n";
      shader += "  vec3 I = voxpos - eyepos;\n";
      shader += "  I = normalize(I);\n";
      shader += "  float IdotN = dot(I, normal1);\n";

	  if(nvol == 2) 
	  {
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
	    shader += QString("  val1 = smoothstep(peelMin, peelMax, vIdotN);\n");
	  else
	    shader += QString("  val1 = smoothstep(peelMax, peelMin, -vIdotN);\n");
	}
      else if (peelType == 1)
	{ //---- keep inside
	  shader += QString("  val1 = one-smoothstep(peelMinSub, peelMin, vIdotN);\n");
	  shader += QString("  val1 *= smoothstep(peelMax, peelMaxPlus, vIdotN);\n");
	}
//      else
//	{ //---- keep outside
//	  shader += QString("  val1 = smoothstep(peelMinSub, peelMin, vIdotN);\n");
//	  shader += QString("  val1 = max(val1, (one - smoothstep(peelMax, peelMaxPlus, vIdotN)));\n");
//	}

      shader += QString("  vIdotN = mix(val1, one, peelMix);\n").arg(peelMix);

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


  if (Global::emptySpaceSkip())
    {
      if (PruneHandler::blend())
	shader += blendVolume(nvol);
      else
	shader += tagVolume();
    }

  // --- modulate shadow by homogenity
  for(int i=1; i<=nvol; i++)
    {
      shader += QString("  color%2.rgba *= 1.0-smoothstep(0.0, float(%1), grad%2);\n").	\
	arg(shadowintensity).arg(i);
    }

  if (interpolateVolumes > 0 && nvol == 2)
    {
      if (interpolateVolumes == 2) // interpolate values between the volumes
	shader += "  gl_FragColor = color1;\n";
      else                         // interpolate colors between the volumes
	shader += "  gl_FragColor = mix(color1, color2, interpVol);\n";
    }
  else
    shader += mixColorOpacity(nvol, mixvol, mixColor, mixOpacity);

  if (Global::emptySpaceSkip())
    shader += "  gl_FragColor.rgba = mix(vec4(0.0,0.0,0.0,0.0), gl_FragColor.rgba, prunefeather.x);\n";

  // -- apply feather
  if (tearPresent || cropPresent || pathCropPresent)
    shader += "  gl_FragColor.rgba = mix(gl_FragColor.rgba, vec4(0.0,0.0,0.0,0.0), feather);\n";

  if (peel && peelType==2)
    {
      float reduceShadow = 1.0f - qBound(0.0f, (peelMin+1.0f)*0.5f, 1.0f);
      shader += QString("  gl_FragColor.rgba = mix(gl_FragColor.rgba, vec4(0.0,0.0,0.0,0.0), float(%1));\n").\
	        arg(reduceShadow);
    }

//------------------------------------
  shader += "gl_FragColor = 1.0-pow((vec4(1,1,1,1)-gl_FragColor),";
  shader += "vec4(lod,lod,lod,lod));\n";
//------------------------------------

  shader += "  if (gl_FragColor.a < 0.01)\n";
  shader += "	discard;\n";
  shader += "\n";
  shader += QString("  gl_FragColor.rgba *= vec4(%1, %2, %3, %4);\n").\
                                        arg(r).arg(g).arg(b).arg(maxrgb);

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";
  shader += "}\n";

  return shader;
}
QString
ShaderFactory2::genPruneTexture(int nvol)
{
  QString shader;

  QString vstr;
  if (nvol == 2) vstr = "  vec2";
  else if (nvol == 3) vstr = "  vec3";
  else if (nvol == 4) vstr = "  vec4";

  QString xyzw;
  if (nvol == 2) xyzw = "xw";
  else if (nvol == 3) xyzw = "xyz";
  else if (nvol == 4) xyzw = "xyzw";

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect dragTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += vstr + " samplex1, samplex2;\n";
  shader += vstr + " sampley1, sampley2;\n";
  shader += vstr + " samplez1, samplez2;\n";
  shader += vstr + " val;\n";
  shader += "  float g;\n";
  shader += "  vec3 sample1, sample2;\n";
  shader += "  int row, col;\n";
  shader += "  int x,y,z, x1,y1,z1;\n";
  shader +="   float s = 0.0;\n";

  shader += "  col = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "  row = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "  x = int(gl_TexCoord[0].x) - col*gridx;\n";
  shader += "  y = int(gl_TexCoord[0].y) - row*gridy;\n";
  shader += "  z = row*ncols + col;\n";
  shader += "  col *= gridx;\n";
  shader += "  row *= gridy;\n";

  shader += "  x1 = int(max(0.0, float(x-1)));\n";
  shader += "  samplex1 = texture2DRect(dragTex, vec2(col+x1, row+y))."+xyzw+";\n";
  shader += "  x1 = int(min(float(gridx-1), float(x+1)));\n";
  shader += "  samplex2 = texture2DRect(dragTex, vec2(col+x1, row+y))."+xyzw+";\n";

  shader += "  y1 = int(max(0.0, float(y-1)));\n";
  shader += "  sampley1 = texture2DRect(dragTex, vec2(col+x, row+y1))."+xyzw+";\n";
  shader += "  y1 = int(min(float(gridy-1), float(y+1)));\n";
  shader += "  sampley2 = texture2DRect(dragTex, vec2(col+x, row+y1))."+xyzw+";\n";

  shader += "  z1 = int(max(0.0, float(z-1)));\n";
  shader += "  row = z1/ncols;\n";
  shader += "  row *= gridy;\n";
  shader += "  col = z1 - row*ncols;\n";
  shader += "  col *= gridx;\n";
  shader += "  samplez1 = texture2DRect(dragTex, vec2(col+x, row+y))."+xyzw+";\n";
  shader += "  z1 = int(min(float(gridz-1), float(z+1)));\n";
  shader += "  row = z1/ncols;\n";
  shader += "  row *= gridy;\n";
  shader += "  col = z1 - row*ncols;\n";
  shader += "  col *= gridx;\n";
  shader += "  samplez2 = texture2DRect(dragTex, vec2(col+x, row+y))."+xyzw+";\n";

  shader += "  val = texture2DRect(dragTex, gl_TexCoord[0].xy)."+xyzw+";\n";

  for(int i=1; i<=nvol; i++)
    {
      QString c;
      if (i == 1) c = "x";
      else if (i == 2) c = "y";
      else if (i == 3) c = "z";
      else if (i == 4) c = "w";

      shader += QString("  sample1 = vec3(samplex1.%1, sampley1.%1, samplez1.%1);\n").arg(c);
      shader += QString("  sample2 = vec3(samplex2.%1, sampley2.%1, samplez2.%1);\n").arg(c);
      shader += "  g = distance(sample1, sample2);\n";
      shader += QString("  s = max(s, texture2D(lutTex, vec2(val.%1,g)).x);\n").arg(c);
    }
  shader += "  s = step(0.9/255.0, s);\n";
  shader += "  gl_FragColor = vec4(s,s,0.0,1.0);\n";

  shader += "}\n";

  return shader;
}
