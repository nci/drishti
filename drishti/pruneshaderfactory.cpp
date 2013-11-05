#include "pruneshaderfactory.h"

QString
PruneShaderFactory::genPruneTexture(bool bit16)
{
  QString shader;

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
  shader += "  vec3 sample1, sample2;\n";
  shader += "  vec2 vg;\n";
  shader += "  int row, col;\n";
  shader += "  int x,y,z, x1,y1,z1;\n";

  shader += "  col = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "  row = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "  x = int(gl_TexCoord[0].x) - col*gridx;\n";
  shader += "  y = int(gl_TexCoord[0].y) - row*gridy;\n";
  shader += "  z = row*ncols + col;\n";
  shader += "  col *= gridx;\n";
  shader += "  row *= gridy;\n";

  shader += "  vg.x = texture2DRect(dragTex, gl_TexCoord[0].xy).x;\n";

  if (!bit16)
    {
      shader += "  x1 = int(max(0.0, float(x-1)));\n";
      shader += "  sample1.x = texture2DRect(dragTex, vec2(col+x1, row+y)).x;\n";
      shader += "  x1 = int(min(float(gridx-1), float(x+1)));\n";
      shader += "  sample2.x = texture2DRect(dragTex, vec2(col+x1, row+y)).x;\n";

      shader += "  y1 = int(max(0.0, float(y-1)));\n";
      shader += "  sample1.y = texture2DRect(dragTex, vec2(col+x, row+y1)).x;\n";
      shader += "  y1 = int(min(float(gridy-1), float(y+1)));\n";
      shader += "  sample2.y = texture2DRect(dragTex, vec2(col+x, row+y1)).x;\n";

      shader += "  z1 = int(max(0.0, float(z-1)));\n";
      shader += "  row = z1/ncols;\n";
      shader += "  col = z1 - row*ncols;\n";
      shader += "  row *= gridy;\n";
      shader += "  col *= gridx;\n";
      shader += "  sample1.z = texture2DRect(dragTex, vec2(col+x, row+y)).x;\n";
      shader += "  z1 = int(min(float(gridz-1), float(z+1)));\n";
      shader += "  row = z1/ncols;\n";
      shader += "  col = z1 - row*ncols;\n";
      shader += "  row *= gridy;\n";
      shader += "  col *= gridx;\n";
      shader += "  sample2.z = texture2DRect(dragTex, vec2(col+x, row+y)).x;\n";
    
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

  shader += "  float op = texture2D(lutTex, vg.xy).x;\n";
  shader += "  float s = step(0.9/255.0, op);\n";
  
  shader += "  gl_FragColor = vec4(s,op,0.0,1.0);\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::dilate()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int chan;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

  shader += "if (fc[chan] > 0.9/255.0) return;\n";

  shader += "float t = 0.0;\n";
  shader += "for(int i=-1; i<=1; i++)\n";
  shader += " for(int j=-1; j<=1; j++)\n";
  shader += "  for(int k=-1; k<=1; k++)\n";
  shader += "   {\n";
  shader += "    if (i==0 || j==0 || k==0)\n";
  shader += "     {\n";
  shader += "       int x1 = int(clamp(float(ox+i), 0.0, float(gridx-1)));\n";
  shader += "       int y1 = int(clamp(float(oy+j), 0.0, float(gridy-1)));\n";
  shader += "       int z1 = int(clamp(float(oz+k), 0.0, float(gridz-1)));\n";
  shader += "       int row = z1/ncols;\n";
  shader += "       int col = z1 - row*ncols;\n";
  shader += "       col *= gridx;\n";
  shader += "       row *= gridy;\n";
  shader += "       vec4 tc = texture2DRect(pruneTex, vec2(col+x1, row+y1) + vec2(0.5,0.5));\n";
  shader += "       t = max(t, tc[chan]);\n";
  shader += "     }\n";
  shader += "   }\n";

  shader += "fc[chan] = t;\n";
  shader += "gl_FragColor = fc;\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::restrictedDilate()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int val;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

  shader += "if (fc.x > 0.9/255.0) return;\n";

  shader += "float val0 = (float(val)-0.5)/255.0;\n";
  shader += "float val1 = (float(val)+0.5)/255.0;\n";
  shader += "float t = 0.0;\n";
  shader += "for(int i=-1; i<=1; i++)\n";
  shader += " for(int j=-1; j<=1; j++)\n";
  shader += "  for(int k=-1; k<=1; k++)\n";
  shader += "   {\n";
  shader += "    if (i==0 || j==0 || k==0)\n";
  shader += "     {\n";
  shader += "       int x1 = int(clamp(float(ox+i), 0.0, float(gridx-1)));\n";
  shader += "       int y1 = int(clamp(float(oy+j), 0.0, float(gridy-1)));\n";
  shader += "       int z1 = int(clamp(float(oz+k), 0.0, float(gridz-1)));\n";
  shader += "       int row = z1/ncols;\n";
  shader += "       int col = z1 - row*ncols;\n";
  shader += "       col *= gridx;\n";
  shader += "       row *= gridy;\n";
  shader += "       float v = texture2DRect(pruneTex, vec2(col+x1, row+y1) + vec2(0.5,0.5)).x;\n";
  //shader += "       t += (step(val0,v)*step(v,val1)) ;\n";
  shader += "       if (v > val0 && v < val1) t = 1.0;\n";
  shader += "     }\n";
  shader += "   }\n";

  shader += "t = (float(val)/255.0)*step(0.5, t);\n";
  shader += "gl_FragColor.xw=vec2(t,1.0);\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::erode()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int chan;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  //shader += "gl_FragColor = vec4(0.0,0.0,0.0,0.0);\n";
  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

  shader += "if (fc[chan] < 0.9/255.0) return;\n";

  shader += "float t = 0.0;\n";
  shader += "for(int i=-1; i<=1; i++)\n";
  shader += " for(int j=-1; j<=1; j++)\n";
  shader += "  for(int k=-1; k<=1; k++)\n";
  shader += "   {\n";
  shader += "    if (i==0 || j==0 || k==0)\n";
  shader += "     {\n";
  shader += "       int x1 = int(clamp(float(ox+i), 0.0, float(gridx-1)));\n";
  shader += "       int y1 = int(clamp(float(oy+j), 0.0, float(gridy-1)));\n";
  shader += "       int z1 = int(clamp(float(oz+k), 0.0, float(gridz-1)));\n";
  shader += "       int row = z1/ncols;\n";
  shader += "       int col = z1 - row*ncols;\n";
  shader += "       col *= gridx;\n";
  shader += "       row *= gridy;\n";
  shader += "       vec4 tc = texture2DRect(pruneTex, vec2(col+x1, row+y1) + vec2(0.5,0.5));\n";
  shader += "       t += step(tc[chan], 0.9/255.0);\n"; // tc[chan] is zero
  shader += "     }\n";
  shader += "   }\n";

  shader += "if (t > 1.5) fc[chan] = 0.0;\n";

  shader += "gl_FragColor = fc;\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::shrink()
// shrink channel 0 based on mask in chan
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int chan;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  //shader += "gl_FragColor = vec4(0.0,0.0,0.0,0.0);\n";
  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

  shader += "if (fc.x < 0.9/255.0) return;\n";

  // use chan as mask - erode only if chan is also 0
  shader += "if (fc[chan] > 0.9/255.0) return;\n";

  shader += "float t = 0.0;\n";
  shader += "for(int i=-1; i<=1; i++)\n";
  shader += " for(int j=-1; j<=1; j++)\n";
  shader += "  for(int k=-1; k<=1; k++)\n";
  shader += "   {\n";
  shader += "    if (i==0 || j==0 || k==0)\n";
  shader += "     {\n";
  shader += "       int x1 = int(clamp(float(ox+i), 0.0, float(gridx-1)));\n";
  shader += "       int y1 = int(clamp(float(oy+j), 0.0, float(gridy-1)));\n";
  shader += "       int z1 = int(clamp(float(oz+k), 0.0, float(gridz-1)));\n";
  shader += "       int row = z1/ncols;\n";
  shader += "       int col = z1 - row*ncols;\n";
  shader += "       col *= gridx;\n";
  shader += "       row *= gridy;\n";
  shader += "       vec4 tc = texture2DRect(pruneTex, vec2(col+x1, row+y1) + vec2(0.5,0.5));\n";
  shader += "       t += step(tc.x, 0.9/255.0);\n"; // tc.x is zero
  shader += "     }\n";
  shader += "   }\n";

  shader += "if (t > 1.5) fc.x = 0.0;\n";
  shader += "gl_FragColor = fc;\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::invert()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int chan;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  gl_FragColor = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "  if (chan == -1)\n";
  shader += "    gl_FragColor.x = 1.0 - gl_FragColor.x;\n";
  shader += "  else\n";
  shader += "   {\n";
  shader += "     vec4 col = gl_FragColor;\n";
  shader += "     col[chan] = 1.0 - col[chan];\n";
  shader += "     gl_FragColor = col;\n";
  shader += "   }\n";
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::setValue()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int val;\n";
  shader += "uniform int chan;\n";
  shader += "uniform int minval;\n";
  shader += "uniform int maxval;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  float t = min(1.0, (float(val)+0.5)/255.0);\n";
  shader += "  float tmin = (float(minval)+0.5)/255.0;\n";
  shader += "  float tmax = (float(maxval)+0.5)/255.0;\n";
  shader += "  gl_FragColor = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "  vec4 col = gl_FragColor;\n";
  shader += "  if (col[chan] >= tmin && col[chan] <= tmax)\n";
  shader += "  col[chan] = t;\n";
  shader += "  gl_FragColor = col;\n";
//  shader += "  if (gl_FragColor.x >= tmin && gl_FragColor.x <= tmax)\n";
//  shader += "    gl_FragColor.x = t;\n";
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::thicken()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform bool cityblock;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "gl_FragColor = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";

  // return if this voxel is already set
  shader += "if (gl_FragColor.x > 0.0) return;\n";
  
  shader += "float t = 0.0;\n";
  shader += "for(int i=-1; i<=1; i++)\n";
  shader += " for(int j=-1; j<=1; j++)\n";
  shader += "  for(int k=-1; k<=1; k++)\n";
  shader += "   {\n";
  shader += "    bool doit = (cityblock && (i==0 || j==0 || k==0)) || \n"; // cityblock distance
  shader += "                (!cityblock && !(i==0 && j==0 && k==0));\n";  // chessboard distance
  shader += "    if (doit)\n";
  shader += "     {\n";
  shader += "       int x1 = int(clamp(float(ox+i), 0.0, float(gridx-1)));\n";
  shader += "       int y1 = int(clamp(float(oy+j), 0.0, float(gridy-1)));\n";
  shader += "       int z1 = int(clamp(float(oz+k), 0.0, float(gridz-1)));\n";
  shader += "       int row = z1/ncols;\n";
  shader += "       int col = z1 - row*ncols;\n";
  shader += "       col *= gridx;\n";
  shader += "       row *= gridy;\n";
  shader += "       vec2 crd = vec2(col+x1, row+y1) + vec2(0.5,0.5);\n";
  shader += "       float pval = texture2DRect(pruneTex, crd).x;\n";
  shader += "       t = max(t, pval);\n";
  shader += "     }\n";
  shader += "   }\n";

  // decrement t
  shader += "t = min(1.0, t-1.2/255.0);\n";

  shader += "gl_FragColor.xw = vec2(t,1.0);\n";
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::edgeTexture()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int val;\n";
  shader += "uniform int sz;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "bvec3 b1 = lessThanEqual(ivec3(ox,oy,oz), ivec3(sz-1,sz-1,sz-1));\n";
  shader += "bvec3 b2 = greaterThanEqual(ivec3(ox,oy,oz), ivec3(gridx-sz,gridy-sz,gridz-sz));\n";

  shader += "bool tag = any(b1) || any(b2);\n";

  shader += "gl_FragColor = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "float t = float(val)/255.0;\n";
  shader += "if (tag) gl_FragColor.x = t;\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::dilateEdgeTexture()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int erodeval;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "gl_FragColor = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";

  shader += "if (gl_FragColor.x < 0.9/255.0) return;\n";
  shader += "if (gl_FragColor.x > (float(erodeval)-0.5)/255.0) return;\n";
  //shader += "if (gl_FragColor.x > 254.0/255.0) return;\n";

  shader += "float t = 1.0;\n";
  shader += "for(int i=-2; i<=2; i++)\n";
  shader += " for(int j=-2; j<=2; j++)\n";
  shader += "  for(int k=-2; k<=2; k++)\n";
  shader += "   {\n";
  shader += "    if (i==0 || j==0 || k==0)\n";
  shader += "     {\n";
  shader += "       int x1 = int(clamp(float(ox+i), 0.0, float(gridx-1)));\n";
  shader += "       int y1 = int(clamp(float(oy+j), 0.0, float(gridy-1)));\n";
  shader += "       int z1 = int(clamp(float(oz+k), 0.0, float(gridz-1)));\n";
  shader += "       int row = z1/ncols;\n";
  shader += "       int col = z1 - row*ncols;\n";
  shader += "       col *= gridx;\n";
  shader += "       row *= gridy;\n";
  shader += "       vec2 crd = vec2(col+x1, row+y1) + vec2(0.5,0.5);\n";
  shader += "       float pval = texture2DRect(pruneTex, crd).x;\n";
  shader += "       t = min(t, pval);\n";
  shader += "     }\n";
  shader += "   }\n";

  shader += "if (t > 0.9/255.0) return;\n";

  shader += "gl_FragColor.xw = vec2(0.0,1.0);\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::copyChannel()
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int src;\n";
  shader += "uniform int dst;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 spos = gl_TexCoord[0];\n";
  shader += "  vec4 col = texture2DRect(pruneTex, spos.xy);\n";  
  shader += "  col[dst] = texture2DRect(pruneTex, spos.xy)[src];\n";
  shader += "  gl_FragColor = col;\n";  
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::average()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int chan1;\n";
  shader += "uniform int chan2;\n";
  shader += "uniform int dst;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 col = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";  
  //  average > 0.5 => take next value
  shader += "  col[dst] = ((col[chan1]+col[chan2])*0.5 + 0.5/255.0);\n";
  shader += "  gl_FragColor = col;\n";  
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::minTexture()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex1;\n";
  shader += "uniform sampler2DRect pruneTex2;\n";
  shader += "uniform int ch1;\n";
  shader += "uniform int ch2;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 spos = gl_TexCoord[0];\n";
  shader += "  if (ch1 == -1 || ch2 == -1)\n";
  shader += "  {\n";
  shader += "    gl_FragColor = texture2DRect(pruneTex1, spos.xy);\n";
  shader += "    gl_FragColor = min(gl_FragColor,texture2DRect(pruneTex2, spos.xy));\n";
  shader += "  }\n";
  shader += "  else\n";
  shader += "  {\n";
  shader += "    vec4 col1 = texture2DRect(pruneTex1, spos.xy);\n";  
  shader += "    vec4 col2 = texture2DRect(pruneTex2, spos.xy);\n";  
  shader += "    col1[ch1] = min(col1[ch1], col2[ch2]);\n";
  shader += "    gl_FragColor = col1;\n";  
  shader += "  }\n";
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::maxTexture()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex1;\n";
  shader += "uniform sampler2DRect pruneTex2;\n";
  shader += "uniform int ch1;\n";
  shader += "uniform int ch2;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 spos = gl_TexCoord[0];\n";
  shader += "  if (ch1 == -1 || ch2 == -1)\n";
  shader += "  {\n";
  shader += "    gl_FragColor = texture2DRect(pruneTex1, spos.xy);\n";
  shader += "    gl_FragColor = max(gl_FragColor,texture2DRect(pruneTex2, spos.xy));\n";
  shader += "  }\n";
  shader += "  else\n";
  shader += "  {\n";
  shader += "    vec4 col1 = texture2DRect(pruneTex1, spos.xy);\n";  
  shader += "    vec4 col2 = texture2DRect(pruneTex2, spos.xy);\n";  
  shader += "    col1[ch1] = max(col1[ch1], col2[ch2]);\n";
  shader += "    gl_FragColor = col1;\n";  
  shader += "  }\n";
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::xorTexture()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex1;\n";
  shader += "uniform sampler2DRect pruneTex2;\n";
  shader += "uniform int ch1;\n";
  shader += "uniform int ch2;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 spos = gl_TexCoord[0];\n";
  shader += "  if (ch1 == -1 || ch2 == -1)\n";
  shader += "  {\n";
  shader += "    gl_FragColor = texture2DRect(pruneTex1, spos.xy);\n";
  shader += "    gl_FragColor.a = 1.0;\n";
  shader += "    vec4 col1 = gl_FragColor;\n";
  shader += "    vec4 col2 = texture2DRect(pruneTex2, spos.xy);\n";
  shader += "    for(int chan=0; chan<3; chan++)\n";
  shader += "     {\n";
  shader += "      float f1 = col1[chan];\n";
  shader += "      float f2 = col2[chan];\n";
  shader += "      if ((f1 > 0.0 && f2 > 0.0) || (f1 < 0.001 && f2 < 0.001))\n";
  shader += "        col1[chan] = 0.0;\n";
  shader += "      else\n";
  shader += "        col1[chan] = max(f1,f2);\n";
  shader += "     }\n";
  shader += "    gl_FragColor.rgb = col1.rgb;\n";
  shader += "  }\n";
  shader += "  else\n";
  shader += "  {\n";
  shader += "    vec4 col1 = texture2DRect(pruneTex1, spos.xy);\n";  
  shader += "    vec4 col2 = texture2DRect(pruneTex2, spos.xy);\n";  
  shader += "    if ((col1[ch1] > 0.0 && col2[ch2] > 0.0) ||\n";
  shader += "        (col1[ch1] < 0.001 && col2[ch2] < 0.001))\n";
  shader += "      col1[ch1] = 0.0;\n";
  shader += "    else\n";
  shader += "      col1[ch1] = max(col1[ch1],col2[ch2]);\n";
  shader += "    gl_FragColor = col1;\n";  
  shader += "  }\n";
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::localMaximum()
{
  // if (voxel < any of neighbours), set it to 0, otherwise set it to 1

  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

  shader += "if (fc.x < 1.0/255.0) return;\n";

  shader += "float t = 0.0;\n";
  shader += "for(int i=-1; i<=1; i++)\n";
  shader += " for(int j=-1; j<=1; j++)\n";
  shader += "  for(int k=-1; k<=1; k++)\n";
  shader += "   {\n";
  shader += "    if (i==0 || j==0 || k==0)\n";
  shader += "     {\n";
  shader += "       int x1 = int(clamp(float(ox+i), 0.0, float(gridx-1)));\n";
  shader += "       int y1 = int(clamp(float(oy+j), 0.0, float(gridy-1)));\n";
  shader += "       int z1 = int(clamp(float(oz+k), 0.0, float(gridz-1)));\n";
  shader += "       int row = z1/ncols;\n";
  shader += "       int col = z1 - row*ncols;\n";
  shader += "       col *= gridx;\n";
  shader += "       row *= gridy;\n";
  shader += "       t = max(t, texture2DRect(pruneTex, vec2(col+x1, row+y1) + vec2(0.5,0.5)).x);\n";
  shader += "     }\n";
  shader += "   }\n";

  shader += "t = step(t, fc.x);\n";

  shader += "gl_FragColor.xw = vec2(t,1.0);\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::carve()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform vec3 center;\n";
  shader += "uniform float radius;\n";
  shader += "uniform float decay;\n";
  shader += "uniform int docarve;\n";
  shader += "uniform bool planarcarve;\n";
  shader += "uniform vec3 clipp;\n";
  shader += "uniform vec3 clipn;\n";
  shader += "uniform float clipt;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

 // modify only x value, y value is used for restoration

  shader += "vec3 cpt = vec3(ox, oy, oz);\n";
  shader += "if (planarcarve && ";
  shader += "    abs(dot(clipn,(cpt-clipp))) > clipt) return;\n";

  shader += "if (docarve == 1)\n";
  shader += " {\n";
  shader += "   if (fc.x < 1.0/255.0 || fc.y < 1.0/255.0) return;\n";
  shader += "   float l = distance(vec3(ox, oy, oz), center);\n";
  shader += "   l = smoothstep(radius-decay, radius, l);\n";
  shader += "   gl_FragColor.x *= l;\n";
  shader += " }\n";
  shader += "else if (docarve == 2)\n"; // restore original inside sphere
  shader += " {\n";
  shader += "   if (fc.x > 254.0/255.0 || fc.y < 1.0/255.0) return;\n";
  shader += "   float l = distance(vec3(ox, oy, oz), center);\n";
  shader += "   l = 1.0-step(radius, l);\n";
  shader += "   gl_FragColor.x = mix(gl_FragColor.x, gl_FragColor.y, l);\n";
  shader += " }\n";
  shader += "else if (docarve == 3)\n"; // set : set to 1 inside sphere
  shader += " {\n";
  shader += "   if (fc.x > 254.0/255.0) return;\n";
  shader += "   float l = distance(vec3(ox, oy, oz), center);\n";
  shader += "   l = 1.0-step(radius, l);\n";
  shader += "   gl_FragColor.x = mix(gl_FragColor.x, l, l);\n";
  shader += " }\n";
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::paint()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform vec3 center;\n";
  shader += "uniform float radius;\n";
  shader += "uniform float decay;\n";
  shader += "uniform int dopaint;\n";
  shader += "uniform float tag;\n";
  shader += "uniform bool planarcarve;\n";
  shader += "uniform vec3 clipp;\n";
  shader += "uniform vec3 clipn;\n";
  shader += "uniform float clipt;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";
  shader += "if (fc.x < 1.0/255.0) return;\n";

  shader += "vec3 cpt = vec3(ox, oy, oz);\n";
  shader += "if (planarcarve && ";
  shader += "    abs(dot(clipn,(cpt-clipp))) > clipt) return;\n";

  // consider voxels within radius
  shader += "float cptdist = distance(cpt, center);\n";
  shader += "float l = step(cptdist, radius);\n";
  shader += "if (l < 0.001) return;\n";


 // modify z values only
  //shader += "if (dopaint == 1)\n";
  shader += "  gl_FragColor.z = mix(gl_FragColor.z, (tag+0.5)/255.0, l);\n";
  //shader += "else \n"; // restore
  //shader += "  gl_FragColor.z = mix(gl_FragColor.z, 0.0, l);\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::fillTriangle()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform vec3 A;\n";
  shader += "uniform vec3 B;\n";
  shader += "uniform vec3 C;\n";
  shader += "uniform float thick;\n";
  shader += "uniform int val;\n";
  shader += "uniform bool paint;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

  // if not painting then fill only transparent region
  shader += "if (!paint && fc.x > 0.9/255.0) return;\n";

  // if painting then paint only visible region
  shader += "if (paint && fc.x < 0.9/255.0) return;\n";

  // use barycentric coordinates for checking point inside triangle

  shader += "vec3 BA = B-A;\n";
  shader += "vec3 CA = C-A;\n";
  shader += "vec3 vA = vec3(ox,oy,oz)-A;\n";
  shader += "vec3 perp = normalize(cross(BA, CA));\n";
  shader += "float nv = dot(vA, perp);\n";

  // if point beyond certain thickness return
  shader += "if (abs(nv) > thick) return;\n";

  // project point onto ABC plane
  shader += "vec3 P = vec3(ox,oy,oz) - nv*perp;\n";

  shader += "vec3 v0 = C - A;\n";
  shader += "vec3 v1 = B - A;\n";
  shader += "vec3 v2 = P - A;\n";
  shader += "float dot00 = dot(v0, v0);\n";
  shader += "float dot01 = dot(v0, v1);\n";
  shader += "float dot02 = dot(v0, v2);\n";
  shader += "float dot11 = dot(v1, v1);\n";
  shader += "float dot12 = dot(v1, v2);\n";
  shader += "float invDenom = (dot00 * dot11 - dot01 * dot01);\n";
  shader += "if (abs(invDenom)<0.0001) return;\n";
  shader += "float u = (dot11 * dot02 - dot01 * dot12)/invDenom;\n";
  shader += "float v = (dot00 * dot12 - dot01 * dot02)/invDenom;\n";
  shader += "float t = (1.0-step(u,0.0))*(1.0-step(v,0.0))*step(u+v,1.0);\n";
  shader += "float tv = t*(float(val)+0.5)/255.0;\n";
  shader += "if (paint)\n";
  shader += "  {\n";
  shader += "    if (t > 0.0) \n";
  shader += "      gl_FragColor.z = tv;\n";
  shader += "  }\n";
  shader += "else\n";
  shader += "  gl_FragColor.xyw = mix(gl_FragColor.xyw, vec3(tv,0.2,1.0), t);\n";
  
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::removePatch()
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform bool remove;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 spos = gl_TexCoord[0];\n";
  shader += "  vec4 fc = texture2DRect(pruneTex, spos.xy);\n";  
  shader += "  gl_FragColor = fc;\n";
  shader += "  float t = step(0.3, fc.y);\n";  
  shader += "  if (remove)\n";
  shader += "    gl_FragColor.xy = mix(vec2(0.0,0.0), gl_FragColor.xy, t);\n";
  shader += "  else\n";
  shader += "    gl_FragColor.y = mix(gl_FragColor.x, gl_FragColor.y, t);\n";
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::clip()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform vec3 pos;\n";
  shader += "uniform vec3 normal;\n";
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

//  // do not clip if second channel > 0
//  shader += "if (gl_FragColor.y > 0.0) return;\n";

 // modify only x value
  shader += "vec3 v = vec3(ox,oy,oz)-pos;\n";
  shader += "float l = dot(v, normal);\n";
  shader += "l = smoothstep(0.0, 3.0, l);\n";
  shader += "gl_FragColor.x = mix(gl_FragColor.x, 0.0, l);\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::crop(QString cropShader)
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int lod;\n";
  shader += "uniform vec3 voxelScaling;\n";
  shader += "uniform vec3 dmin;\n";

  shader += cropShader;
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

 // modify only x value
  shader += "vec3 v = vec3(ox,oy,oz);\n";
  shader += "v = v * vec3(lod,lod,lod);\n";
  shader += "v = v + dmin;\n";
  shader += "v = v * voxelScaling;\n";

  shader += "gl_FragColor.x = min(gl_FragColor.x, 1.0-crop(v, false));\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::maxValue()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int cols;\n";
  shader += "uniform int rows;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "gl_FragColor = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "for(int i=0; i<cols; i++)\n";
  shader += "for(int j=0; j<rows; j++)\n";
  shader += "  gl_FragColor = max(gl_FragColor, texture2DRect(pruneTex, gl_TexCoord[0].xy + vec2(i,j) + vec2(0.5,0.5)));\n";
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::histogram()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int cols;\n";
  shader += "uniform int rows;\n";
  shader += "uniform int val;\n";
  shader += "uniform int addstep;\n";
  shader += "uniform int chan;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  int freq = 0;\n";
  shader += "  int x,y,z;\n";

  shader += "  if (addstep == 0)\n";
  shader += "  {\n";
  shader += "    float lowval = (float(val)-0.5)/255.0;\n";
  shader += "    float highval= (float(val)+0.5)/255.0;\n";
  shader += "    for(int i=0; i<cols; i++)\n";
  shader += "    for(int j=0; j<rows; j++)\n";
  shader += "     {\n";
  shader += "       float v = texture2DRect(pruneTex, gl_TexCoord[0].xy + vec2(i,j) + vec2(0.5,0.5))[chan];\n";
  shader += "       freq += int(step(lowval, v)*step(v, highval));\n";
  shader += "     }\n";
  shader += "  }\n";
  shader += "  else\n";
  shader += "  {\n";
  shader += "    for(int i=0; i<cols; i++)\n";
  shader += "    for(int j=0; j<rows; j++)\n";
  shader += "     {\n";
  shader += "       vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy + vec2(i,j) + vec2(0.5,0.5));\n";
  shader += "       x = int(fc.x*255.0);\n";
  shader += "       y = int(fc.y*255.0);\n";
  shader += "       z = int(fc.z*255.0);\n";
  shader += "       freq += x*255*255 + y*255 + z;\n";
  shader += "     }\n";
  shader += "  }\n";

  shader += "  x = freq/(255*255);\n";
  shader += "  y = (freq - x*255*255)/255;\n";
  shader += "  z = freq - x*255*255 - y*255;\n";

  shader += "  gl_FragColor = vec4(float(x)/255.0, float(y)/255.0, float(z)/255.0, 1.0);\n";
  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::localThickness()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int thickness;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

  shader += "float lowthick = (float(thickness)-0.5)/255.0;\n";
  shader += "float highthick = (float(thickness)+0.5)/255.0;\n";

  shader += "if (fc.x < 1.0/255.0) return;\n";
  shader += "if (fc.x > lowthick) return;\n";

  shader += "float t = fc.x;\n";
  shader += "for(int i=-1; i<=1; i++)\n";
  shader += " for(int j=-1; j<=1; j++)\n";
  shader += "  for(int k=-1; k<=1; k++)\n";
  shader += "   {\n";
  shader += "    if (i==0 || j==0 || k==0)\n";
  shader += "     {\n";
  shader += "       int x1 = int(clamp(float(ox+i), 0.0, float(gridx-1)));\n";
  shader += "       int y1 = int(clamp(float(oy+j), 0.0, float(gridy-1)));\n";
  shader += "       int z1 = int(clamp(float(oz+k), 0.0, float(gridz-1)));\n";
  shader += "       int row = z1/ncols;\n";
  shader += "       int col = z1 - row*ncols;\n";
  shader += "       col *= gridx;\n";
  shader += "       row *= gridy;\n";
  shader += "       float val = texture2DRect(pruneTex, vec2(col+x1, row+y1) + vec2(0.5,0.5)).x;\n";
  shader += "       val = step(lowthick, val)*step(val, highthick);\n";
  shader += "       t = mix(t, highthick, val);\n";
  shader += "     }\n";
  shader += "   }\n";

  shader += "gl_FragColor.x = t;\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::smoothChannel()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform int chan;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

  shader += "if (fc[chan] < 1.0/255.0) return;\n";

  shader += "float nt = 0.0;\n";
  shader += "float t = 0.0;\n";
  shader += "for(int i=-1; i<=1; i++)\n";
  shader += " for(int j=-1; j<=1; j++)\n";
  shader += "  for(int k=-1; k<=1; k++)\n";
  shader += "   {\n";
  shader += "     int x1 = int(clamp(float(ox+i), 0.0, float(gridx-1)));\n";
  shader += "     int y1 = int(clamp(float(oy+j), 0.0, float(gridy-1)));\n";
  shader += "     int z1 = int(clamp(float(oz+k), 0.0, float(gridz-1)));\n";
  shader += "     int row = z1/ncols;\n";
  shader += "     int col = z1 - row*ncols;\n";
  shader += "     col *= gridx;\n";
  shader += "     row *= gridy;\n";
  shader += "     float val = texture2DRect(pruneTex, vec2(col+x1, row+y1) + vec2(0.5,0.5))[chan];\n";
  shader += "     nt += step(0.5/255.0, val);\n";
  shader += "     t += val;\n";
  shader += "   }\n";

  shader += "if (nt > 0.0) fc[chan] = t/nt;\n";
  shader += "gl_FragColor = fc;\n";

  shader += "}\n";

  return shader;
}

QString
PruneShaderFactory::pattern()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect pruneTex;\n";
  shader += "uniform int gridx;\n";
  shader += "uniform int gridy;\n";
  shader += "uniform int gridz;\n";
  shader += "uniform int nrows;\n";
  shader += "uniform int ncols;\n";
  shader += "uniform bool flag;\n";
  shader += "uniform int pat[6];\n";
  
  
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "int ocol = int(gl_TexCoord[0].x)/gridx;\n";
  shader += "int orow = int(gl_TexCoord[0].y)/gridy;\n";
  shader += "int ox = int(gl_TexCoord[0].x) - ocol*gridx;\n";
  shader += "int oy = int(gl_TexCoord[0].y) - orow*gridy;\n";
  shader += "int oz = orow*ncols + ocol;\n";

  shader += "vec4 fc = texture2DRect(pruneTex, gl_TexCoord[0].xy);\n";
  shader += "gl_FragColor = fc;\n";

  shader += "if (fc.x < 0.9/255.0) return;\n";

  shader += "int xn, xd, yn, yd, zn, zd;\n";
  shader += "xn = pat[0];\n";
  shader += "xd = pat[1];\n";
  shader += "yn = pat[2];\n";
  shader += "yd = pat[3];\n";
  shader += "zn = pat[4];\n";
  shader += "zd = pat[5];\n";

  shader += "bool zeroit = true;\n";
  shader += "if (xn > 0 && xd > 0)\n";
  shader += "  zeroit = zeroit && (mod(float(ox),float(xn)) < float(xd));\n";
  shader += "if (yn > 0 && yd > 0)\n";
  shader += "  zeroit = zeroit && (mod(float(oy),float(yn)) < float(yd));\n";
  shader += "if (zn > 0 && zd > 0)\n";
  shader += "  zeroit = zeroit && (mod(float(oz),float(zn)) < float(zd));\n";

  shader += "if (flag == zeroit) fc.x = 0.0;\n";

  shader += "gl_FragColor = fc;\n";

  shader += "}\n";

  return shader;
}

