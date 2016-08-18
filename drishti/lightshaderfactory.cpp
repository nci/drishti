#include "global.h"
#include "lightshaderfactory.h"

QString
LightShaderFactory::genOpacityShader(bool bit16)
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect dragTex;\n";
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

  shader += "void main(void)\n";
  shader += "{\n";

  // we are in lighttexture space
  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";
  shader += "  int lcol = int(tc.x)/lgridx;\n";
  shader += "  int lrow = int(tc.y)/lgridy;\n";
  shader += "  float x = tc.x - float(lcol*lgridx);\n";
  shader += "  float y = tc.y - float(lrow*lgridy);\n";
  shader += "  float z = float(lrow*lncols + lcol);\n";

  //-----------------------------------------
  // set border voxels to 0 opacity
  {
    shader += "  vec3 pos = vec3(x,y,z);\n";
    shader += "  bvec3 pless = lessThan(pos, vec3(1.5,1.5,1.5));\n";
    shader += "  bvec3 pgret = greaterThan(pos, vec3(float(lgridx)-2.5,float(lgridy)-2.5,float(lgridz)-2.5));\n";
    shader += "  if (any(pless) || any(pgret)) \n";
    shader += "    { gl_FragColor = vec4(0.0,0.0,0.0,0.0); return; }\n";
  }
  //-----------------------------------------

  shader += "  x *= float(llod);\n";
  shader += "  y *= float(llod);\n";
  shader += "  z *= float(llod);\n";
  
  // convert x,y,z to dragtexture space
  shader += "  int row = int(z)/ncols;\n";
  shader += "  int col = int(z) - row*ncols;\n";
  shader += "  row *= gridy;\n";
  shader += "  col *= gridx;\n";
  shader += "  tc = vec2(float(col)+x, float(row)+y);\n";

  shader += "  vec2 vg;\n";
  shader += "  vg.x = texture2DRect(dragTex, tc.xy).x;\n";

  if (!bit16)
    {
      shader += "  vec3 sample1, sample2;\n";

      shader += "  float x1 = max(0.0, float(x-1.0));\n";
      shader += "  sample1.x = texture2DRect(dragTex, vec2(float(col)+x1, float(row)+y)).x;\n";
      shader += "  x1 = min(float(gridx-1), float(x+1.0));\n";
      shader += "  sample2.x = texture2DRect(dragTex, vec2(float(col)+x1, float(row)+y)).x;\n";

      shader += "  float y1 = max(0.0, float(y-1.0));\n";
      shader += "  sample1.y = texture2DRect(dragTex, vec2(float(col)+x, float(row)+y1)).x;\n";
      shader += "  y1 = min(float(gridy-1), float(y+1.0));\n";
      shader += "  sample2.y = texture2DRect(dragTex, vec2(float(col)+x, float(row)+y1)).x;\n";

      shader += "  int z1 = int(max(0.0, float(z-1.0)));\n";
      shader += "  row = z1/ncols;\n";
      shader += "  col = z1 - row*ncols;\n";
      shader += "  row *= gridy;\n";
      shader += "  col *= gridx;\n";
      shader += "  sample1.z = texture2DRect(dragTex, vec2(float(col)+x, float(row)+y)).x;\n";
      shader += "  z1 = int(min(float(gridz-1), float(z+1.0)));\n";
      shader += "  row = z1/ncols;\n";
      shader += "  col = z1 - row*ncols;\n";
      shader += "  row *= gridy;\n";
      shader += "  col *= gridx;\n";
      shader += "  sample2.z = texture2DRect(dragTex, vec2(float(col)+x, float(row)+y)).x;\n";
    
      shader += "  vg.y = distance(sample1, sample2);\n";
    }
  else
    {
      shader += "int h0 = int(65535.0*vg.x);\n";
      shader += "int h1 = h0 / 256;\n";
      shader += "h0 = int(mod(float(h0),256.0));\n";
      shader += "float fh0 = float(h0)/256.0;\n";
      shader += "float fh1 = float(h1)/256.0;\n";

      shader += "vg.xy = vec2(fh0, fh1);\n";
    }

  shader += "float val = vg.x;\n";
  //shader += "float grad = vg.y;\n";

  //shader += QString("  vg.y = tfSet + vg.y*%1;\n").arg(1.0/Global::lutSize());

  shader += QString("float grad = vg.y*%1;\n").arg(1.0/Global::lutSize());
  shader += "vg.y = tfSet + grad;\n";

  shader += " if (opshader)\n";
  //shader += "   gl_FragColor = texture2D(lutTex, vg.xy).aaaa;\n";  
  shader += "   {\n";
  shader += "     float alpha = texture2D(lutTex, vg.xy).a;\n";
  shader += "     gl_FragColor = vec4(val, grad, alpha, 1.0);\n"; 
  shader += "   }\n";
  shader += " else\n";
  shader += "   {\n";
  shader += "     gl_FragColor = texture2D(lutTex, vg.xy);\n";  
  shader += "     gl_FragColor.rgb *= gl_FragColor.a;\n";  
  shader += "     gl_FragColor.a = step(0.01, gl_FragColor.a);\n";
  shader += "   }\n";

  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genOpacityShader2(int nvol)
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
  shader += "uniform int ncols;\n";
  shader += "uniform int llod;\n";
  shader += "uniform int lgridx;\n";
  shader += "uniform int lgridy;\n";
  shader += "uniform int lgridz;\n";
  shader += "uniform int lncols;\n";
  shader += "uniform bool opshader;\n";
  shader += "uniform float tfSet;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += vstr + " samplex1, samplex2;\n";
  shader += vstr + " sampley1, sampley2;\n";
  shader += vstr + " samplez1, samplez2;\n";
  shader += vstr + " val;\n";
  shader += "  float g;\n";
  shader += "  vec3 sample1, sample2;\n";

  // we are in lighttexture space
  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";
  shader += "  int lcol = int(tc.x)/lgridx;\n";
  shader += "  int lrow = int(tc.y)/lgridy;\n";
  shader += "  float x = tc.x - float(lcol*lgridx);\n";
  shader += "  float y = tc.y - float(lrow*lgridy);\n";
  shader += "  float z = float(lrow*lncols + lcol);\n";

  //-----------------------------------------
  // set every border voxel to 0
  {
    shader += "  vec3 pos = vec3(x,y,z);\n";
    shader += "  bvec3 pless = lessThan(pos, vec3(1.5,1.5,1.5));\n";
    shader += "  bvec3 pgret = greaterThan(pos, vec3(float(lgridx)-2.5,float(lgridy)-2.5,float(lgridz)-2.5));\n";
    shader += "  if (any(pless) || any(pgret)) \n";
    shader += "    { gl_FragColor = vec4(0.0,0.0,0.0,0.0); return; }\n";
  }
  //-----------------------------------------

  shader += "  x *= float(llod);\n";
  shader += "  y *= float(llod);\n";
  shader += "  z *= float(llod);\n";
  
  // convert x,y,z to dragtexture space
  shader += "  int row = int(z)/ncols;\n";
  shader += "  int col = int(z) - row*ncols;\n";
  shader += "  row *= gridy;\n";
  shader += "  col *= gridx;\n";
  shader += "  tc = vec2(float(col)+x, float(row)+y);\n";

  shader += "  float x1 = max(0.0, x-1.0);\n";
  shader += "  samplex1 = texture2DRect(dragTex, vec2(float(col)+x1, float(row)+y))."+xyzw+";\n";
  shader += "  x1 = min(float(gridx-1), x+1.0);\n";
  shader += "  samplex2 = texture2DRect(dragTex, vec2(float(col)+x1, float(row)+y))."+xyzw+";\n";

  shader += "  float y1 = max(0.0, y-1.0);\n";
  shader += "  sampley1 = texture2DRect(dragTex, vec2(float(col)+x, float(row)+y1))."+xyzw+";\n";
  shader += "  y1 = min(float(gridy-1), y+1.0);\n";
  shader += "  sampley2 = texture2DRect(dragTex, vec2(float(col)+x, float(row)+y1))."+xyzw+";\n";

  shader += "  int z1 = int(max(0.0, z-1.0));\n";
  shader += "  row = z1/ncols;\n";
  shader += "  row *= gridy;\n";
  shader += "  col = z1 - row*ncols;\n";
  shader += "  col *= gridx;\n";
  shader += "  samplez1 = texture2DRect(dragTex, vec2(float(col)+x, float(row)+y))."+xyzw+";\n";
  shader += "  z1 = int(min(float(gridz-1), z+1.0));\n";
  shader += "  row = z1/ncols;\n";
  shader += "  row *= gridy;\n";
  shader += "  col =z1 - row*ncols;\n";
  shader += "  col *= gridx;\n";
  shader += "  samplez2 = texture2DRect(dragTex, vec2(float(col)+x, float(row)+y))."+xyzw+";\n";

  shader += "  val = texture2DRect(dragTex, tc.xy)."+xyzw+";\n";

  shader += "  float alpha = 0.0;\n";
  shader += "  float totalpha = 0.0;\n";
  shader += "  vec3 rgb = vec3(0.0,0.0,0.0);\n";
  shader += "  vec4 color;\n";

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
      //shader += QString("  g = (float(%1) + g)/float(%2);\n").arg(i-1).arg(nvol);
      shader += QString("  g = tfSet + (float(%1) + g)/float(%2);\n").arg(i-1).arg(Global::lutSize());
      shader += QString("  color = texture2D(lutTex, vec2(val.%1,g));\n").arg(c);
      shader += "  rgb += color.rgb;\n";
      shader += "  alpha = max(alpha, color.a);\n";
      shader += "  totalpha += color.a;\n";
    }
  
  shader += "  gl_FragColor.a = alpha;\n";
  shader += "  gl_FragColor.rgb = alpha*rgb/totalpha;\n";

  shader += " if (opshader)\n";
  shader += "   gl_FragColor = gl_FragColor.aaaa;\n";  
  shader += " else\n";
  shader += "   {\n";
  shader += "     gl_FragColor.rgb *= gl_FragColor.a;\n";  
  shader += "     gl_FragColor.a = step(0.01, gl_FragColor.a);\n";
  shader += "   }\n";

  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genOpacityShaderRGB()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect dragTex;\n";
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
  
  shader += "void main(void)\n";
  shader += "{\n";

  // we are in lighttexture space
  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";
  shader += "  int lcol = int(tc.x)/lgridx;\n";
  shader += "  int lrow = int(tc.y)/lgridy;\n";
  shader += "  float x = tc.x - float(lcol*lgridx);\n";
  shader += "  float y = tc.y - float(lrow*lgridy);\n";
  shader += "  float z = float(lrow*lncols + lcol);\n";
  shader += "  x *= float(llod);\n";
  shader += "  y *= float(llod);\n";
  shader += "  z *= float(llod);\n";
  
  // convert x,y,z to dragtexture space
  shader += "  int row = int(z)/ncols;\n";
  shader += "  int col = int(z) - row*ncols;\n";
  shader += "  row *= gridy;\n";
  shader += "  col *= gridx;\n";
  shader += "  tc = vec2(float(col)+x, float(row)+y);\n";

  shader += "  vec4 colOp = vec4(texture2DRect(dragTex, tc).rgb, 1.0);\n";

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

  shader += "  gl_FragColor.rgba = vec4(colOp.rgb,1.0)*colOp.a;\n";

  shader += " if (opshader)\n";
  shader += "   gl_FragColor = gl_FragColor.aaaa;\n";  
  shader += " else\n";
  shader += "   gl_FragColor.a = step(0.01, gl_FragColor.a);\n";

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

  shader += "  float op = texture2DRect(opTex, tc).x;\n"; 
  shader += "  op = clamp(opmod*op, 0.0, 1.0);\n";

  shader += "  vec3 pos = vec3(x,y,z);\n";
  shader += "  bvec3 pless = lessThan(pos, vec3(0.5,0.5,0.5));\n";
  shader += "  bvec3 pgret = greaterThan(pos, vec3(float(gridx)-1.5,float(gridy)-1.5,float(gridz)-1.5));\n";
  shader += "  if (any(pless) || any(pgret)) \n";
  shader += "  { gl_FragColor = vec4(1.0,op,1.0,1.0); return; }\n";

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
  shader += "        float op = texture2DRect(opTex, vec2(x1,y1)).x;\n"; 
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
  shader += "  float op = opmod*texture2DRect(opTex, tc).x;\n"; 
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
  shader += "  int col = int(tc.x)/gridx;\n";
  shader += "  int row = int(tc.y)/gridy;\n";
  shader += "  int x = int(tc.x) - col*gridx;\n";
  shader += "  int y = int(tc.y) - row*gridy;\n";
  shader += "  int z = row*ncols + col;\n";

  shader += "  gl_FragColor = texture2DRect(lightTex, tc.xy);\n";

  shader += "  float nlit = 0.0;\n";
  shader += "  float lit = 0.0;\n";

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
  shader += "     float alit = (1.0-ldop.y)*ldop.x;\n";
  shader += "     nlit += step(0.001, alit);\n";
  shader += "     lit += alit;\n";
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
  shader += "    vec2 ldop = texture2DRect(lightTex, vec2(x1,y1)).xy;\n";
  shader += "    float alit = (1.0-ldop.y)*ldop.x;\n";
  shader += "    nlit += step(0.001, alit);\n";
  shader += "    lit += alit;\n";
  shader += "   }\n";

  shader += "  nlit = max(1.0, nlit);\n";
  shader += "  gl_FragColor.x = clamp(lit/nlit, 0.0, 1.0);\n";

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
  shader += "  vec4 op = texture2DRect(opTex, gl_TexCoord[0].xy).bbbb;\n";
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

  shader += "  float op = texture2DRect(opTex, optc).x;\n";   
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
  //shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";
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
  shader += "  vec3 light = texture2DRect(lightTex, gl_TexCoord[0].xy);\n";
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

  shader += "  float op = texture2DRect(opTex, optc).x;\n";   
  shader += "  op = clamp(opmod*op, 0.0, 1.0);\n";
  shader += "  gl_FragColor = vec4(0.0,op,0.0,1.0);\n";


  // ----- testing ----
  shader += "  if (lradius < 1)\n";
  shader += "     {\n";
  shader += "       bvec3 spless = lessThan(p, vec3(2.0,2.0,2.0));\n";
  shader += "       bvec3 spgret = greaterThan(p, vec3(float(gridx)-2.0,float(gridy)-2.0,float(gridz)-2.0));\n";
  shader += "       if (any(spless) || any(spgret))\n";
  shader += "          gl_FragColor = vec4(1.0,op,1.0,1.0);\n";
  shader += "       return;\n";
  shader += "     }\n";
  //-------------------


  //------------------------------------
  // for point group lights - find nearest light
  shader += "  vec3 closestpt=lpos[0];\n";
  shader += "  float closestdist=distance(p,lpos[0]);\n";
  shader += "  for(int i=1; i<npts; i++)\n";
  shader += "   {\n";
  shader += "     float dist = distance(p,lpos[i]);\n";
  shader += "     if (dist < closestdist)\n";
  shader += "      {\n";
  shader += "        closestpt = lpos[i];\n";
  shader += "        closestdist = dist;\n";
  shader += "      }\n";
  shader += "   }\n";
  //------------------------------------


  //------------------------------------
  shader += "   float len = closestdist;\n";
  shader += "   if (len < lradius) \n";
  shader += "     {\n";
  shader += "        gl_FragColor = vec4(1.0,op,1.0,1.0);\n";
  shader += "        return;\n";
  shader += "     }\n";
  shader += "   else\n";
  shader += "     {\n";
  shader += "       bvec3 pless = lessThan(closestpt, vec3(0.5,0.5,0.5));\n";
  shader += "       bvec3 pgret = greaterThan(closestpt, vec3(float(gridx)-1.5,float(gridy)-1.5,float(gridz)-1.5));\n";
    // if light is outside the box
  shader += "       if (!doshadows || any(pless) || any(pgret)) \n";
  shader += "        {\n";
  shader += "          vec3 ldir = normalize(closestpt-p);\n";
  shader += "          vec3 pos = p + 2.0*ldir;\n";
  shader += "          bvec3 spless = lessThan(pos, vec3(0.0,0.0,0.0));\n";
  shader += "          bvec3 spgret = greaterThan(pos, vec3(float(gridx)-1.5,float(gridy)-1.5,float(gridz)-1.5));\n";
 // light border voxels and interior as well in case doshadows is false
  shader += "          if (!doshadows || any(spless) || any(spgret)) \n";
  shader += "           {\n";
  shader += "             len -= lradius;\n";
  shader += "             float den = pow(ldecay, len);\n"; // apply appropriate decay
  shader += "             gl_FragColor = vec4(den,op,den,1.0);\n";
  shader += "           }\n";
  shader += "        }\n";
  shader += "     }\n";
  //------------------------------------

//  //------------------------------------
//  // for tube light
//  shader += "  for(int i=0; i<npts-1; i++)\n";
//  shader += "   {\n";
//  shader += "     vec3 p0 = p-lpos[i];\n";
//  shader += "     vec3 l0 = lpos[i+1]-lpos[i];\n";
//  shader += "     float len = length(l0);\n";
//  shader += "     l0 /= vec3(len,len,len);\n";
//  shader += "     float pl = dot(p0, l0);\n";
//  shader += "     if (pl <= len)\n";
//  shader += "      {\n";
//  shader += "        vec3 lp = lpos[i]+pl*l0;\n";
//  shader += "        if (distance(lp, vec3(x,y,z)) < lradius) \n";
//  shader += "        {\n";
//  shader += "          gl_FragColor = vec4(1.0,op,1.0,1.0);\n";
//  shader += "          return;\n";
//  shader += "        }\n";
//  shader += "      }\n";
//  shader += "   }\n";
//  //------------------------------------

  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::genTubeLightShader() // point shader
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
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
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec2 tc = gl_TexCoord[0].xy;\n";
  shader += "  int col = int(tc.x)/gridx;\n";
  shader += "  int row = int(tc.y)/gridy;\n";
  shader += "  int x = int(tc.x) - col*gridx;\n";
  shader += "  int y = int(tc.y) - row*gridy;\n";
  shader += "  int z = row*ncols + col;\n";

  //-------- skylight ----
  shader += "  if (lradius < 1)\n";
  shader += "   {\n";
  shader += "     gl_FragColor = texture2DRect(lightTex, tc.xy);\n";
  shader += " vec3 p = vec3(x,y,z);\n";
  shader += " bvec3 spless = lessThan(p, vec3(2.0,2.0,2.0));\n";
  shader += " bvec3 spgret = greaterThan(p, ";
  shader += "vec3(float(gridx)-2.0,float(gridy)-2.0,float(gridz)-2.0));\n";
  shader += " if (any(spless) || any(spgret)) return;\n";

  shader += "     float nlit = 0.0;\n";
  shader += "     float lit = 0.0;\n";
  // -- take contributions from left, right, front and back
  shader += "       int row = z/ncols;\n";
  shader += "       int col = z - row*ncols;\n";
  shader += "       row *= gridy;\n";
  shader += "       col *= gridx;\n";
  shader += "       for(int i=-1; i<=1; i+=2)\n";
  shader += "       for(int j=-1; j<=1; j+=2)\n";
  shader += "        {\n";
  shader += "          float x1 = float(col+x+i)+0.5;\n";
  shader += "          float y1 = float(row+y+j)+0.5;\n";
  shader += "          vec2 ldop = texture2DRect(lightTex, vec2(x1,y1)).xy;\n";
  shader += "          float alit = (1.0-ldop.y)*ldop.x;\n";
  shader += "          nlit += step(0.001, alit);\n";
  shader += "          lit += alit;\n";
  shader += "        }\n";
  // -- take contributions from top and bottom
  shader += "     for(int k=-1; k<=1; k+=2)\n";
  shader += "      {\n";
  shader += "       int z1 = z+k;\n";
  shader += "       int row = z1/ncols;\n";
  shader += "       int col = z1 - row*ncols;\n";
  shader += "       row *= gridy;\n";
  shader += "       col *= gridx;\n";
  shader += "       float x1 = float(col+x)+0.5;\n";
  shader += "       float y1 = float(row+y)+0.5;\n";
  shader += "       vec2 ldop = texture2DRect(lightTex, vec2(x1,y1)).xy;\n";
  shader += "       float alit = (1.0-ldop.y)*ldop.x;\n";
  shader += "       nlit += step(0.001, alit);\n";
  shader += "       lit += alit;\n";
  shader += "      }\n";
  shader += "     nlit = max(1.0, nlit);\n";
  shader += "     gl_FragColor.x = clamp(lit/(nlit*ldecay), 0.0, 1.0);\n";
  shader += "     return;\n";
  shader += "   }\n";
  //--------

  shader += "  vec2 dop = texture2DRect(lightTex, tc.xy).xy;\n";
  shader += "  gl_FragColor = vec4(dop,1.0,1.0);\n";
  shader += "  if (dop.x > 0.98) return;\n";

  shader += "  vec3 p = vec3(x,y,z);\n";

  //------------------------------------
  // for point group lights - find nearest light
  shader += "  vec3 closestpt=lpos[0];\n";
  shader += "  float closestdist=distance(p,lpos[0]);\n";
  shader += "  for(int i=1; i<npts; i++)\n";
  shader += "   {\n";
  shader += "     float dist = distance(p,lpos[i]);\n";
  shader += "     if (dist < closestdist)\n";
  shader += "      {\n";
  shader += "        closestpt = lpos[i];\n";
  shader += "        closestdist = dist;\n";
  shader += "      }\n";
  shader += "   }\n";
  //------------------------------------

  shader += "  vec3 ldir = normalize(closestpt-p);\n";
  shader += "  vec2 fop = vec2(0.0,0.0);\n";
  shader += "  for(int k=-1; k<=1; k++)\n";
  shader += "   {\n";
  shader += "    int z1 = z+k;\n";
  shader += "    int row = z1/ncols;\n";
  shader += "    int col = z1 - row*ncols;\n";
  shader += "    row *= gridy;\n";
  shader += "    col *= gridx;\n";
  shader += "    for(int i=-1; i<=1; i++)\n";
  shader += "    for(int j=-1; j<=1; j++)\n";
  shader += "     {\n";
  shader += "       vec3 dr = vec3(i,j,k);\n";
  shader += "       float len = length(dr);\n";
  shader += "       if (len > 0.1)\n";
  shader += "         {\n";
  shader += "           dr /= vec3(len,len,len);\n";  
  shader += "           float dotdl = dot(dr,ldir);\n";
  shader += "           if (dotdl > cangle)\n"; // angle less than cangle
  shader += "             {\n";
  shader += "               float x1 = float(col+x+i)+0.5;\n";
  shader += "               float y1 = float(row+y+j)+0.5;\n";
  shader += "               vec2 ldop = texture2DRect(lightTex, vec2(x1,y1)).xy;\n";
  shader += "               fop += vec2(dotdl, dotdl*(1.0-ldop.y)*ldop.x);\n";
  shader += "             }\n";
  shader += "         }\n";
  shader += "     }\n";
  shader += "   }\n";
  shader += "  dop.x = (ldecay*fop.y/fop.x);\n";
  shader += "  dop.x = clamp(dop.x, 2.0/255.0, 1.0);\n";
  shader += "  gl_FragColor = vec4(dop,1.0,1.0);\n";

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
  
  // convert x,y,z to lighttexture space
  shader += "  x *= llod;\n";
  shader += "  y *= llod;\n";
  shader += "  z *= llod;\n";
  shader += "  int lrow = int(z)/lncols;\n";
  shader += "  int lcol = int(z) - lrow*lncols;\n";
  shader += "  lrow *= lgridy;\n";
  shader += "  lcol *= lgridx;\n";
  shader += "  vec2 ltc = vec2(float(lcol)+x, float(lrow)+y);\n";


  shader += "  gl_FragColor = vec4(texture2DRect(lightTex, ltc).rgb, 1.0);\n";   

  shader += "}\n";

  return shader;
}

QString
LightShaderFactory::blend(QString blendShader)
{
  QString shader;
  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
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
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "  int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "  int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "  int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "  int oz = orow*ncols + ocol;\n";

  shader += "  vec4 op = texture2DRect(opTex, gl_TexCoord[0].xy);\n";
  shader += "  vec2 vg = op.xy;\n";

  shader += "  vec3 v = vec3(ox,oy,oz);\n";
  shader += "  v = v * vec3(lod,lod,lod);\n";
  shader += "  v = v + dmin;\n";

  shader += "  vec4 fcol = op.bbbb;\n";
  shader += "  blend(false, v, vg, fcol);\n";
  shader += "  op.rgb = fcol.aaa;\n";

  shader += "  op.rgb *= texture2DRect(lightTex, gl_TexCoord[0].xy).xxx;\n";
  shader += "  gl_FragColor = op;\n";

  shader += "}\n";

  return shader;
}

