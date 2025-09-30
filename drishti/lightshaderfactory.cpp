#include "global.h"
#include "lightshaderfactory.h"

QString
LightShaderFactory::getVal()
{
  QString shader;
  shader += "vec4 getVal(vec3 voxelCoord)\n";  
  shader += "{\n";
  shader += "  float layer = voxelCoord.z;\n";
  shader += "  vec3 vcrd0 = vec3(voxelCoord.xy/dragsize.xy, layer);\n";
  shader += "  return texture(dragTex, vcrd0);\n";
  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genOpacityShader(int nvol, bool bit16, bool amrData)
{
  QString shader;

  QString vstr;
  vstr = "  float";
  if (nvol == 2) vstr = "  vec2";
  else if (nvol == 3) vstr = "  vec3";
  else if (nvol == 4) vstr = "  vec4";

  QString xyzw;
  xyzw = "x";
  if (nvol == 2) xyzw = "xw";
  else if (nvol == 3) xyzw = "xyz";
  else if (nvol == 4) xyzw = "xyzw";

  shader = "#version 450 core\n";
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "in vec3 glTexCoord0;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DArray dragTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int llod;\n";
  shader += "uniform int lgridx;\n";
  shader += "uniform int lgridy;\n";
  shader += "uniform int lgridz;\n";
  shader += "uniform int lncols;\n";
  shader += "uniform bool opshader;\n";
  shader += "uniform float tfSet;\n";
  shader += "uniform vec3 dragsize;\n";

  if (amrData)
    {
      shader += "uniform sampler2DRect amrTex;\n";
      shader += "uniform float dragLod;\n";
    }

  shader += "uniform sampler2DRect pruneTex;\n";

  
  shader += "out vec4 glFragColor;\n";
  
  //---------------------
  // get voxel value from array texture
  shader += getVal();
  //---------------------

  shader += "void main(void)\n";
  shader += "{\n";
  shader += vstr + " val;\n";
  shader += "  float g;\n";

  // we are in lighttexture space
  shader += "  vec2 tc = glTexCoord0.xy;\n";
  shader += "  int lcol = int(tc.x)/lgridx;\n";
  shader += "  int lrow = int(tc.y)/lgridy;\n";
  shader += "  float x = tc.x - float(lcol*lgridx);\n";
  shader += "  float y = tc.y - float(lrow*lgridy);\n";
  shader += "  float z = float(lrow*lncols + lcol);\n";

//  //-----------------------------------------
//  // set every border voxel to 0
//  {
//    shader += "  vec3 pos = vec3(x,y,z);\n";
//    shader += "  bvec3 pless = lessThan(pos, vec3(1.5,1.5,1.5));\n";
//    shader += "  bvec3 pgret = greaterThan(pos, vec3(float(lgridx)-2.5,float(lgridy)-2.5,float(lgridz)-2.5));\n";
//    shader += "  if (any(pless) || any(pgret)) \n";
//    shader += "    { glFragColor = vec4(0.0,0.0,0.0,0.0); return; }\n";
//  }
//  shader += "  vec3 pos = vec3(x,y,z);\n";
//  shader += "  pos += step(pos, vec3(llod));\n";
//  shader += "  pos -= step(vec3(lgridx,lgridy,lgridz)-vec3(llod), pos);\n";
//  //-----------------------------------------

  shader += "  x *= float(llod);\n";
  shader += "  y *= float(llod);\n";
  shader += "  z *= float(llod);\n";

  //---------------
  // generate prune texture coordinate
  shader += "  int prow = int(z/ncols);\n";
  shader += "  int pcol = int(z - prow*ncols);\n";
  shader += "  vec2 prunetc = vec2(x + float(pcol*gridx),\n";
  shader += "                      y + float(prow*gridy));\n";
  //---------------
  
  shader += "  float alpha = 0.0;\n";
  shader += "  float totalpha = 0.0;\n";
  shader += "  vec3 rgb = vec3(0.0,0.0,0.0);\n";
  shader += "  vec4 color;\n";

  shader += "  vec3 vcrd = vec3(x,y,z);\n";

  if (amrData)
    {      
      shader += "  float amrX = texture2DRect(amrTex, vec2(dragLod*x,0)).x;\n";
      shader += "  float amrY = texture2DRect(amrTex, vec2(dragLod*y,0)).y;\n";
      shader += "  float amrZ = texture2DRect(amrTex, vec2(dragLod*z,0)).z;\n";
      
      shader += "  vcrd = vec3(amrX/dragLod, amrY/dragLod, amrZ/dragLod);\n";
    }

  shader += "  val = getVal(vcrd)."+xyzw+";\n";

  shader += "  vec2 k = vec2(1.0, -1.0);\n";
  shader += vstr + " g1,g2,g3,g4;\n";
  shader += "  vec3 grad;\n";
  shader += "  int h0, h1;\n";
  
  for(int i=1; i<=nvol; i++)
    {
      QString c;
      if (i == 1) c = "x";
      else if (i == 2) c = "y";
      else if (i == 3) c = "z";
      else if (i == 4) c = "w";
      
      if (!bit16)
	{
	  shader += "  g1 = getVal(vcrd+k.xyy)."+xyzw+";\n";
	  shader += "  g2 = getVal(vcrd+k.yyx)."+xyzw+";\n";
	  shader += "  g3 = getVal(vcrd+k.yxy)."+xyzw+";\n";
	  shader += "  g4 = getVal(vcrd+k.xxx)."+xyzw+";\n";
	  shader += "  grad = (k.xyy * g1."+c+" + k.yyx * g2."+c+" + k.yxy * g3."+c+" + k.xxx * g4."+c+");\n";
	  shader += "  g = length(grad/2.0);\n";
	}
      else
	{
	  shader += "h0 = int(65535.0*val."+c+");\n";
	  shader += "h1 = h0 / 256;\n";
	  shader += "h0 = int(mod(float(h0),256.0));\n";
	  shader += "val."+c+" += float(h0)/256.0;\n";
	  shader += "g = float(h1)/256.0;\n";
	}
      shader += QString("  g = tfSet + (float(%1) + g)/float(%2);\n").arg(i-1).arg(Global::lutSize());
      shader += "  color = texture(lutTex, vec2(val."+c+",g));\n";
      shader += "  rgb += color.rgb;\n";
      shader += "  alpha = max(alpha, color.a);\n";      

      // modify alpha by prune texture
      shader += "  alpha *= texture(pruneTex, prunetc).x;\n";
      //---------------

      shader += "  totalpha += color.a;\n";
    }
  
  shader += "  glFragColor.a = alpha;\n";
  shader += "  glFragColor.rgb = alpha*rgb/totalpha;\n";

  shader += " if (opshader)\n";
  {
    if (nvol == 1)
      shader += "   glFragColor = vec4(val.x, g, glFragColor.a, 1.0);\n";
    else if (nvol == 2)
      shader += "   glFragColor = vec4(val.y, g, glFragColor.a, 1.0);\n";
    else
      shader += "   glFragColor = glFragColor.aaaa;\n";  
  }
  shader += " else\n";
  shader += "   {\n";
  shader += "     glFragColor.rgb *= glFragColor.a;\n";  
  shader += "     glFragColor.a = step(0.01, glFragColor.a);\n";
  shader += "   }\n";

  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genOpacityShaderRGB()
{
  QString shader;

  shader = "#version 450 core\n";
  shader +=  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "in vec3 glTexCoord0;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DArray dragTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int llod;\n";
  shader += "uniform int lgridx;\n";
  shader += "uniform int lgridy;\n";
  shader += "uniform int lgridz;\n";
  shader += "uniform int lncols;\n";
  shader += "uniform bool opshader;\n";
  shader += "uniform float tfSet;\n";
  shader += "uniform vec3 dragsize;\n";

  shader += "out vec4 glFragColor;\n";
  
  //---------------------
  // get voxel value from array texture
  shader += getVal();
  //---------------------

  shader += "void main(void)\n";
  shader += "{\n";

  // we are in lighttexture space
  shader += "  vec2 tc = glTexCoord0.xy;\n";
  shader += "  int lcol = int(tc.x)/lgridx;\n";
  shader += "  int lrow = int(tc.y)/lgridy;\n";
  shader += "  float x = tc.x - float(lcol*lgridx);\n";
  shader += "  float y = tc.y - float(lrow*lgridy);\n";
  shader += "  float z = float(lrow*lncols + lcol);\n";

//  //-----------------------------------------
//  // set every border voxel to 0
//  {
//    shader += "  vec3 pos = vec3(x,y,z);\n";
//    shader += "  bvec3 pless = lessThan(pos, vec3(1.5,1.5,1.5));\n";
//    shader += "  bvec3 pgret = greaterThan(pos, vec3(float(lgridx)-2.5,float(lgridy)-2.5,float(lgridz)-2.5));\n";
//    shader += "  if (any(pless) || any(pgret)) \n";
//    shader += "    { glFragColor = vec4(0.0,0.0,0.0,0.0); return; }\n";
//  }
//  //-----------------------------------------


  shader += "  x *= float(llod);\n";
  shader += "  y *= float(llod);\n";
  shader += "  z *= float(llod);\n";
  
  shader += "  vec4 colOp = vec4(getVal(vec3(x,y,z)).xyz, 1.0);\n";
  
  float lutStep = 1.0/(float)Global::lutSize();
  shader += "  vec2 rgLut = colOp.rg;\n";
  shader += QString("  rgLut.y *= %1;\n").arg(lutStep);
  shader += "  rgLut.y += tfSet;\n";

  shader += "  vec2 gbLut = colOp.gb;\n";
  shader += QString("  gbLut.y *= %1;\n").arg(lutStep);
  shader += QString("  gbLut.y += %1;\n").arg(lutStep);
  shader += "  gbLut.y += tfSet;\n";

  shader += "  vec2 brLut = colOp.br;\n";
  shader += QString("  brLut.y *= %1;\n").arg(lutStep);
  shader += QString("  brLut.y += %1;\n").arg(2*lutStep);
  shader += "  brLut.y += tfSet;\n";

  shader += "  float rg = texture2D(lutTex, rgLut).a;\n";
  shader += "  float gb = texture2D(lutTex, gbLut).a;\n";
  shader += "  float br = texture2D(lutTex, brLut).a;\n";
  shader += "  float alpha = rg * gb * br;\n";

  shader += "  colOp.a *= alpha;\n";      

  shader += "  glFragColor.rgba = vec4(colOp.rgb,1.0)*colOp.a;\n";

  shader += " if (opshader)\n";
  shader += "   glFragColor = glFragColor.aaaa;\n";  
  shader += " else\n";
  shader += "   glFragColor.a = step(0.01, glFragColor.a);\n";


  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genAOLightShader() // surround shader
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect opTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int orad;\n";
  shader += "uniform float ofrac;\n";
  shader += "uniform float den1;\n";
  shader += "uniform float den2;\n";
  shader += "uniform float opmod;\n";

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";

  shader += "  int col = int(tc.x)/gridx;\n";
  shader += "  int row = int(tc.y)/gridy;\n";
  shader += "  int x = int(tc.x) - col*gridx;\n";
  shader += "  int y = int(tc.y) - row*gridy;\n";
  shader += "  int z = row*ncols + col;\n";

  shader += "  float op = texture2DRect(opTex, tc).z;\n"; 
  shader += "  op = clamp(opmod*op, 0.0, 1.0);\n";

  shader += "  vec3 pos = vec3(x,y,z);\n";
  shader += "  pos += step(pos, vec3(1.0));\n";
  shader += "  pos -= step(vec3(gridx,gridy,gridz)-vec3(2.0), pos);\n";
//  shader += "  bvec3 pless = lessThan(pos, vec3(0.5,0.5,0.5));\n";
//  shader += "  bvec3 pgret = greaterThan(pos, vec3(float(gridx)-1.5,float(gridy)-1.5,float(gridz)-1.5));\n";
//  shader += "  if (any(pless) || any(pgret)) \n";
//  shader += "  { gl_FragColor = vec4(1.0,op,1.0,1.0); return; }\n";

  shader += "  float fop = 0.0;\n";
  shader += "  for(int k=-orad; k<=orad; k++)\n";
  shader += "   {\n";
  shader += "     int z1 = z+k;\n";
  shader += "     int row = z1/ncols;\n";
  shader += "     int col = z1 - row*ncols;\n";
  shader += "     row *= gridy;\n";
  shader += "     col *= gridx;\n";
  shader += "     for(int i=-orad; i<=orad; i++)\n";
  shader += "     for(int j=-orad; j<=orad; j++)\n";
  shader += "      {\n";
  shader += "        float x1 = float(col+x+i)+0.5;\n";
  shader += "        float y1 = float(row+y+j)+0.5;\n";
  shader += "        float op = texture2DRect(opTex, vec2(x1,y1)).z;\n"; 
  shader += "        op = clamp(opmod*op, 0.0, 1.0);\n";
  shader += "        fop += step(0.1, op);\n";
  shader += "      }\n";
  shader += "   }\n";

  shader += "  float ni = float(2*orad+1);\n";
  shader += "  float den = mix(den2, den1, smoothstep(0.0, ni*ni*ni*ofrac, fop));\n";
  shader += "  gl_FragColor = vec4(den,op,den,1.0);\n";

  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genInitEmissiveShader() // tf emissive shader
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect opTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform float opmod;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform sampler2DRect eTex;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";

  shader += "  int col = int(tc.x)/gridx;\n";
  shader += "  int row = int(tc.y)/gridy;\n";
  shader += "  int x = int(tc.x) - col*gridx;\n";
  shader += "  int y = int(tc.y) - row*gridy;\n";
  shader += "  int z = row*ncols + col;\n";

  shader += "  float den = step(0.005, texture2DRect(eTex, tc).a);\n"; 
  //shader += "  float den = texture2DRect(eTex, tc).a;\n"; 
  shader += "  float op = opmod*texture2DRect(opTex, tc).z;\n"; 
  shader += "  gl_FragColor = vec4(den,op,den,1.0);\n";
  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genEmissiveShader() // tf emissive shader
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int ncols;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";

  shader += "  gl_FragColor = texture2DRect(lightTex, tc.xy);\n";

  shader += "  if (gl_FragColor.x > 0.95) return;\n";


  shader += "  int col = int(tc.x)/gridx;\n";
  shader += "  int row = int(tc.y)/gridy;\n";
  shader += "  int x = int(tc.x) - col*gridx;\n";
  shader += "  int y = int(tc.y) - row*gridy;\n";
  shader += "  int z = row*ncols + col;\n";
  
  shader += "  float nlit = 0.0;\n";
  shader += "  float lit = 0.0;\n";
//  shader += "  float nlit = 1.0;\n";
//  shader += "  float lit = gl_FragColor.x;\n";

  shader += "  int i,j,k;\n";
  shader += "  int idx = 0;\n";
  shader += "  for(i=-1; i<=1; i++)\n";
  shader += "  for(j=-1; j<=1; j++)\n";
  shader += "  for(k=-1; k<=1; k++)\n";
  shader += "  {\n";
  shader += "     int z1 = z+k;\n";
  shader += "     int row = z1/ncols;\n";
  shader += "     int col = z1 - row*ncols;\n";
  shader += "     row *= gridy;\n";
  shader += "     col *= gridx;\n";
  shader += "     float x1 = float(col+x+i)+0.5;\n";
  shader += "     float y1 = float(row+y+j)+0.5;\n";
  shader += "     vec2 ldop = texture2DRect(lightTex, vec2(x1,y1)).xy;\n";
  shader += "     float illum = (1.0-ldop.y)*ldop.x;\n";
  shader += "     nlit += step(0.001, illum);\n";
  shader += "     lit += illum;\n";
  shader += "     idx = idx + 1;\n";
  shader += "  }\n";

  shader += "  nlit = max(1.0, nlit);\n";
  shader += "  gl_FragColor.x = clamp(lit/nlit, 0.0, 0.9);\n";

  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genEFinalLightShader()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform sampler2DRect eTex;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 light = texture2DRect(eTex, gl_TexCoord[0].xy);\n";
  shader += "  light.rgb *= texture2DRect(lightTex, gl_TexCoord[0].xy).x;\n";
  shader += "  gl_FragColor = vec4(light.rgb, 1.0);\n";
  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0), vec4(1.0));\n";
  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genMergeOpPruneShader()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform sampler2DRect opTex;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  //shader += "  vec4 op = texture2DRect(opTex, gl_TexCoord[0].xy);\n";
  shader += "  vec4 op = texture2DRect(opTex, gl_TexCoord[0].xy).zzzz;\n";
  shader += "  op.rgb *= texture2DRect(lightTex, gl_TexCoord[0].xy).xxx;\n";
  shader += "  gl_FragColor = op;\n";
  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genInitDLightShader() // directional shader
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect opTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform vec3 ldir;\n";  
  shader += "uniform float oplod;\n";
  shader += "uniform int opgridx;\n";
  shader += "uniform int opgridy;\n";
  shader += "uniform int opncols;\n";
  shader += "uniform float opmod;\n";

  shader += "void main(void)\n";
  shader += "{\n";

//  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";
//  shader += "  int col = int(tc.x)/gridx;\n";
//  shader += "  int row = int(tc.y)/gridy;\n";
//  shader += "  int x = int(tc.x) - col*gridx;\n";
//  shader += "  int y = int(tc.y) - row*gridy;\n";
//  shader += "  int z = row*ncols + col;\n";

  // we are in lighttexture space
  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";
  shader += "  int col = int(tc.x)/gridx;\n";
  shader += "  int row = int(tc.y)/gridy;\n";
  shader += "  float x = tc.x - float(col*gridx);\n";
  shader += "  float y = tc.y - float(row*gridy);\n";
  shader += "  float z = float(row*ncols + col);\n";
  shader += "  vec3 orig = vec3(x,y,z);\n";

  // convert x,y,z to optexture space
  shader += "  float xo = x/oplod;\n";
  shader += "  float yo = y/oplod;\n";
  shader += "  float zo = z/oplod;\n";
  shader += "  int oprow = int(zo)/opncols;\n";
  shader += "  int opcol = int(zo) - oprow*opncols;\n";
  shader += "  oprow *= opgridy;\n";
  shader += "  opcol *= opgridx;\n";
  shader += "  vec2 optc = vec2(float(opcol)+xo, float(oprow)+yo);\n";

  //shader += "  vec3 orig = vec3(x,y,z);\n";
  shader += "  vec3 pos = orig + ldir;\n";
  shader += "  bvec3 pless = lessThan(pos, vec3(0.5,0.5,0.5));\n";
  shader += "  bvec3 pgret = greaterThan(pos, vec3(float(gridx)-1.5,float(gridy)-1.5,float(gridz)-1.5));\n";
  shader += "  float den = 0.0;\n";
  shader += "  if (any(pless) || any(pgret)) \n";
  shader += "    den = 1.0;\n";  

  shader += "  float op = texture2DRect(opTex, optc).z;\n";   
  shader += "  op = clamp(opmod*op, 0.0, 1.0);\n";
  shader += "  gl_FragColor = vec4(den,op,den,1.0);\n";
  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genDLightShader() // directional shader
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform vec3 ldir;\n";
  shader += "uniform float cangle;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  int row, col;\n";
  shader += "  int x,y,z, x1,y1,z1;\n";

  
  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";
  shader += "  col = int(tc.x)/gridx;\n";
  shader += "  row = int(tc.y)/gridy;\n";
  shader += "  x = int(tc.x) - col*gridx;\n";
  shader += "  y = int(tc.y) - row*gridy;\n";
  shader += "  z = row*ncols + col;\n";

  shader += "  vec2 dop = texture2DRect(lightTex, tc.xy).xy;\n";
  shader += "  gl_FragColor = vec4(dop,1.0,1.0);\n";
  shader += "  if (dop.x > 0.98)\n";
  shader += "    return;\n";

//  shader += "  if (dop.x > 2.0/255.0)\n";
//  shader += "    return;\n";
  
  // add all weighted opacities
  shader += "  vec2 fop = vec2(0.0,0.0);\n";
  shader += "  for(int k=-1; k<=1; k++)\n";
  shader += "   {\n";
  shader += "     int z1 = z+k;\n";
  shader += "     int row = z1/ncols;\n";
  shader += "     int col = z1 - row*ncols;\n";
  shader += "     row *= gridy;\n";
  shader += "     col *= gridx;\n";
  shader += "     for(int i=-1; i<=1; i++)\n";
  shader += "     for(int j=-1; j<=1; j++)\n";
  shader += "      {\n";
  shader += "        vec3 dr = vec3(i,j,k);\n";
  shader += "        float len = length(dr);\n";
  shader += "        if (len > 0.5)\n";
  shader += "          {\n";
  shader += "            dr /= vec3(len,len,len);\n";  
  shader += "            float dotdl = dot(dr,ldir);\n";
  shader += "            if (dotdl > cangle)\n"; // angle less than cangle
  shader += "              {\n";
  shader += "                float x1 = float(col+x+i)+0.5;\n";
  shader += "                float y1 = float(row+y+j)+0.5;\n";
  shader += "                vec2 ldop = texture2DRect(lightTex, vec2(x1,y1)).xy;\n";
  shader += "                fop += vec2(dotdl, dotdl*(1.0-ldop.y)*ldop.x);\n";
  shader += "              }\n";
  shader += "           }\n";
  shader += "       }\n";
  shader += "   }\n";

  // divide by total weight
  shader += "  dop.x = fop.y/fop.x;\n";
  shader += "  dop.x = max(2.0/255.0, dop.x);\n";

  shader += "  gl_FragColor = vec4(dop,1.0,1.0);\n";
  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genFinalLightShader()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform vec3 lcol;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec3 light = lcol*texture2DRect(lightTex, gl_TexCoord[0].xy).x;\n";
  shader += "  gl_FragColor = vec4(light, 1.0);\n";
  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0), vec4(1.0));\n";
  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genInvertLightShader()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect lightTex;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec3 light = texture2DRect(lightTex, gl_TexCoord[0].xy).rgb;\n";
  shader += "  gl_FragColor = vec4(vec3(1.0)-light, 1.0);\n";
  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genDiffuseLightShader()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform float boost;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";  
  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";
  shader += "  int col = int(tc.x)/gridx;\n";
  shader += "  int row = int(tc.y)/gridy;\n";
  shader += "  int x = int(tc.x) - col*gridx;\n";
  shader += "  int y = int(tc.y) - row*gridy;\n";
  shader += "  int z = row*ncols + col;\n";

//  shader += "  vec4 fop = vec4(0.0,0.0,0.0,0.0);\n";
//  shader += "  for(int k=-1; k<=1; k++)\n";
//  shader += "   {\n";
//  shader += "    int z1 = z+k;\n";
//  shader += "    int row = z1/ncols;\n";
//  shader += "    int col = z1 - row*ncols;\n";
//  shader += "    row *= gridy;\n";
//  shader += "    col *= gridx;\n";
//  shader += "    for(int i=-1; i<=1; i++)\n";
//  shader += "    for(int j=-1; j<=1; j++)\n";
//  shader += "     {\n";
//  shader += "       float x1 = float(col+x+i)+0.5;\n";
//  shader += "       float y1 = float(row+y+j)+0.5;\n";
//  shader += "       fop += texture2DRect(lightTex, vec2(x1,y1));\n";
//  shader += "     }\n";
//  shader += "   }\n";
//
//  shader += "  fop.rgb *= boost;\n";
//  shader += "  gl_FragColor = vec4(fop.rgb/27.0, 1.0);\n";

  shader += "  vec4 fop = vec4(0.0,0.0,0.0,0.0);\n";
  // -- take contributions from left, right, front and back
  shader += "  row = z/ncols;\n";
  shader += "  col = z - row*ncols;\n";
  shader += "  row *= gridy;\n";
  shader += "  col *= gridx;\n";
  shader += "  for(int i=-1; i<=1; i+=2)\n";
  shader += "  for(int j=-1; j<=1; j+=2)\n";
  shader += "   {\n";
  shader += "     float x1 = float(col+x+i)+0.5;\n";
  shader += "     float y1 = float(row+y+j)+0.5;\n";
  shader += "     vec2 ldop = texture2DRect(lightTex, vec2(x1,y1)).xy;\n";
  shader += "     fop += texture2DRect(lightTex, vec2(x1,y1));\n";
  shader += "   }\n";
  // -- take contributions from top and bottom
  shader += "  for(int k=-1; k<=1; k+=2)\n";
  shader += "   {\n";
  shader += "    int z1 = z+k;\n";
  shader += "    row = z1/ncols;\n";
  shader += "    col = z1 - row*ncols;\n";
  shader += "    row *= gridy;\n";
  shader += "    col *= gridx;\n";
  shader += "    float x1 = float(col+x)+0.5;\n";
  shader += "    float y1 = float(row+y)+0.5;\n";
  shader += "    fop += texture2DRect(lightTex, vec2(x1,y1));\n";
  shader += "   }\n";
  shader += "  fop.rgb *= boost;\n";
  shader += "  fop.rgb = clamp(fop.rgb/6.0, vec3(0.0), vec3(1.0));\n";
  shader += "  gl_FragColor = vec4(fop.rgb, 1.0);\n";

  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genInitTubeLightShader() // point shader
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect opTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int npts;\n";
  shader += "uniform vec3 lpos[200];\n";
  shader += "uniform float lradius;\n";
  shader += "uniform float ldecay;\n";
  shader += "uniform float oplod;\n";
  shader += "uniform int opgridx;\n";
  shader += "uniform int opgridy;\n";
  shader += "uniform int opncols;\n";
  shader += "uniform bool doshadows;\n";
  shader += "uniform float opmod;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";

  // we are in lighttexture space
  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";
  shader += "  int col = int(tc.x)/gridx;\n";
  shader += "  int row = int(tc.y)/gridy;\n";
  shader += "  float x = tc.x - float(col*gridx);\n";
  shader += "  float y = tc.y - float(row*gridy);\n";
  shader += "  float z = float(row*ncols + col);\n";
  shader += "  vec3 p = vec3(x,y,z);\n";

  // convert x,y,z to optexture space
  shader += "  float xo = x/oplod;\n";
  shader += "  float yo = y/oplod;\n";
  shader += "  float zo = z/oplod;\n";
  shader += "  int oprow = int(zo)/opncols;\n";
  shader += "  int opcol = int(zo) - oprow*opncols;\n";
  shader += "  oprow *= opgridy;\n";
  shader += "  opcol *= opgridx;\n";
  shader += "  vec2 optc = vec2(float(opcol)+xo, float(oprow)+yo);\n";

  shader += "  float op = texture2DRect(opTex, optc).z;\n";   
  shader += "  op = clamp(opmod*op, 0.0, 0.95);\n";
  shader += "  gl_FragColor = vec4(0.0,op,opmod,1.0);\n";


  // ----- ambient occlusion ----
  shader += "  if (lradius < 1.0)\n";
  shader += "     {\n";
  shader += "       vec3 ig = vec3(2.5);\n";
  shader += "       bvec3 spless = lessThan(p, ig);\n";
  shader += "       bvec3 spgret = greaterThan(p, vec3(gridx,gridy,gridz)-ig);\n";
  shader += "       if (any(spless) || any(spgret))\n";
  shader += "       {\n";
  shader += "         gl_FragColor = vec4(1.0,op,opmod,1.0);\n";
//  shader += "         if (op > 0.01)\n";
//  shader += "           gl_FragColor = vec4(0.5,op,opmod,1.0);\n";
  shader += "         return;\n";
  shader += "       }\n";
  //shader += "       if (any(spless) || any(spgret) || op < 0.0001)\n";
  shader += "       if (op < 0.0001)\n";
  shader += "         gl_FragColor = vec4(1.0,op,opmod,1.0);\n";
  shader += "       return;\n";
  shader += "     }\n";
  //-----------------------------


  //------------------------------------
  //------------------------------------
  //------------------------------------
  shader += "  float den = 0.0;\n";
  shader += "  for(int pl=0; pl<npts; pl++)\n";
  shader += "   {\n";
  shader += "     float len = distance(p,lpos[pl]);\n";
  shader += "     if (len < lradius) \n";
  shader += "       {\n";
  shader += "          gl_FragColor = vec4(1.0,op,1.0,1.0);\n";
  shader += "          return;\n";
  shader += "       }\n";
  shader += "     else\n";
  shader += "       {\n";
  shader += "         bvec3 pless = lessThan(lpos[pl], vec3(0.5,0.5,0.5));\n";
  shader += "         bvec3 pgret = greaterThan(lpos[pl], vec3(float(gridx)-1.5,float(gridy)-1.5,float(gridz)-1.5));\n";
    // if light is outside the box
  shader += "         if (!doshadows || any(pless) || any(pgret)) \n";
  shader += "          {\n";
  shader += "            vec3 ldir = normalize(lpos[pl]-p);\n";
  shader += "            vec3 pos = p + 2.0*ldir;\n";
  shader += "            bvec3 spless = lessThan(pos, vec3(0.0,0.0,0.0));\n";
  shader += "            bvec3 spgret = greaterThan(pos, vec3(float(gridx)-1.5,float(gridy)-1.5,float(gridz)-1.5));\n";
 // light border voxels and interior as well in case doshadows is false
  shader += "            if (!doshadows || any(spless) || any(spgret)) \n";
  shader += "             {\n";
  shader += "               len -= lradius;\n";
  shader += "               den = max(den, pow(ldecay, len));\n"; // apply appropriate decay
  shader += "             }\n";
  shader += "          }\n";
  shader += "       }\n";  
  shader += "   }\n";
  shader += "   gl_FragColor = vec4(den,op,opmod,1.0);\n";
  

  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genTubeLightShader() // point shader
{
  QString shader;

  shader += "#version 420\n";
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int npts;\n";
  shader += "uniform vec3 lpos[200];\n";
  shader += "uniform float lradius;\n";
  shader += "uniform float ldecay;\n";
  shader += "uniform float cangle;\n";
  
  shader += "in vec3 glTexCoord0;\n";
  shader += "out vec4 glFragColor;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  //shader += "  vec2 tc = gl_TexCoord[0].xy;\n";
  shader += "  vec2 tc = glTexCoord0.xy;\n";
  shader += "  int col = int(tc.x)/gridx;\n";
  shader += "  int row = int(tc.y)/gridy;\n";
  shader += "  int x = int(tc.x) - col*gridx;\n";
  shader += "  int y = int(tc.y) - row*gridy;\n";
  shader += "  int z = row*ncols + col;\n";

  shader += "  glFragColor = texture2DRect(lightTex, tc.xy);\n";

 
  //----------------------------------------------
  //----------------------------------------------
  //-------- ambient occlusion ----
  shader += "  if (lradius < 1.0)\n";
  shader += "   {\n";
  shader += "     vec3 p = vec3(x,y,z);\n";
  shader += "     vec3 ig = vec3(1.1);\n";
  shader += "     bvec3 spless = lessThan(p, ig);\n";
  shader += "     bvec3 spgret = greaterThan(p, vec3(gridx,gridy,gridz)-ig-vec3(1.0));\n";
  shader += "     if (any(spless) || any(spgret)) return;\n";
  // -- not border, compute illumination
  shader += "     float nlit = 0.0;\n";
  shader += "     float lit = 0.0;\n";
  shader += "     float maxlit = 0.0;\n";
  shader += "     const ivec3 indices[14] = ivec3[14] (\n";
  shader += "                ivec3( 1, 0, 0), ivec3(-1, 0, 0),\n";
  shader += "                ivec3( 0, 1, 0), ivec3( 0,-1, 0),\n";
  shader += "                ivec3( 0, 0, 1), ivec3( 0, 0,-1),\n";
  shader += "                ivec3(-1,-1,-1), ivec3(-1, 1,-1),\n";
  shader += "                ivec3( 1,-1,-1), ivec3( 1, 1,-1),\n";
  shader += "                ivec3(-1,-1, 1), ivec3(-1, 1, 1),\n";
  shader += "                ivec3( 1,-1, 1), ivec3( 1, 1, 1));\n";
  shader += "     for(int i=0; i<14; i++)\n";
  shader += "     {\n";
  shader += "        int z1 = z+indices[i].z;\n";
  shader += "        int row = z1/ncols;\n";
  shader += "        int col = z1 - row*ncols;\n";
  shader += "        row *= gridy;\n";
  shader += "        col *= gridx;\n";
  shader += "        float x1 = float(col+x+indices[i].x)+0.5;\n";
  shader += "        float y1 = float(row+y+indices[i].y)+0.5;\n";
  shader += "        vec2 ldop = texture2DRect(lightTex, vec2(x1,y1)).xy;\n";
  shader += "        float illumination = (1.0-ldop.y)*ldop.x;\n";
  shader += "        nlit += step(0.001, illumination);\n";
  shader += "        lit += illumination;\n";
  shader += "        maxlit = max(illumination, maxlit);\n";
  shader += "     }\n";
  // -- merge all contributions
  shader += "     nlit = max(1.0, nlit);\n";
  shader += "     lit /= nlit;\n";
  shader += "     lit = mix(maxlit, lit, ldecay);\n";
  shader += "     glFragColor.x = clamp(lit, 0.0, 1.0);\n";
  shader += "     return;\n";
  shader += "   }\n";
  //----------------------------------------------
  //----------------------------------------------




  //----------------------------------------------
  //----------------------------------------------
  // ---- point/tube light -------------
  // cache the illumination around current voxel
  shader += "  float illum[27];\n";
  shader += "  int idx = 0;\n";
  shader += "  for(int i=-1; i<=1; i++)\n";
  shader += "  for(int j=-1; j<=1; j++)\n";
  shader += "  for(int k=-1; k<=1; k++)\n";
  shader += "  {\n";
  shader += "     int z1 = z+k;\n";
  shader += "     int row = z1/ncols;\n";
  shader += "     int col = z1 - row*ncols;\n";
  shader += "     row *= gridy;\n";
  shader += "     col *= gridx;\n";
  shader += "     float x1 = float(col+x+i)+0.5;\n";
  shader += "     float y1 = float(row+y+j)+0.5;\n";
  shader += "     vec2 ldop = texture2DRect(lightTex, vec2(x1,y1)).xy;\n";
  shader += "     illum[idx] = (1.0-ldop.y)*ldop.x;\n";
  shader += "     idx = idx + 1;\n";
  shader += "  }\n";
  

  shader += "  vec2 dop = texture2DRect(lightTex, tc.xy).xy;\n";
  shader += "  if (dop.x > 0.98) return;\n";

  shader += "  vec3 p = vec3(x,y,z);\n";
  shader += "  dop.x = 0.0;\n";
  shader += "  for(int pl=0; pl<npts; pl++)\n";
  shader += "   {\n";
  shader += "     vec3 ldir = normalize(lpos[pl]-p);\n";
  shader += "     vec2 fop = vec2(0.0,0.0);\n";
  shader += "     int idx = 0;\n";
  shader += "     for(int i=-1; i<=1; i++)\n";
  shader += "     for(int j=-1; j<=1; j++)\n";
  shader += "     for(int k=-1; k<=1; k++)\n";
  shader += "      {\n";
  shader += "        vec3 dr = p-vec3(x+i,y+j,z+k);\n";
  shader += "        float len = length(dr);\n";
  shader += "        if (len > 0.1)\n";
  shader += "          {\n";
  shader += "            dr = normalize(dr);\n";  
  shader += "            float dotdl = dot(dr,ldir);\n";
  shader += "            if (dotdl < cangle)\n"; // angle less than cangle
  shader += "              {\n";
  shader += "                fop += vec2(dotdl, dotdl*illum[idx]);\n";
  shader += "              }\n";
  shader += "          }\n"; // len > 0.1
  shader += "        idx = idx + 1;\n";
  shader += "      }\n"; // k loop
  shader += "      dop.x = max(dop.x, (ldecay*fop.y/fop.x));\n";
  shader += "   }\n"; // pl loop
  shader += "  dop.x = clamp(dop.x, 2.0/255.0, 1.0);\n";
  shader += "  glFragColor.x = dop.x;\n";
  //----------------------------------------------
  //----------------------------------------------

  
  
  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genExpandLightShader()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int ncols;\n";

  shader += "uniform float llod;\n";
  shader += "uniform int lgridx;\n";
  shader += "uniform int lgridy;\n";
  shader += "uniform int lncols;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";

  // we are in finallighttexture space
  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";
  shader += "  int col = int(tc.x)/gridx;\n";
  shader += "  int row = int(tc.y)/gridy;\n";
  shader += "  float x = tc.x - float(col*gridx);\n";
  shader += "  float y = tc.y - float(row*gridy);\n";
  shader += "  float z = float(row*ncols + col);\n";
  
//  // convert x,y,z to lighttexture space
//  shader += "  x *= llod;\n";
//  shader += "  y *= llod;\n";
//  shader += "  z *= llod;\n";
//  shader += "  int lrow = int(z)/lncols;\n";
//  shader += "  int lcol = int(z) - lrow*lncols;\n";
//  shader += "  lrow *= lgridy;\n";
//  shader += "  lcol *= lgridx;\n";
//  shader += "  vec2 ltc = vec2(float(lcol)+x, float(lrow)+y);\n";
//
//  shader += "  gl_FragColor = vec4(texture2DRect(lightTex, ltc).rgb, 1.0);\n";   


  shader += "  float xO = x*float(llod);\n";
  shader += "  float yO = y*float(llod);\n";
  shader += "  float zO = z*float(llod);\n";

  shader += "int alod = int(llod)-1;\n";
  shader += "vec4 fcolor = vec4(0.0,0.0,0.0,0.0);\n";
  shader += "for(int za=-alod; za<=alod; za++)\n";
  shader += "{\n";
  shader += "  float z = zO + float(za);\n";
  shader += "  int lrow = int(z)/lncols;\n";
  shader += "  int lcol = int(z) - lrow*lncols;\n";
  shader += "  lrow *= lgridy;\n";
  shader += "  lcol *= lgridx;\n";
  shader += "  for(int xa=-alod; xa<=alod; xa++)\n";
  shader += "  for(int ya=-alod; ya<=alod; ya++)\n";
  shader += "  {\n";
  shader += "    float x = xO + float(xa);\n";
  shader += "    float y = yO + float(ya);\n";
  shader += "    vec2 ltc = vec2(float(lcol)+x, float(lrow)+y);\n";
  shader += "    fcolor += vec4(texture2DRect(lightTex, ltc).rgb, 1.0);\n";   
  shader += "  }\n";
  shader += "}\n";
  
  //shader += " gl_FragColor = fcolor/pow(2.0*(float)alod+1, 3.0);\n";
  shader += " gl_FragColor = fcolor/pow(2.0*float(alod)+1.0, 3.0);\n";

  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::blend(QString blendShader)
{
  QString shader;
  
  shader = "#version 450 core\n";
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect opTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int lod;\n";
  shader += "uniform vec3 voxelScaling;\n";
  shader += "uniform vec3 dmin;\n";
  shader += "uniform sampler2DRect lightTex;\n";
  shader += "uniform sampler2D lutTex;\n";

  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 dirUp;\n";
  shader += "uniform vec3 dirRight;\n";

  shader += blendShader;

  shader += "in vec3 glTexCoord0;\n";
  shader += "out vec4 glFragColor;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  int ocol = int(glTexCoord0.x)/gridx;\n";
  shader += "  int orow = int(glTexCoord0.y)/gridy;\n";
  shader += "  int ox = int(glTexCoord0.x) - ocol*gridx;\n";
  shader += "  int oy = int(glTexCoord0.y) - orow*gridy;\n";
  shader += "  int oz = orow*ncols + ocol;\n";

  shader += "  vec4 op = texture2DRect(opTex, glTexCoord0.xy);\n";
  shader += "  vec2 vg = op.xy;\n";

  shader += "  vec3 v = vec3(ox,oy,oz);\n";
  shader += "  v = v * vec3(lod,lod,lod);\n";
  shader += "  v = v + dmin;\n";

  shader += "  vec4 fcol = op.bbbb;\n";
  shader += "  blend(false, v, vg, fcol);\n";
  shader += "  op.rgb = fcol.aaa;\n";

  shader += "  op.rgb *= texture2DRect(lightTex, glTexCoord0.xy).xxx;\n";
  shader += "  glFragColor = op;\n";

  shader += "}\n";

  return shader;
}

