#include "shaderfactory.h"
#include "cropshaderfactory.h"
#include "tearshaderfactory.h"
#include "shaderfactoryrgb.h"
#include "pathshaderfactory.h"
#include "global.h"

QString
ShaderFactoryRGB::getColorOpacity(bool lowres)
{
  float lutStep = 1.0/Global::lutSize();

  QString shader;

  shader = "  vec4 colOp;\n";
  shader += "  vec2 rgLut, gbLut, brLut;\n";

  if (lowres)
    shader += "  colOp = texture3D(dataTex, gl_TexCoord[0].xyz);\n";
  else
    {
      shader += "  slice = int(floor(texCoord.z));\n";
      shader += "  slicef = fract(texCoord.z);\n";

      shader += "  t0 = getTextureCoordinate(slice, gridx, tsizex, tsizey, texCoord.xy);";
      shader += "  t1 = getTextureCoordinate(slice+1, gridx, tsizex, tsizey, texCoord.xy);";

      shader += "  vg = texture2DRect(dataTex, t0);\n";
      shader += "  vg1 = texture2DRect(dataTex, t1);\n";
      shader += "  colOp = mix(vg, vg1, slicef);\n";
    }

  shader += "  colOp.rgb = min(colOp.rgb, vec3(0.996, 0.996, 0.996));\n";  

  if (Global::volumeType() == Global::RGBVolume)
    shader += "  colOp.a = 1.0;\n";

  shader += "  if (colOp.a < 0.005)\n";
  shader += "	discard;\n";

  shader += "  rgLut = colOp.rg;\n";
  shader += QString("  rgLut.y *= %1;\n").arg(lutStep);

  shader += "  gbLut = colOp.gb;\n";
  shader += QString("  gbLut.y *= %1;\n").arg(lutStep);
  shader += QString("  gbLut.y += %1;\n").arg(lutStep);

  shader += "  brLut = colOp.br;\n";
  shader += QString("  brLut.y *= %1;\n").arg(lutStep);
  shader += QString("  brLut.y += %1;\n").arg(2*lutStep);

  shader += "  float rg = texture2D(lutTex, rgLut).a;\n";
  shader += "  float gb = texture2D(lutTex, gbLut).a;\n";
  shader += "  float br = texture2D(lutTex, brLut).a;\n";
  shader += "  float alpha = rg * gb * br;\n";

  if (Global::volumeType() == Global::RGBAVolume)
    {
      // take original alpha and max of rgb values
      // as index into the lookup table
      shader += "  vec2 aLut = vec2(colOp.a, max(colOp.r, max(colOp.g, colOp.b)));\n";
      shader += QString("  aLut.y *= %1;\n").arg(lutStep);
      shader += QString("  aLut.y += %1;\n").arg(3*lutStep);
      shader += "  alpha *= texture2D(lutTex, aLut).a;\n";
    }

  shader += "  colOp.a *= alpha;\n";      
  
  shader += "  gl_FragColor.a = 1.0 - pow(1.0-colOp.a, layerSpacing);\n";

  shader += "  gl_FragColor.rgb = colOp.rgb * gl_FragColor.a;\n";

  return shader;
}

QString
ShaderFactoryRGB::genEmissiveColor(bool lowres)
{
  float lutStep = 1.0/Global::lutSize();
  float lastSet3 = 1.0-3*lutStep;

  QString shader;

  shader = "vec4 emissiveColor(vec3 texCoord)\n";
  shader += "{\n";
  shader += "  vec4 emis;\n";
  shader += "  vec2 rgLut, gbLut, brLut;\n";
  if (lowres)
    shader += "  emis = texture3D(dataTex, gl_TexCoord[0].xyz);\n";
  else
    {
      shader += "  vec4 vg, vg1;\n";
      shader += "  int row, col, slice;\n";
      shader += "  vec2 t0, t1;\n";
      shader += "  float slicef;\n";
      shader += "  slice = int(floor(texCoord.z));\n";
      shader += "  slicef = fract(texCoord.z);\n";

      shader += "  t0 = getTextureCoordinate(slice, gridx, tsizex, tsizey, texCoord.xy);";
      shader += "  t1 = getTextureCoordinate(slice+1, gridx, tsizex, tsizey, texCoord.xy);";
  
      shader += "  vg = texture2DRect(dataTex, t0);\n";
      shader += "  vg1 = texture2DRect(dataTex, t1);\n";
      shader += "  emis = mix(vg, vg1, slicef);\n";
    }

  shader += "  emis.rgb = min(emis.rgb, vec3(0.996, 0.996, 0.996));\n";  

  if (Global::volumeType() == Global::RGBVolume)
    shader += "  emis.a = 1.0;\n";

  shader += "  if (emis.a < 0.005)\n";
  shader += "	return vec4(0,0,0,0);\n";

  shader += "  rgLut = emis.rg;\n";
  shader += QString("  rgLut.y *= %1;\n").arg(lutStep);
  shader += QString("  rgLut.y += %1;\n").arg(lastSet3);

  shader += "  gbLut = emis.gb;\n";
  shader += QString("  gbLut.y *= %1;\n").arg(lutStep);
  shader += QString("  gbLut.y += %1;\n").arg(lutStep);
  shader += QString("  gbLut.y += %1;\n").arg(lastSet3);

  shader += "  brLut = emis.br;\n";
  shader += QString("  brLut.y *= %1;\n").arg(lutStep);
  shader += QString("  brLut.y += %1;\n").arg(2*lutStep);
  shader += QString("  brLut.y += %1;\n").arg(lastSet3);

  shader += "  vec4 color1, color2, color3;\n";
  shader += "  color1 = texture2D(lutTex, rgLut);\n";
  shader += "  color2 = texture2D(lutTex, gbLut);\n";
  shader += "  color3 = texture2D(lutTex, brLut);\n";
  shader += "  float alpha = color1.a*color2.a*color3.a;\n";
  shader += "  emis.a *= alpha;\n";      
  
  shader += "  emis.a = 1.0 - pow(1.0-emis.a, layerSpacing);\n";
  shader += "  alpha = max(color1.a, max(color2.a, color3.a));\n";
  shader += "  if (alpha > 0.0)\n";
  shader += "    emis.rgb = (color1.a*color1.rgb + color2.a*color2.rgb + color3.a*color3.rgb)/alpha;\n";
  shader += "  else\n";
  shader += "    emis.rgb = vec3(0,0,0)\n;";
  shader += "  emis.rgb *= emis.a;\n";

  shader += "  return emis;\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactoryRGB::getAlpha()
{
  float lutStep = 1.0/Global::lutSize();

  QString shader;

  shader = "float getAlpha(vec2 da)\n";
  shader += "{\n";
  shader += "  vec4 samp = texture2DRect(dataTex, da);\n";

  shader += "  vec2 rgLut = samp.rg;\n";
  shader += QString("  rgLut.y *= %1;\n").arg(lutStep);

  shader += "  vec2 gbLut = samp.gb;\n";
  shader += QString("  gbLut.y *= %1;\n").arg(lutStep);
  shader += QString("  gbLut.y += %1;\n").arg(lutStep);

  shader += "  vec2 brLut = samp.br;\n";
  shader += QString("  brLut.y *= %1;\n").arg(lutStep);
  shader += QString("  brLut.y += %1;\n").arg(2*lutStep);

  shader += "  float rg = texture2D(lutTex, rgLut).a;\n";
  shader += "  float gb = texture2D(lutTex, gbLut).a;\n";
  shader += "  float br = texture2D(lutTex, brLut).a;\n";
  shader += "  float alpha = rg * gb * br;\n";

  if (Global::volumeType() == Global::RGBAVolume)
    {
      // take original alpha and max of rgb values
      // as index into the lookup table
      shader += "  vec2 aLut = vec2(samp.a, max(samp.r, max(samp.g, samp.b)));\n";
      shader += QString("  aLut.y *= %1;\n").arg(lutStep);
      shader += QString("  aLut.y += %1;\n").arg(3*lutStep);
      shader += "  alpha *= texture2D(lutTex, aLut).a;\n";
    }

  shader += "  alpha *= samp.a;\n";

  shader += "  return alpha;\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactoryRGB::genDefaultSliceShaderString(bool lighting,
					      bool emissive,
					      QList<CropObject> crops,
					      bool peel, int peelType,
					      float peelMin, float peelMax, float peelMix)
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


  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointpos;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect dataTex;\n";

  shader += "uniform float layerSpacing;\n";
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

  shader += "uniform vec2 dataMin;\n";
  shader += "uniform vec2 dataSize;\n";
  shader += "uniform int tminz;\n";

  shader += "uniform int lod;\n";
  shader += "uniform vec3 dirFront;\n";
  shader += "uniform vec3 dirUp;\n";
  shader += "uniform vec3 dirRight;\n";

  shader += "uniform vec3 brickMin;\n";
  shader += "uniform vec3 brickMax;\n";

  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform int lightgridx;\n";
  shader += "uniform int lightgridy;\n";
  shader += "uniform int lightgridz;\n";
  shader += "uniform int lightnrows;\n";
  shader += "uniform int lightncols;\n";
  shader += "uniform int lightlod;\n";

  shader += "uniform float prunelod;\n";
  shader += "uniform int zoffset;\n";

  shader += ShaderFactory::genTextureCoordinate();

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();

  if (emissive) shader += genEmissiveColor();

  if (peel || lighting) shader += getAlpha();


  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec3 lightcol = vec3(1.0,1.0,1.0);\n";

  shader += "  vec3 texCoord = gl_TexCoord[0].xyz;\n";

  shader += "if (any(lessThan(texCoord,brickMin)) || any(greaterThan(texCoord, brickMax)))\n";
  shader += "  discard;\n";

  if (crops.count() > 0)
    shader += "  vec3 otexCoord = texCoord;\n";


  if (cropPresent || tearPresent)
    {
      shader += "  float feather = 1.0;\n";
      shader += "  float myfeather;\n";
      shader += "  vec3 p0, pvec, saxis, taxis;\n";
      shader += "  vec3 op0, opvec, osaxis, otaxis;\n";
      shader += "  float plen, srad1, srad2, trad1, trad2;\n";
    }

  if (crops.count() == 0)
    {
      if (pathCropPresent)
	{
	  shader += "  float feather = 1.0;\n";
	  shader += "  float myfeather;\n";
	}
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

  shader += "  vec4 vg, vg1;\n";
  shader += "  int row, col, slice;\n";
  shader += "  vec2 t0, t1;\n";
  shader += "  float slicef;\n";

  shader += getColorOpacity();

  // -- apply feather
  if (tearPresent || cropPresent || pathCropPresent)
    shader += "  gl_FragColor.rgba = mix(gl_FragColor.rgba, vec4(0.0,0.0,0.0,0.0), feather);\n";  

  shader += "  if (gl_FragColor.a < 0.005)\n";
  shader += "	discard;\n";

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

  if (peel || lighting)
    shader += getNormal();

  if (lighting)
    shader += addLighting();

  //------------------
  if (peel && peelType!=2)
    {
      if (!lighting) 
	{
	  shader += "  vec3 voxpos = pointpos;\n";
	  shader += "  vec3 I = voxpos - eyepos;\n";
	  shader += "  I = normalize(I);\n";
	}
      shader += "  float IdotN = dot(I, normal);\n";

      if (peelType == 0)
	{
	  if (peelMin < peelMax)
	    shader += QString("  float val1 = smoothstep(float(%1), float(%2), IdotN);\n").arg(peelMin).arg(peelMax);
	  else
	    shader += QString("  float val1 = smoothstep(float(%1), float(%2), -IdotN);\n").arg(peelMax).arg(peelMin);
	}
      else if (peelType == 1)
	{ //---- keep inside
	  shader += QString("  float val1 = 1.0-smoothstep(float(%1)-0.1, float(%1), IdotN);\n").arg(peelMin);
	  shader += QString("  val1 *= smoothstep(float(%1), float(%1)+0.1, IdotN);\n").arg(peelMax);
	}
//      else
//	{ //---- keep outside
//	  shader += QString("  float val1 = smoothstep(float(%1)-0.1, float(%1), IdotN);\n").arg(peelMin);
//	  shader += QString("  val1 = max(val1, (1.0 - smoothstep(float(%1), float(%1)+0.1, IdotN)));\n").arg(peelMax);
//	}

      shader += QString("  IdotN = mix(val1, 1.0, float(%1));\n").arg(peelMix);
      shader += "  gl_FragColor.rgba *= IdotN;\n";
    }
  //------------------

  if (emissive)
    {
      shader += "  vec4 eColor = emissiveColor(texCoord)\n;";
      shader += "  gl_FragColor.rgb += eColor.rgb;\n";
    }

  // -- depth cueing
  shader += "  gl_FragColor.rgb *= depthcue;\n";

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";
  
  shader += "\n";
  shader += "}\n";

  return shader;
}
QString
ShaderFactoryRGB::genHighQualitySliceShaderString(bool lighting,
						  bool emissive,
						  bool shadows,
						  float shadowintensity,
						  QList<CropObject> crops,
						  bool peel, int peelType,
						  float peelMin, float peelMax, float peelMix)
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


  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointpos;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect dataTex;\n";
  shader += "uniform sampler2DRect shadowTex;\n";

  shader += "uniform float layerSpacing;\n";
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

  shader += "uniform vec2 dataMin;\n";
  shader += "uniform vec2 dataSize;\n";
  shader += "uniform int tminz;\n";

  shader += "uniform int lod;\n";
  shader += "uniform vec3 dirFront;\n";
  shader += "uniform vec3 dirUp;\n";
  shader += "uniform vec3 dirRight;\n";

  shader += "uniform vec3 brickMin;\n";
  shader += "uniform vec3 brickMax;\n";

  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform int lightgridx;\n";
  shader += "uniform int lightgridy;\n";
  shader += "uniform int lightgridz;\n";
  shader += "uniform int lightnrows;\n";
  shader += "uniform int lightncols;\n";
  shader += "uniform int lightlod;\n";

  shader += "uniform float prunelod;\n";
  shader += "uniform int zoffset;\n";

  shader += ShaderFactory::genTextureCoordinate();

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();

  if (emissive) shader += genEmissiveColor();

  if (peel || lighting)
    shader += getAlpha();


  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec3 lightcol = vec3(1.0,1.0,1.0);\n";

  shader += "  vec3 texCoord = gl_TexCoord[0].xyz;\n";

  shader += "if (any(lessThan(texCoord,brickMin)) || any(greaterThan(texCoord, brickMax)))\n";
  shader += "  discard;\n";

  if (crops.count() > 0)
    shader += "  vec3 otexCoord = texCoord;\n";


  if (cropPresent || tearPresent)
    {
      shader += "  float feather = 1.0;\n";
      shader += "  float myfeather;\n";
      shader += "  vec3 p0, pvec, saxis, taxis;\n";
      shader += "  vec3 op0, opvec, osaxis, otaxis;\n";
      shader += "  float plen, srad1, srad2, trad1, trad2;\n";
    }

  if (crops.count() == 0)
    {
      if (pathCropPresent)
	{
	  shader += "  float feather = 1.0;\n";
	  shader += "  float myfeather;\n";
	}
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

  shader += "  vec4 vg, vg1;\n";
  shader += "  int row, col, slice;\n";
  shader += "  vec2 t0, t1;\n";
  shader += "  float slicef;\n";

  shader += getColorOpacity();

  // -- apply feather
  if (cropPresent || tearPresent || pathCropPresent)
    shader += "  gl_FragColor.rgba = mix(gl_FragColor.rgba, vec4(0.0,0.0,0.0,0.0), feather);\n";  

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

  if (peel || lighting)
    shader += getNormal();
  if (lighting)
    shader += addLighting();

  //------------------
  if (peel && peelType!=2)
    {
      if (!lighting) 
	{
	  shader += "  vec3 voxpos = pointpos;\n";
	  shader += "  vec3 I = voxpos - eyepos;\n";
	  shader += "  I = normalize(I);\n";
	}
      shader += "  float IdotN = dot(I, normal);\n";

      if (peelType == 0)
	{
	  if (peelMin < peelMax)
	    shader += QString("  float val1 = smoothstep(float(%1), float(%2), IdotN);\n").arg(peelMin).arg(peelMax);
	  else
	    shader += QString("  float val1 = smoothstep(float(%1), float(%2), -IdotN);\n").arg(peelMax).arg(peelMin);
	}
      else if (peelType == 1)
	{ //---- keep inside
	  shader += QString("  float val1 = 1.0-smoothstep(float(%1)-0.1, float(%1), IdotN);\n").arg(peelMin);
	  shader += QString("  val1 *= smoothstep(float(%1), float(%1)+0.1, IdotN);\n").arg(peelMax);
	}
//      else
//	{ //---- keep outside
//	  shader += QString("  float val1 = smoothstep(float(%1)-0.1, float(%1), IdotN);\n").arg(peelMin);
//	  shader += QString("  val1 = max(val1, (1.0 - smoothstep(float(%1), float(%1)+0.1, IdotN)));\n").arg(peelMax);
//	}

      shader += QString("  IdotN = mix(val1, 1.0, float(%1));\n").arg(peelMix);
      shader += "  gl_FragColor.rgba *= IdotN;\n";
    }
  //------------------

  if (shadows)
    {
      //----------------------
      // shadows
      shader += "  vec4 incident_light = texture2DRect(shadowTex, gl_TexCoord[1].xy);\n";
      shader += "  float maxval = 1.0 - incident_light.a;\n";
      if (shadowintensity < 0.95)
	{
	  if (shadowintensity > 0.05)
	    {
	      shader += "  incident_light = maxval*(incident_light + maxval);\n";
	      shader += "  gl_FragColor.rgb *= mix(vec3(1.0,1.0,1.0), ";
	      shader += QString("incident_light.rgb, %1);\n").arg(shadowintensity);
	    }
	}
      else
	shader += "  gl_FragColor.rgb *= maxval*(incident_light.rgb + maxval);\n";
      //----------------------
    }

  if (emissive)
    {
      shader += "  vec4 eColor = emissiveColor(texCoord)\n;";
      shader += "  gl_FragColor.rgb += eColor.rgb;\n";
    }

  // -- depth cueing
  shader += "  gl_FragColor.rgb *= depthcue;\n";

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";
  
  shader += "}\n";

  return shader;
}

QString
ShaderFactoryRGB::getNormal()
{
  QString shader;

  shader = "  vec3 normal, sample1, sample2;\n";

  shader += "  vec4 rgba;\n";
  
  for (int i=0; i<6; i++)
    {
      if (i == 0) shader += "  sample1.x = getAlpha(t0+vec2(1,0));\n";
      if (i == 1) shader += "  sample2.x = getAlpha(t0-vec2(1,0));\n";
      if (i == 2) shader += "  sample1.y = getAlpha(t0+vec2(0,1));\n";
      if (i == 3) shader += "  sample2.y = getAlpha(t0-vec2(0,1));\n";
      if (i == 4) shader += "  sample1.z = getAlpha(t0);\n";
      if (i == 5)
	{
	  shader += "  t1 = getTextureCoordinate(slice-1, gridx, tsizex, tsizey, texCoord.xy);";
	  shader += "  sample2.z = getAlpha(t1);\n";
	}
    }

  shader += "  float normlen = distance(sample1,sample2);\n"; 
  shader += "  if (normlen > 0.0)\n"; 
  shader += "     normal = normalize(sample1 - sample2);\n";
  shader += "  else\n";
  shader += "     normal = vec3(0.0,0.0,0.0);\n";

  return shader;
}

QString
ShaderFactoryRGB::addLighting()
{
  QString shader;

  //shader = "  vec3 lightcol = vec3(1.0,1.0,1.0);\n";
  shader += "  vec3 voxpos = pointpos;\n";
  shader += "  vec3 I = voxpos - eyepos;\n";
  shader += "  vec3 lightvec = voxpos - lightpos;\n";
  shader += "  I = normalize(I);\n";
  shader += "  lightvec = normalize(lightvec);\n";

  shader += "  vec3 reflecvec = reflect(lightvec, normal);\n";
  shader += "  float DiffMag = abs(dot(normal, lightvec));\n";
  shader += "  vec3 Diff = diffuse*DiffMag*gl_FragColor.rgb;\n";
  shader += "  float Spec = pow(abs(dot(normal, reflecvec)), speccoeff);\n";
  shader += "  Spec *= specular*gl_FragColor.a;\n";
  shader += "  vec3 Amb = ambient*gl_FragColor.rgb;\n";
  shader += "  float litfrac = smoothstep(0.01, 0.02, normlen);\n";
  shader += "  gl_FragColor.rgb *= (1.0-litfrac);\n";
  shader += "  gl_FragColor.rgb += litfrac*Amb;\n";
  shader += "  if (normlen > 0.005)\n";
  shader += "   {\n";
  shader += "     vec3 frgb = gl_FragColor.rgb + litfrac*(Diff + Spec);\n";
  shader += "     if (any(greaterThan(frgb,vec3(1.0,1.0,1.0)))) \n";
  shader += "        frgb = vec3(1.0,1.0,1.0);\n";
  shader += "     if (any(greaterThan(frgb,gl_FragColor.aaa))) \n";  
  shader += "        frgb = gl_FragColor.aaa;\n";
  shader += "     gl_FragColor.rgb = frgb;\n";
  shader += "   }\n";
  shader += "  gl_FragColor.rgb *= lightcol;\n";

  return shader;
}

QString
ShaderFactoryRGB::genSliceShadowShaderString(float r, float g, float b,
					     QList<CropObject> crops,
					     bool peel, int peelType,
					     float peelMin, float peelMax, float peelMix)
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

  shader += "uniform float layerSpacing;\n";

  shader += "uniform vec3 eyepos;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int tsizex;\n";
  shader += "uniform int tsizey;\n";

  shader += "uniform vec2 dataMin;\n";
  shader += "uniform vec2 dataSize;\n";
  shader += "uniform int tminz;\n";

  shader += "uniform int lod;\n";
  shader += "uniform vec3 dirFront;\n";
  shader += "uniform vec3 dirUp;\n";
  shader += "uniform vec3 dirRight;\n";

  shader += "uniform vec3 brickMin;\n";
  shader += "uniform vec3 brickMax;\n";

  shader += ShaderFactory::genTextureCoordinate();

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();

  if (peel)
    shader += getAlpha();

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec3 texCoord = gl_TexCoord[0].xyz;\n";

  shader += "if (any(lessThan(texCoord,brickMin)) || any(greaterThan(texCoord, brickMax)))\n";
  shader += "  discard;\n";

  if (crops.count() > 0)
    shader += "  vec3 otexCoord = texCoord;\n";

  if (cropPresent || tearPresent)
    {
      shader += "  float feather = 1.0;\n";
      shader += "  float myfeather;\n";
      shader += "  vec3 p0, pvec, saxis, taxis;\n";
      shader += "  vec3 op0, opvec, osaxis, otaxis;\n";
      shader += "  float plen, srad1, srad2, trad1, trad2;\n";
    }

  if (crops.count() == 0)
    {
      if (pathCropPresent)
	{
	  shader += "  float feather = 1.0;\n";
	  shader += "  float myfeather;\n";
	}
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


  shader += "  vec4 vg, vg1;\n";
  shader += "  int row, col, slice;\n";
  shader += "  vec2 t0, t1;\n";
  shader += "  float slicef;\n";

  shader += getColorOpacity();
	
  //---------------------
  if (cropPresent || tearPresent || pathCropPresent)
    shader += "  gl_FragColor.rgba = mix(gl_FragColor.rgba, vec4(0.0,0.0,0.0,0.0), feather);\n";  
  //---------------------

  if (peel && peelType!=2)
    {
      shader += getNormal();
      shader += "  vec3 voxpos = pointpos;\n";
      shader += "  vec3 I = voxpos - eyepos;\n";
      shader += "  I = normalize(I);\n";
      shader += "  float IdotN = dot(I, normal);\n";

      if (peelType == 0)
	{
	  if (peelMin < peelMax)
	    shader += QString("  float val1 = smoothstep(float(%1), float(%2), IdotN);\n").arg(peelMin).arg(peelMax);
	  else
	    shader += QString("  float val1 = smoothstep(float(%1), float(%2), -IdotN);\n").arg(peelMax).arg(peelMin);
	}
      else if (peelType == 1)
	{ //---- keep inside
	  shader += QString("  float val1 = 1.0-smoothstep(float(%1)-0.1, float(%1), IdotN);\n").arg(peelMin);
	  shader += QString("  val1 *= smoothstep(float(%1), float(%1)+0.1, IdotN);\n").arg(peelMax);
	}
//      else
//	{ //---- keep outside
//	  shader += QString("  float val1 = smoothstep(float(%1)-0.1, float(%1), IdotN);\n").arg(peelMin);
//	  shader += QString("  val1 = max(val1, (1.0 - smoothstep(float(%1), float(%1)+0.1, IdotN)));\n").arg(peelMax);
//	}

      shader += QString("  IdotN = mix(val1, 1.0, float(%1));\n").arg(peelMix);
      shader += "  gl_FragColor.rgba *= IdotN;\n";
    }
  //------------------

  if (peel && peelType==2)
    {
      float reduceShadow = 1.0f - qBound(0.0f, (peelMin+1.0f)*0.5f, 1.0f);
      shader += QString("  gl_FragColor.rgba = mix(gl_FragColor.rgba, vec4(0.0,0.0,0.0,0.0), float(%1));\n").\
	        arg(reduceShadow);
    }


  shader += QString("  gl_FragColor.rgba *= vec4(%1, %2, %3, %4);\n").\
                                        arg(r).arg(g).arg(b).arg(maxrgb);
 
  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactoryRGB::genDefaultShaderString(bool emissive, bool lowres)
{
  QString shader;

  shader = "uniform sampler2D lutTex;\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform float layerSpacing;\n";

  if (emissive)
    shader += genEmissiveColor(lowres);

  shader += "void main(void)\n";
  shader += "{\n";

  shader += getColorOpacity(lowres);

  if (emissive)
    {
      shader += "  vec4 eColor = emissiveColor(vec3(0,0,0))\n;";
      shader += "  gl_FragColor.rgb += eColor.rgb;\n";
    }

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";
  
  shader += "\n";
  shader += "}\n";

  return shader;
}
