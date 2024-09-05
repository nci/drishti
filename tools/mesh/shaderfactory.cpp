#include "staticfunctions.h"
#include "shaderfactory.h"
#include "global.h"

#include <QTextEdit>
#include <QVBoxLayout>

QList<GLuint> ShaderFactory::m_shaderList;


QString
ShaderFactory::OrenNayarDiffuseShader()
{
  QString shader;

  shader += "float OrenNayarDiffuse(vec3 L, vec3 V, vec3 N, float roughness)\n";  
  shader += "{  \n";  
  shader += "  float PI=3.14159265;\n";
  shader += "  float LdotV = dot(L, V);\n";  
  shader += "  float NdotL = dot(L, N);\n";  
  shader += "  float NdotV = dot(N, V);\n";  
  shader += "  \n";  
  shader += "  float s = LdotV - NdotL * NdotV;\n";  
  shader += "  float t = mix(1.0, max(NdotL, NdotV), step(0.0, s));\n";  
  shader += "  \n";  
  shader += "  float sigma2 = roughness * roughness;\n";  
  shader += "  float A = 1.0 + sigma2 * (1.0 / (sigma2 + 0.13) + 0.5 / (sigma2 + 0.33));\n";  
  shader += "  float B = 0.45 * sigma2 / (sigma2 + 0.09);\n";  
  shader += "  \n";  
  shader += "  return abs(NdotL) * (A + B * s / t) / PI;\n";  
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::ggxShader()
{
  QString shader;
  shader += "vec3 shadingSpecularGGX(vec3 N, vec3 V, vec3 L, float roughness, vec3 F0)\n";
  shader += "{\n";
  shader += "    // see http://www.filmicworlds.com/2014/04/21/optimizing-ggx-shaders-with-dotlh/\n";
  shader += "    vec3 H = normalize(V + L);\n";
  shader += "\n";
  shader += "    float dotLH = max(dot(L, H), 0.0);\n";
  shader += "    float dotNH = max(dot(N, H), 0.0);\n";
  shader += "    float dotNL = max(dot(N, L), 0.0);\n";
  shader += "    float dotNV = max(dot(N, V), 0.0);\n";
  shader += "\n";
  shader += "    float alpha = roughness * roughness;\n";
  shader += "\n";
  shader += "    // D (GGX normal distribution)\n";
  shader += "    float alphaSqr = alpha * alpha;\n";
  shader += "    float denom = dotNH * dotNH * (alphaSqr - 1.0) + 1.0;\n";
  shader += "    float D = alphaSqr / (denom * denom);\n";
  shader += "    // no pi because BRDF -> lighting\n";
  shader += "\n";
  shader += "    // F (Fresnel term)\n";
  shader += "    float F_a = 1.0;\n";
  shader += "    float F_b = pow(1.0 - dotLH, 5); // manually?\n";
  shader += "    vec3 F = mix(vec3(F_b), vec3(F_a), F0);\n";
  shader += "\n";
  shader += "    // G (remapped hotness, see Unreal Shading)\n";
  shader += "    float k = (alpha + 2 * roughness + 1) / 8.0;\n";
  shader += "    float G = dotNL / (mix(dotNL, 1, k) * mix(dotNV, 1, k));\n";
  shader += "\n";
  shader += "    return D * F * G / 4.0;\n";
  shader += "}\n";

  return shader;
}

bool
ShaderFactory::loadShader(GLhandleARB &progObj,
			  QString fragShaderString)
{
    QString vertShaderString;
#ifndef Q_OS_MACX
    vertShaderString += "#version 130\n";
    vertShaderString += "out float gl_ClipDistance[2];\n";
    vertShaderString += "uniform vec4 ClipPlane0;\n";
    vertShaderString += "uniform vec4 ClipPlane1;\n";
#endif
    vertShaderString += "out vec3 pointpos;\n";
    vertShaderString += "out vec3 glTexCoord0;\n";
    vertShaderString += "void main(void)\n";
    vertShaderString += "{\n";
    vertShaderString += "  // Transform vertex position into homogenous clip-space.\n";
    vertShaderString += "  gl_FrontColor = gl_Color;\n";
    vertShaderString += "  gl_BackColor = gl_Color;\n";
    vertShaderString += "  gl_Position = ftransform();\n";
    vertShaderString += "  gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n";
    vertShaderString += "  gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;\n";
    vertShaderString += "  gl_TexCoord[2] = gl_TextureMatrix[2] * gl_MultiTexCoord2;\n";
    vertShaderString += "  pointpos = gl_Vertex.xyz;\n";
    vertShaderString += "  glTexCoord0 = gl_TexCoord[0].xyz;\n";
#ifndef Q_OS_MACX
    vertShaderString += "  gl_ClipDistance[0] = dot(gl_Vertex, ClipPlane0);\n";
    vertShaderString += "  gl_ClipDistance[1] = dot(gl_Vertex, ClipPlane1);\n";
#else
    vertShaderString += "  gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n";
#endif
    vertShaderString += "}\n";

    if (loadShader(progObj, vertShaderString, fragShaderString))
      return true;

    
    QMessageBox::information(0, "", "Cannot load shaders");
    return false;
}

QString
ShaderFactory::genBlurShaderString(bool softShadows,
				   int blurSize,
				   float blurRadius)
{
  QString shader;

  shader  = "uniform sampler2DRect shadowTex;\n";
  shader += "\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 shadow;\n";
  shader += "  vec4 spos = gl_TexCoord[0];\n";
  shader += "\n";
  shader += "  shadow = texture2DRect(shadowTex, spos.xy);\n";

  if (softShadows)
    {
      shader += "  vec4 vshadow;\n";
      shader += "\n";
      shader += "  vshadow = vec4(0.0,0.0,0.0,0.0);\n";
      shader += "\n";

      shader += "  ivec2 idel;\n";
      shader += "  idel = ivec2(spos.xy);\n";
      shader += "  idel -= 2*(idel/2);\n";
      shader += "  idel.y = int(idel.x == idel.y);\n";
      shader += "  idel.x = 1;\n";
      shader += "\n";

      shader += "  vec2 del, delprev, delstep;\n";
      shader += QString("  del = vec2(idel)*float(%1);\n").arg(blurRadius);
      shader += "  delstep = del;\n";
      shader += "\n";

      float frc = 1.0/(4*blurSize + 1);
      for (int i=0; i<blurSize; i++)
	{
	  if (i<blurSize-1)
	    shader += "  delprev = del;\n";

	  shader += "  vshadow += texture2DRect(shadowTex, spos.xy+del.xy);\n";
	  shader += "  vshadow += texture2DRect(shadowTex, spos.xy-del.xy);\n";
	  shader += "  del.xy = del.yx;\n";
	  shader += "  del.x = -del.x;\n";
	  shader += "  vshadow += texture2DRect(shadowTex, spos.xy+del.xy);\n";
	  shader += "  vshadow += texture2DRect(shadowTex, spos.xy-del.xy);\n";

	  shader += "\n";
	  
	  if (i<blurSize-1)
	    shader += "  del = delprev + delstep;\n";
	}

      shader += QString("  shadow = float(%1)*(shadow + vshadow);\n").arg(frc);
    }

  shader += "  gl_FragColor.rgba = shadow;\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genBoxShaderString()
{
  QString shader;

  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect blurTex;\n";
  shader += "uniform vec2 direc;\n";
  shader += "uniform int type;\n";
  shader += "\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 color = vec4(0.0);\n";
  shader += "  vec2 uv = gl_TexCoord[0].xy;\n";
  shader += "\n";
  shader += "  if (type == 1)\n";
  shader += "   {\n";
  shader += "    gl_FragColor = texture2DRect(blurTex, uv+direc*0.5)*0.5;\n";
  shader += "    gl_FragColor+= texture2DRect(blurTex, uv-direc*0.5)*0.5;\n";
  shader += "   }\n";
  shader += "  else if (type == 2)\n";
  shader += "   {\n";
  shader += "    vec2 off1 = vec2(1.3333333333333333) * direc;\n";
  shader += "    gl_FragColor =  texture2DRect(blurTex, uv)*0.29411764705882354;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv+off1)*0.35294117647058826;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv-off1)*0.35294117647058826;\n";
  shader += "   }\n";
  shader += "  else if (type == 3)\n";
  shader += "   {\n";
  shader += "    vec2 off1 = vec2(1.3846153846) * direc;\n";
  shader += "    vec2 off2 = vec2(3.2307692308) * direc;\n";
  shader += "    gl_FragColor =  texture2DRect(blurTex, uv) * 0.2270270270;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv + off1) * 0.3162162162;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv - off1) * 0.3162162162;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv + off2) * 0.0702702703;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv - off2) * 0.0702702703;\n";
  shader += "   }\n";
  shader += "  else if (type == 4)\n";
  shader += "   {\n";
  shader += "    vec2 off1 = vec2(1.411764705882353) * direc;\n";
  shader += "    vec2 off2 = vec2(3.2941176470588234)* direc;\n";
  shader += "    vec2 off3 = vec2(5.176470588235294) * direc;\n";
  shader += "    gl_FragColor =  texture2DRect(blurTex, uv) * 0.1964825501511404;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv + off1) * 0.2969069646728344;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv - off1) * 0.2969069646728344;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv + off2) * 0.09447039785044732;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv - off2) * 0.09447039785044732;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv + off3) * 0.010381362401148057;\n";
  shader += "    gl_FragColor += texture2DRect(blurTex, uv - off3) * 0.010381362401148057;\n";
  shader += "  }\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genSmoothDilatedShaderString()
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect blurTex;\n";
  shader += "\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 color;\n";
  shader += "  vec4 spos = gl_TexCoord[0];\n";
  shader += "\n";
  shader += "  color = texture2DRect(blurTex, spos.xy);\n";
  shader += "  float s = 0;\n";
  shader += "  int bs = 1;\n";
  shader += "  for (int i=-bs; i<=bs; i++)\n";
  shader += "  for (int j=bs; j<=bs; j++)\n";
  shader += "  {\n";
  shader += "    s += step(0.0, texture2DRect(blurTex, spos.xy + vec2(i,j)).b);\n";
  shader += "  }\n";
  shader += "  if (s > 0.4*(2*bs+1)*(2*bs+1)) color = vec4(0.0);\n";
  shader += "  gl_FragColor = color;\n";
  shader += "}\n";

  return shader;
}  


QString
ShaderFactory::genRectBlurShaderString(int filter)
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect blurTex;\n";
  shader += "\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 color;\n";
  shader += "  vec4 spos = gl_TexCoord[0];\n";
  shader += "\n";  

  if (filter == Global::_GaussianFilter)
    { // gaussian filter
      shader += "  color  = vec4(4.0,4.0,4.0,4.0)*texture2DRect(blurTex, spos.xy);\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2( 0.0, 1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2( 0.0,-1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2( 1.0, 0.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2(-1.0, 0.0));\n";
      shader += "  color += texture2DRect(blurTex, spos.xy + vec2( 1.0, 1.0));\n";
      shader += "  color += texture2DRect(blurTex, spos.xy + vec2( 1.0,-1.0));\n";
      shader += "  color += texture2DRect(blurTex, spos.xy + vec2(-1.0, 1.0));\n";
      shader += "  color += texture2DRect(blurTex, spos.xy + vec2(-1.0,-1.0));\n";
      shader += "  gl_FragColor.rgba = color/vec4(16.0);\n";
    }
  else if (filter == Global::_SharpnessFilter)
    { // sharpness filter
      shader += "  color = vec4(8.0,8.0,8.0,8.0)*texture2DRect(blurTex, spos.xy);\n";

      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2(1.0,0.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy - vec2(1.0,0.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2(0.0,1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy - vec2(0.0,1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2( 1.0, 1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2(-1.0, 1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2( 1.0,-1.0));\n";
      shader += "  color += vec4(2.0,2.0,2.0,2.0)*texture2DRect(blurTex, spos.xy + vec2(-1.0,-1.0));\n";

      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 2.0, 2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 2.0, 1.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 2.0, 0.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 2.0,-1.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 2.0,-2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-2.0, 2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-2.0, 1.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-2.0, 0.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-2.0,-1.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-2.0,-2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-1.0, 2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 0.0, 2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 1.0, 2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2(-1.0,-2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 0.0,-2.0));\n";
      shader += "  color -= texture2DRect(blurTex, spos.xy + vec2( 1.0,-2.0));\n";

      shader += "  gl_FragColor.rgba = color/vec4(8.0);\n";
    }
  else
    shader += "  color = texture2DRect(blurTex, spos.xy);\n";

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";

  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genCopyShaderString()
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect shadowTex;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 spos = gl_TexCoord[0];\n";
  shader += "  gl_FragColor.rgba = texture2DRect(shadowTex, spos.xy);\n";
  shader += "}\n";

  return shader;
}


QString
ShaderFactory::genDilateShaderString()
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect tex;\n";
  shader += "uniform int radius;\n";
  shader += "uniform vec2 direc;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec2 spos = gl_TexCoord[0].xy;\n";
  shader += "  gl_FragColor.rgba = texture2DRect(tex, spos);\n";

  shader += "  float maxd = 0;\n";
  shader += "  float d = 0;\n";
  shader += "  float twt = 0;\n";
  shader += "  for(int i=-radius; i<=radius; i++)\n";
  shader += "  {\n";
  shader += "    vec2 xz = texture2DRect(tex, spos + i*direc).xz;\n";
  shader += "    maxd = max(maxd, xz.r);\n";
  shader += "    float wt = radius+1 - abs(float(i));\n";
  shader += "    d += wt*xz.g;\n";
  shader += "    twt += wt;\n";
  shader += "  }\n";

  shader += "  gl_FragColor = vec4(maxd, 0, d/twt, 0.5);\n";
  shader += "}\n";

  return shader;
}


QString
ShaderFactory::genBackplaneShaderString1(float scale)
{
  QString shader;

  shader += "uniform sampler2DRect shadowTex;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 spos = gl_TexCoord[0];\n";
  shader += "  vec4 incident_light = texture2DRect(shadowTex, spos.xy);\n";
  shader += "  gl_FragColor.a = 1.0;\n";

  if (scale < 0.95)
      shader += QString("  incident_light *= float(%1);\n").arg(scale);

  shader += "  float maxval = 1.0 - incident_light.a;\n";
  shader += "  gl_FragColor.rgb = maxval*((incident_light.rgb+maxval)*gl_Color.rgb);\n";

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";

  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genBackplaneShaderString2(float scale)
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect bgTex;\n";
  shader += "uniform sampler2DRect shadowTex;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 spos = gl_TexCoord[0];\n";
  shader += "  vec4 incident_light = texture2DRect(shadowTex, spos.xy);\n";

  if (scale < 0.95)
      shader += QString("  incident_light *= float(%1);\n").arg(scale);

  shader += "  float maxval = 1.0 - incident_light.a;\n";

  shader += "  spos = gl_TexCoord[1];\n";
  shader += "  vec4 bg_Color = texture2DRect(bgTex, spos.xy);\n";
  shader += "  gl_FragColor.rgb = maxval*((incident_light.rgb+maxval)*bg_Color.rgb);\n";
  shader += "  gl_FragColor.a = bg_Color.a;\n";
  
  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genPassThruShaderString()
{
  QString shader;

  shader = "void main(void)\n";
  shader += "{\n";
  shader += "  gl_FragColor = gl_Color;\n";
  shader += "}\n";

  return shader;
}


//----------------------------
//----------------------------

GLint ShaderFactory::m_oitShaderParm[30];
GLint* ShaderFactory::oitShaderParm() { return &m_oitShaderParm[0]; }

GLuint ShaderFactory::m_oitShader = 0;
GLuint ShaderFactory::oitShader()
{
  if (m_oitShader)
    return m_oitShader;

  QString vertShaderString = oitShaderV();
  QString fragShaderString = oitShaderF();
  
  m_oitShader = glCreateProgram();
  if (! loadShader(m_oitShader,
		   vertShaderString,
		   fragShaderString))
    {
      QMessageBox::information(0, "", "Cannot load oit shaders");
      return 0;
    }

  m_oitShaderParm[0] = glGetUniformLocation(m_oitShader, "MVP");
  m_oitShaderParm[1] = glGetUniformLocation(m_oitShader, "viewDir");
  m_oitShaderParm[2] = glGetUniformLocation(m_oitShader, "extras");
  m_oitShaderParm[3] = glGetUniformLocation(m_oitShader, "localXform");

  m_oitShaderParm[4] = glGetUniformLocation(m_oitShader, "MV");
  
  m_oitShaderParm[5] = glGetUniformLocation(m_oitShader, "roughness");
  m_oitShaderParm[6] = glGetUniformLocation(m_oitShader, "ambient");
  m_oitShaderParm[7] = glGetUniformLocation(m_oitShader, "diffuse");
  m_oitShaderParm[8] = glGetUniformLocation(m_oitShader, "specular");
  m_oitShaderParm[9] = glGetUniformLocation(m_oitShader, "nclip");
  m_oitShaderParm[10] = glGetUniformLocation(m_oitShader,"clipPos");
  m_oitShaderParm[11] = glGetUniformLocation(m_oitShader,"clipNormal");
  m_oitShaderParm[12] = glGetUniformLocation(m_oitShader,"sceneRadius");  
  m_oitShaderParm[13] = glGetUniformLocation(m_oitShader,"hasUV");
  m_oitShaderParm[14] = glGetUniformLocation(m_oitShader,"diffuseTex");
  m_oitShaderParm[15] = glGetUniformLocation(m_oitShader,"featherSize");
  m_oitShaderParm[16] = glGetUniformLocation(m_oitShader,"revealTransparency");
  m_oitShaderParm[17] = glGetUniformLocation(m_oitShader,"idx");
  m_oitShaderParm[18] = glGetUniformLocation(m_oitShader,"applyMaterial");
  m_oitShaderParm[19] = glGetUniformLocation(m_oitShader,"matcapTex");	
  m_oitShaderParm[20] = glGetUniformLocation(m_oitShader,"rightDir");
  m_oitShaderParm[21] = glGetUniformLocation(m_oitShader,"upDir");
  m_oitShaderParm[22] = glGetUniformLocation(m_oitShader,"matMix");

  m_oitShaderParm[25] = glGetUniformLocation(m_oitShader,"hideBlack");
  
  m_oitShaderParm[26] = glGetUniformLocation(m_oitShader,"hasNormalTex");
  m_oitShaderParm[27] = glGetUniformLocation(m_oitShader,"normalTex");
  m_oitShaderParm[28] = glGetUniformLocation(m_oitShader,"lightDir");

  
  return m_oitShader;      
}

QString
ShaderFactory::oitShaderV()
{
  QString shader;

  shader += "#version 420 core\n";

  shader += "uniform mat4 MVP;\n";
  shader += "uniform mat4 MV;\n";
  shader += "uniform mat4 localXform;\n";
  shader += "uniform float idx;\n";

  shader += "layout(location = 0) in vec3 position;\n";
  shader += "layout(location = 1) in vec3 normalIn;\n";
  shader += "layout(location = 2) in vec3 colorIn;\n";
  shader += "layout(location = 3) in vec3 tangentIn;\n";

  shader += "out vec3 v3Normal;\n";
  shader += "out vec3 v3Color;\n";
  shader += "out vec3 pointPos;\n";
  shader += "out float zdepth;\n";
  shader += "out float surfId;\n";
  shader += "out vec3 oPosition;\n";
  shader += "out float zdepthLinear;\n";
  shader += "out mat3 TBN;\n";

  shader += "void main()\n";
  shader += "{\n";
  shader += "   oPosition = position;\n";
  shader += "   pointPos = (localXform * vec4(position, 1)).xyz;\n";
  shader += "   v3Color = colorIn;\n";
  shader += "   v3Normal = normalIn;\n";

  shader += "   vec3 v3Tangent = tangentIn;\n";
  //shader += "   vec3 v3Tangent = normalize(tangentIn-dot(tangentIn, normalIn)*normalIn);\n";
  shader += "   vec3 v3BiTangent = cross(v3Normal, v3Tangent);\n";
  shader += "   TBN = mat3(v3Tangent, v3BiTangent, v3Normal);\n";
  
  shader += "   gl_Position = MVP * vec4(position, 1);\n";
  shader += "   zdepth = ((gl_DepthRange.diff * gl_Position.z/gl_Position.w) +\n";
  shader += "              gl_DepthRange.near + gl_DepthRange.far) / 2.0;\n";
  shader += "   surfId = idx;\n";
  shader += "\n";
  shader += "   zdepthLinear = -(MV * vec4(pointPos, 1.0)).z;\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::oitShaderF()
{
  QString shader;

  shader += "#version 420 core\n";
  shader += "uniform sampler2D diffuseTex;\n";
  shader += "uniform sampler2D normalTex;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform vec4 extras;\n";
  shader += "uniform float sceneRadius;\n";
  shader += "uniform float roughness;\n";
  shader += "uniform float ambient;\n";
  shader += "uniform float diffuse;\n";
  shader += "uniform float specular;\n";
  shader += "uniform int nclip;\n";
  shader += "uniform vec3 clipPos[10];\n";
  shader += "uniform vec3 clipNormal[10];\n";
  shader += "uniform bool hasUV;\n";
  shader += "uniform float featherSize;\n";
  shader += "uniform int applyMaterial;\n";
  shader += "uniform sampler2D matcapTex;\n";
  shader += "uniform float revealTransparency;\n";
  shader += "uniform bool hideBlack;\n";
  shader += "uniform bool hasNormalTex;\n";

  shader += "uniform vec3 rightDir;\n";
  shader += "uniform vec3 upDir;\n";
  shader += "uniform float matMix;\n";

  shader += "uniform vec3 lightDir;\n";

  shader += "in vec3 v3Color;\n";
  shader += "in vec3 v3Normal;\n";
  shader += "in vec3 pointPos;\n";
  shader += "in float zdepth;\n";
  shader += "in float surfId;\n";
  shader += "in vec3 oPosition;\n";
  shader += "in float zdepthLinear;\n";

  shader += "in mat3 TBN;\n";

// Ouput data
  shader += "layout (location=0) out vec4 outputColor;\n";
  shader += "layout (location=1) out vec3 alpha;\n";
  
  shader += "layout (depth_greater) out float gl_FragDepth;\n";

  shader += "void main()\n";
  shader += "{\n";
  shader += "  gl_FragDepth = zdepth;\n";
  

  shader += "  alpha = vec3(1, 1, zdepthLinear/sceneRadius);\n";

  // transparent reveal => edges are more opaque
  shader += "  float NdotV = abs(dot(v3Normal,viewDir));\n";
  
  shader += "  float decay = 1.0 - clamp(NdotV/(1.0-min(extras.y, 0.999)), 0.0, 1.0);\n";
  shader += "  decay = mix(decay, 0.0, step(0.95, extras.y));\n";
  shader += "  alpha.x = max(revealTransparency, sqrt(decay));\n";
  
  
  shader += "  if (hasUV)\n";
  shader += "     outputColor = texture(diffuseTex, vec2(v3Color.x, 1-v3Color.y));\n";
  shader += "  else\n";
  shader += "     outputColor = vec4(v3Color, 1.0);\n";

  
  // hide if black color
  shader += "  if (hideBlack && all(lessThan(outputColor.rgb, vec3(0.01)))) discard; \n";

  
  shader += "  vec3 defCol = mix(vec3(0.0), outputColor.rgb, NdotV*NdotV);\n";
  shader += "  outputColor.rgb = mix(outputColor.rgb, vec3(0.0), pow(decay, 0.5/revealTransparency));\n";
  shader += "  outputColor.rgb = mix(outputColor.rgb, defCol, step(0.95, extras.y));\n";
    
  shader += "  float cfeather = 1.0;\n";
  shader += "  if (nclip > 0)\n";
  shader += "    {\n";
  shader += "      for(int c=0; c<nclip; c++)\n";
  shader += "        {\n";
  shader += "          vec3 cpos = clipPos[c];\n";
  shader += "          vec3 cnorm = clipNormal[c];\n";
  shader += "          float cp = dot(pointPos-cpos, cnorm);\n";
  shader += "          if (cp > 0.0)\n";
  shader += "            discard;\n";
  shader += "          else\n";
  shader += "            cfeather *= smoothstep(0.0, featherSize, -cp);\n";
  shader += "        }\n";
  shader += "      cfeather = 1.0 - cfeather;\n";
  shader += "    }\n";

  shader += "    if (nclip > 0)\n";
  shader += "      outputColor.rgb = mix(outputColor.rgb, vec3(1,1,1), cfeather);\n";


  //---------------------------------------------------------
  //---------------------------------------------------------
  // apply matcap texture
  shader += "  float dx = dot(rightDir,v3Normal)*0.5+0.5;\n";
  shader += "  float dy = dot(upDir,v3Normal)*0.5+0.5;\n";
  shader += "  outputColor.rgb = mix(outputColor.rgb, texture(matcapTex, vec2(dx, 1.0-dy)).rgb, matMix*vec3(step(1, applyMaterial)));\n"; 
  //---------------------------------------------------------
  //---------------------------------------------------------
  

  // glow when active or has glow switched on
  shader += "    outputColor.rgb += pow(outputColor.rgb,vec3(3.0))*max(extras.z,extras.x);\n";

  // desaturate if required
  shader += "  outputColor.rgb *= extras.w;\n";


  //=======================================  
  shader += "  vec3 Amb = ambient * outputColor.rgb;\n";
  shader += "  vec3 Diff = vec3(0.0);\n";

  shader += " vec3 normal = v3Normal;\n";
  shader += " if (hasNormalTex)\n";
  shader += "  {\n";
  shader += "    normal = texture(normalTex, vec2(v3Color.x, 1-v3Color.y)).rgb;\n";
  shader += "    normal = normalize(normal.rgb*2.0 - 1.0);\n";
  //shader += "    normal = normalize(normal.rgb*vec3(2,2,1) - vec3(1,1,0));\n";  // Blender style
  shader += "    normal = normalize(TBN*normal);\n";
  shader += "  }\n";

  shader += "  if (length(normal) > 0.0)\n";
  shader += "  {\n";
  shader += "      float diffMag = pow(abs(dot(normal, lightDir)), 0.5);\n";
  shader += "      Diff = diffuse*diffMag*outputColor.rgb;\n";
  shader += "  }\n";

  //shader += "  float diffMag = pow(abs(dot(v3Normal, viewDir)), 0.5);\n";
  //shader += "  vec3 Diff = diffuse * diffMag*outputColor.rgb;\n";
  shader += "  outputColor.rgb = Amb + Diff;\n";
  shader += "  float specCoeff = 512.0/mix(0.1, 64.0, roughness*roughness);\n";
  shader += "  outputColor.rgb += specular * vec3(pow(abs(dot(v3Normal, lightDir)), specCoeff));\n";
  shader += "  outputColor.rgb = clamp(outputColor.rgb, vec3(0.0), vec3(1.0));\n";
  //=======================================  

  // more glow for higher transparency
  shader += "  outputColor.rgb += vec3(0.2+0.3*extras.y)*outputColor.rgb;\n";

  //=======================================  
  shader += "  float w = 3.0-zdepthLinear/sceneRadius;\n";
  shader += "  outputColor = vec4(outputColor.rgb, 1.0)*alpha.x*pow(w, 2.0);\n";
  //=======================================  
  shader += "}\n";

  return shader;
}

//----------------------------
//----------------------------

//----------------------------
//----------------------------

GLint ShaderFactory::m_oitFinalShaderParm[30];
GLint* ShaderFactory::oitFinalShaderParm() { return &m_oitFinalShaderParm[0]; }

GLuint ShaderFactory::m_oitFinalShader = 0;
GLuint ShaderFactory::oitFinalShader()
{
  if (!m_oitFinalShader)
    {      
      QString vertShaderString = meshShadowShaderV();
      QString fragShaderString = oitFinalShaderF();
  
      m_oitFinalShader = glCreateProgram();
      bool ok = loadShader(m_oitFinalShader,
			   vertShaderString,
			   fragShaderString);
				    
      if (!ok)
	{
	  QMessageBox::information(0, "", "Cannot load oitFinal shaders");
	  return 0;
	}
	
      
      m_oitFinalShaderParm[0] = glGetUniformLocation(m_oitFinalShader, "MVP");
      m_oitFinalShaderParm[1] = glGetUniformLocation(m_oitFinalShader, "oitTex");
      m_oitFinalShaderParm[2] = glGetUniformLocation(m_oitFinalShader, "alphaTex");
      m_oitFinalShaderParm[3] = glGetUniformLocation(m_oitFinalShader, "roughness");
      m_oitFinalShaderParm[4] = glGetUniformLocation(m_oitFinalShader, "specular");
    }
  
  return m_oitFinalShader;      
}

QString
ShaderFactory::oitFinalShaderF()
{
  QString shader;

  shader += "#version 420 core\n";

  shader += "layout(location=0) out vec4 color;\n";

  shader += "uniform sampler2DRect oitTex;\n";
  shader += "uniform sampler2DRect alphaTex;\n";
  shader += "uniform float roughness;\n";
  shader += "uniform float specular;\n";

  shader += "void main()\n";
  shader += "{\n";
  shader += "  vec4 oit = texture2DRect(oitTex, gl_FragCoord.xy);\n";

  shader += "  if (oit.a < 0.01)\n";
  shader += "     discard;\n";

  shader += "  float alpha = texture2DRect(alphaTex, gl_FragCoord.xy).x;\n";
  shader += "  color = vec4(oit.rgb/max(oit.a,0.0001), 1.0)*(1.0-alpha);\n";
  shader += "}\n";

  return shader;
}

//----------------------------
//----------------------------

GLint ShaderFactory::m_meshShaderParm[40];
GLint* ShaderFactory::meshShaderParm() { return &m_meshShaderParm[0]; }

GLuint ShaderFactory::m_meshShader = 0;
GLuint ShaderFactory::meshShader()
{
  if (!m_meshShader)
    {
      m_meshShader = glCreateProgram();
      QString vertShaderString = meshShaderV();
      QString fragShaderString = meshShaderF();
  
      bool ok = loadShader(m_meshShader,
			   vertShaderString,
			   fragShaderString);  

      if (!ok)
	{
	  QMessageBox::information(0, "", "Cannot load mesh shaders");
	  return 0;
	}
	
	m_meshShaderParm[0] = glGetUniformLocation(m_meshShader, "MVP");
	m_meshShaderParm[1] = glGetUniformLocation(m_meshShader, "viewDir");
	m_meshShaderParm[2] = glGetUniformLocation(m_meshShader, "extras");
	m_meshShaderParm[3] = glGetUniformLocation(m_meshShader, "localXform");

	m_meshShaderParm[4] = glGetUniformLocation(m_meshShader,"opacity");

	m_meshShaderParm[5] = glGetUniformLocation(m_meshShader, "roughness");
	m_meshShaderParm[6] = glGetUniformLocation(m_meshShader, "ambient");
	m_meshShaderParm[7] = glGetUniformLocation(m_meshShader, "diffuse");
	m_meshShaderParm[8] = glGetUniformLocation(m_meshShader, "specular");
	m_meshShaderParm[9] = glGetUniformLocation(m_meshShader, "nclip");
	m_meshShaderParm[10] = glGetUniformLocation(m_meshShader,"clipPos");
	m_meshShaderParm[11] = glGetUniformLocation(m_meshShader,"clipNormal");
	m_meshShaderParm[12] = glGetUniformLocation(m_meshShader,"sceneRadius");
	m_meshShaderParm[13] = glGetUniformLocation(m_meshShader,"hasUV");
	m_meshShaderParm[14] = glGetUniformLocation(m_meshShader,"diffuseTex");
	m_meshShaderParm[15] = glGetUniformLocation(m_meshShader,"featherSize");
	m_meshShaderParm[16] = glGetUniformLocation(m_meshShader,"shadowCam");
	m_meshShaderParm[17] = glGetUniformLocation(m_meshShader,"idx");
	m_meshShaderParm[18] = glGetUniformLocation(m_meshShader,"applyMaterial");
	m_meshShaderParm[19] = glGetUniformLocation(m_meshShader,"matcapTex");	
	m_meshShaderParm[20] = glGetUniformLocation(m_meshShader,"matMix");

    	m_meshShaderParm[21] = glGetUniformLocation(m_meshShader,"depthTex");
    	m_meshShaderParm[22] = glGetUniformLocation(m_meshShader,"shadowRender");
    	m_meshShaderParm[23] = glGetUniformLocation(m_meshShader,"screenSize");
    	m_meshShaderParm[24] = glGetUniformLocation(m_meshShader,"MVPShadow");
    	m_meshShaderParm[25] = glGetUniformLocation(m_meshShader,"hideBlack");

	m_meshShaderParm[26] = glGetUniformLocation(m_meshShader,"hasNormalTex");
	m_meshShaderParm[27] = glGetUniformLocation(m_meshShader,"normalTex");
	m_meshShaderParm[28] = glGetUniformLocation(m_meshShader,"lightDir");

	
	m_meshShaderParm[29] = glGetUniformLocation(m_meshShader, "rightDir");
	m_meshShaderParm[30] = glGetUniformLocation(m_meshShader, "upDir");

	m_meshShaderParm[31] = glGetUniformLocation(m_meshShader, "renderingClearView");
	m_meshShaderParm[32] = glGetUniformLocation(m_meshShader, "processClearView");
	m_meshShaderParm[33] = glGetUniformLocation(m_meshShader, "clearViewDepthTex");
    }

  return m_meshShader;
}
  
QString
ShaderFactory::meshShaderV()
{
  QString shader;

  shader += "#version 420 core\n";

  shader += "uniform mat4 MVPShadow;\n";
  shader += "uniform mat4 MVP;\n";
  shader += "uniform mat4 localXform;\n";
  shader += "uniform float idx;\n";
  shader += "uniform vec3 shadowCam;\n";
  shader += "uniform vec2 screenSize;\n";
  shader += "uniform vec3 lightDir;\n";
  
  shader += "layout(location = 0) in vec3 position;\n";
  shader += "layout(location = 1) in vec3 normalIn;\n";
  shader += "layout(location = 2) in vec3 colorIn;\n";
  shader += "layout(location = 3) in vec3 tangentIn;\n";

  shader += "out vec3 v3Normal;\n";
  shader += "out vec3 v3Color;\n";
  shader += "out vec3 pointPos;\n";
  shader += "out float zdepth;\n";
  shader += "out float zdepthS;\n";
  shader += "out float surfId;\n";
  shader += "out vec3 oPosition;\n";
  shader += "out vec3 shadowInfo;\n";
  shader += "out mat3 TBN;\n";
  
  shader += "void main()\n";
  shader += "{\n";
  shader += "   oPosition = position;\n";
  shader += "   pointPos = (localXform * vec4(position, 1)).xyz;\n";
  shader += "   v3Color = colorIn;\n";
  shader += "   v3Normal = normalIn;\n";

  shader += "   vec3 v3Tangent = tangentIn;\n";
  //shader += "   vec3 v3Tangent = normalize(tangentIn-dot(tangentIn, normalIn)*normalIn);\n";
  shader += "   vec3 v3BiTangent = cross(v3Normal, v3Tangent);\n";
  shader += "   TBN = mat3(v3Tangent, v3BiTangent, v3Normal);\n";
  
  shader += "   gl_Position = MVP * vec4(position, 1);\n";
  shader += "   zdepth = ((gl_DepthRange.diff * gl_Position.z/gl_Position.w) +\n";
  shader += "              gl_DepthRange.near + gl_DepthRange.far) / 2.0;\n";
  shader += "   surfId = idx;\n";

  //shader += "   zdepthS = length(pointPos-shadowCam);\n";
  shader += "   zdepthS = dot(lightDir,(pointPos-shadowCam));\n"; // ORTHOGRAPIC
  shader += "   vec4 shadowPos = MVPShadow * vec4(pointPos, 1);\n";
  shader += "   vec2 xy = shadowPos.xy/vec2(shadowPos.w);\n";
  shader += "   xy = (xy + vec2(1.0)) * vec2(0.5);\n";
  shader += "   xy = xy * screenSize;\n";
  shader += "   shadowInfo = vec3(xy, zdepthS);\n";
  
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::meshShaderF()
{
  QString shader;

  shader += "#version 420 core\n";
  shader += "uniform sampler2D diffuseTex;\n";
  shader += "uniform sampler2D normalTex;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform vec4 extras;\n";
  shader += "uniform float sceneRadius;\n";
  shader += "uniform float roughness;\n";
  shader += "uniform float ambient;\n";
  shader += "uniform float diffuse;\n";
  shader += "uniform float specular;\n";
  shader += "uniform int nclip;\n";
  shader += "uniform vec3 clipPos[10];\n";
  shader += "uniform vec3 clipNormal[10];\n";
  shader += "uniform bool hasUV;\n";
  shader += "uniform float featherSize;\n";
  shader += "uniform int applyMaterial;\n";
  shader += "uniform sampler2D matcapTex;\n";
  shader += "uniform float opacity;\n";

  shader += "uniform sampler2DRect depthTex;\n";
  shader += "uniform bool shadowRender;\n";
  shader += "uniform bool hideBlack;\n";
  shader += "uniform bool hasNormalTex;\n";

  shader += "uniform vec3 lightDir;\n";
  
  shader += "uniform vec3 rightDir;\n";
  shader += "uniform vec3 upDir;\n";
  shader += "uniform float matMix;\n";

  shader += "uniform bool renderingClearView;\n";
  shader += "uniform bool processClearView;\n";
  shader += "uniform sampler2DRect clearViewDepthTex;\n";
  
  
  shader += "\n";
  shader += "in vec3 v3Color;\n";
  shader += "in vec3 v3Normal;\n";
  shader += "in vec3 pointPos;\n";
  shader += "in float zdepth;\n";
  shader += "in float zdepthS;\n";
  shader += "in float surfId;\n";
  shader += "in vec3 oPosition;\n";
  shader += "in vec3 shadowInfo;\n";

  shader += "in mat3 TBN;\n";
  
  // Ouput data
  shader += "layout (location=0) out vec4 outputColor;\n";
  shader += "layout (location=1) out vec4 depth;\n";
  shader += "layout (depth_greater) out float gl_FragDepth;\n";
  
  shader += rgb2hsv();
  shader += hsv2rgb();
  //shader += OrenNayarDiffuseShader();

  shader += "void main()\n";
  shader += "{\n";

  //----------------------
  //Clear View
  shader += "  if (processClearView)\n";
  shader += "  {\n";
  shader += "    if (zdepthS/sceneRadius < texture2DRect(clearViewDepthTex, gl_FragCoord.xy).x &&";
  shader += "        texture2DRect(clearViewDepthTex, gl_FragCoord.xy).b > 1.0) discard;\n";
  shader += "  }\n";
  //----------------------



  shader += "  float NdotV = dot(v3Normal,-viewDir);\n";
  shader += "  if (extras.y >= 0.0 && NdotV > extras.y) discard;\n";

  //shader += "  if (dot(v3Normal,-viewDir) > extras.y) discard;\n";
  //shader += "  if (extras.y < 0.0 && abs(NdotV) > 1.0+extras.y) discard;\n";
  
  shader += "gl_FragDepth = zdepth;\n";


  //------------
  // when rendering clearview
  shader += "outputColor = vec4(1);\n";
  shader += "depth = vec4(zdepthS/sceneRadius, 0, max(10.0, 15.0 - 5*zdepthS/sceneRadius), gl_FragDepth);\n";
  shader += "if (renderingClearView) return;";
  //------------

  

  //  //-------------------  store depth values
  shader += "float inShadow = 0.0;\n";
  shader += "float shadowZ = zdepthS/sceneRadius;\n";
  shader += "if (shadowRender)\n";
  shader += "{\n"; // create shadow map
  shader += "  depth = vec4(shadowZ, extras.z, surfId, gl_FragDepth);\n";
//  //-------------------  Camera depth,  glow,  surface id, fragment depth
  shader += "}\n";
  shader += "else\n";
  shader += "{\n";  // use the shadow map to detect shadows
  shader += "  shadowZ = texture2DRect(depthTex, shadowInfo.xy).x;\n";
  shader += "  inShadow = step(shadowZ+0.01, zdepthS/sceneRadius);\n";
  shader += "  depth = vec4(zdepthS/sceneRadius, extras.z, surfId, gl_FragDepth);\n";
//  //-------------------  Camera depth,  glow,  surface id, fragment depth
  shader += "}\n";
 
  
  shader += "  if (hasUV)\n";
  shader += "     outputColor = texture(diffuseTex, vec2(v3Color.x, 1-v3Color.y));\n";
  shader += "  else\n";
  shader += "     outputColor = vec4(v3Color, 1.0);\n";

  //shader += "  outputColor *= (1.0-0.1*inShadow);\n";
  
  // hide if black color
  shader += "  if (hideBlack && all(lessThan(outputColor.rgb, vec3(0.01)))) discard; \n";

  
  shader += "float cfeather = 1.0;\n";
  shader += "if (nclip > 0)\n";
  shader += "  {\n";
  shader += "    for(int c=0; c<nclip; c++)\n";
  shader += "      {\n";
  shader += "        vec3 cpos = clipPos[c];\n";
  shader += "        vec3 cnorm = clipNormal[c];\n";
  shader += "        float cp = dot(pointPos-cpos, cnorm);\n";
  shader += "        if (cp > 0.0)\n";
  shader += "          discard;\n";
  shader += "        else\n";
  shader += "          cfeather *= smoothstep(0.0, featherSize, -cp);\n";
  shader += "      }\n";
  shader += "    cfeather = 1.0 - cfeather;\n";
  shader += "  }\n";

  shader += "  if (nclip > 0)\n";
  shader += "    outputColor.rgb = mix(outputColor.rgb, vec3(1,1,1), cfeather);\n";

  shader += "\n";
  shader += "  vec3 Amb = ambient*outputColor.rgb;\n";
  shader += "  vec3 Diff = vec3(0.0);\n";

  shader += " vec3 normal = v3Normal;\n";
  shader += " if (hasNormalTex)\n";
  shader += "  {\n";
  shader += "    normal = texture(normalTex, vec2(v3Color.x, 1-v3Color.y)).rgb;\n";

  shader += "    normal = normalize(normal.rgb*2.0 - 1.0);\n";
  
  //change the fragment depth as well so that we can access it in shadowshader
  //shader += "    depth.x -= mix(0.0, 0.05*dot(normal,viewDir), smoothstep(0.055, 0.1, depth.x));\n";
  shader += "    depth.x += 0.05*dot(normal,viewDir);\n";

  //shader += "    normal = normalize(normal.rgb*vec3(2,2,1) - vec3(1,1,0));\n";  // Blender style
  shader += "    normal = normalize(TBN*normal);\n";
  shader += "  }\n";


  shader += "  if (length(normal) > 0.0)\n";
  shader += "  {\n";
  //shader += "      float diffMag = OrenNayarDiffuse(normal, viewDir, lightDir, diffuse);\n";
  shader += "      float diffMag = pow(abs(dot(normal, lightDir)), 0.5);\n";
  shader += "      Diff = diffuse*diffMag*outputColor.rgb;\n";
  shader += "  }\n";
  shader += "  outputColor.rgb = Amb + Diff;\n";

//  shader += "  float specCoeff = 512.0/mix(0.1, 64.0, roughness*roughness);\n";
//  shader += "  float shine = specular * pow(abs(dot(viewDir, reflect(lightDir, normal))), specCoeff);\n";
//  shader += "  outputColor.rgb = mix(outputColor.rgb, pow(outputColor.rgb, vec3(1.0-shine)), vec3(shine));\n";
//  shader += "  outputColor.rgb = clamp(outputColor.rgb, vec3(0.0), vec3(1.0));\n";


  
  //---------------------------------------------------------
  //---------------------------------------------------------
  // apply matcap texture
  shader += "  float dx = dot(rightDir,v3Normal)*0.5+0.5;\n";
  shader += "  float dy = dot(upDir,v3Normal)*0.5+0.5;\n";
  shader += "  outputColor.rgb = mix(outputColor.rgb, (ambient+diffuse)*texture(matcapTex, vec2(dx, 1.0-dy)).rgb, matMix*vec3(step(1, applyMaterial)));\n";
  //---------------------------------------------------------
  //---------------------------------------------------------
  

  // glow when active or has glow switched on
  shader += "    outputColor.rgb += pow(outputColor.rgb,vec3(3.0))*max(extras.z,extras.x);\n";

  // pass on darkening, shadow, (opacity+outline) parameters to next stage
  shader += "  outputColor.a = 10000.0*floor(10.0*extras.w + 0.4) + 1000.0*inShadow + opacity;\n";
  
  shader += "}\n";
    
  return shader;
}

GLint ShaderFactory::m_meshShadowShaderParm[20];
GLint* ShaderFactory::meshShadowShaderParm() { return &m_meshShadowShaderParm[0]; }

GLuint ShaderFactory::m_meshShadowShader = 0;
GLuint ShaderFactory::meshShadowShader()
{
  if (!m_meshShadowShader)
    {
      m_meshShadowShader = glCreateProgram();
      QString vertShaderString = meshShadowShaderV();
      QString fragShaderString = meshShadowShaderF();
  
      bool ok = loadShader(m_meshShadowShader,
			   vertShaderString,
			   fragShaderString);  

      if (!ok)
	{
	  QMessageBox::information(0, "", "Cannot load mesh shadow shaders");
	  return 0;
	}
	
	m_meshShadowShaderParm[0] = glGetUniformLocation(m_meshShadowShader, "MVP");
	m_meshShadowShaderParm[1] = glGetUniformLocation(m_meshShadowShader, "colorTex");
	m_meshShadowShaderParm[2] = glGetUniformLocation(m_meshShadowShader, "depthTex");
	m_meshShadowShaderParm[3] = glGetUniformLocation(m_meshShadowShader, "softshadow");
	m_meshShadowShaderParm[4] = glGetUniformLocation(m_meshShadowShader, "edges");
	m_meshShadowShaderParm[5] = glGetUniformLocation(m_meshShadowShader, "gamma");
	m_meshShadowShaderParm[6] = glGetUniformLocation(m_meshShadowShader, "roughness");	
	m_meshShadowShaderParm[7] = glGetUniformLocation(m_meshShadowShader, "specular");	

	m_meshShadowShaderParm[10] = glGetUniformLocation(m_meshShadowShader, "shadowIntensity");
	m_meshShadowShaderParm[11] = glGetUniformLocation(m_meshShadowShader, "valleyIntensity");
	m_meshShadowShaderParm[12] = glGetUniformLocation(m_meshShadowShader, "peakIntensity");
    }

  return m_meshShadowShader;
}

QString
ShaderFactory::meshShadowShaderV()
{
  QString shader;
  shader += "#version 420 core\n";
  shader += "\n";
  //shader += "layout(location = 0) in vec3 vertex;\n";
  shader += "layout(location = 0) in vec2 vertex;\n";
  shader += "uniform mat4 MVP;\n";
  shader += "\n";
  shader += "void main()\n";
  shader += "{\n";
  shader += "  gl_Position =  MVP * vec4(vertex,0,1);\n";
  shader += "}\n";
  return shader;
}

QString
ShaderFactory::meshShadowShaderF()
{
  QString shader;

  shader += "#version 420 core\n";
 
  // Ouput data
  shader += "layout(location=0) out vec4 color;\n";
  shader += "layout(depth_greater) out float gl_FragDepth;\n";
		      
  shader += "uniform sampler2DRect colorTex;\n";
  shader += "uniform sampler2DRect depthTex;\n";
  shader += "uniform float softshadow;\n";
  shader += "uniform float edges;\n";
  shader += "uniform float gamma;\n";
  shader += "uniform float roughness;\n";
  shader += "uniform float specular;\n";

  shader += "uniform float shadowIntensity;\n";
  shader += "uniform float valleyIntensity;\n";
  shader += "uniform float peakIntensity;\n";

  shader += rgb2hsv();
  shader += hsv2rgb();
  shader += ggxShader();
  
  shader += "void main()\n";
  shader += "{\n";
  
  shader += "  vec2 spos = gl_FragCoord.xy;\n";

  shader += "  color = texture2DRect(colorTex, spos.xy);\n";
  //shader += "  if (color.a < 0.001)\n";
  //shader += "    discard;\n";

  shader += "  vec4 ocolor = color;\n";
  
  // dropShadow
  shader += "  if (color.a < 0.001)\n";
  shader += "    {\n";
  shader += "      int nsteps = int(30.0*softshadow);\n";
  shader += "      float dropS = 0;\n";
  shader += "      for(int i=0; i<nsteps; i++)\n";
  shader += "        {\n";
  shader += "    	 float r = 1.0+i*0.11;\n";
  shader += "            float x = r*sin(radians(i*17));\n";
  shader += "            float y = r*cos(radians(i*17));\n";
  shader += "    	 vec2 pos = spos + vec2(x,y);\n";
  shader += "    	 float adepth = texture2DRect(depthTex, pos).x;\n";
  shader += "            dropS += step(0.001, adepth);\n";
  shader += "        }\n";
  shader += "      dropS /= float(nsteps);\n";
  shader += "      if (dropS < 0.01) discard;\n";
  shader += "      color.a = dropS;\n";
  shader += "      color.rgb *= vec3(dropS);\n";
  shader += "      gl_FragDepth = 0.9999;\n";
  shader += "      return;\n";
  shader += "    }\n";

  
  shader += "  float darken = 0.1*(int(color.a)/10000.0);\n";
  shader += "  darken = 1.0-0.8*darken;\n";
  
  shader += "  color.a = 1.0;\n";
  
  
  shader += "  vec4 dtex = texture2DRect(depthTex, spos.xy);\n";
  shader += "  float depth = dtex.x;\n";

  
  //------------------------
  // dtex.x - shadow depth for original camera
  // dtex.y - glow
  // dtex.z - surface id
  // dtex.w - fragDepth
  // depthTexS.x - shadow depth with shadow camera
  //------------------------
  
  shader += "  if (depth < 0.001) \n";
  shader += "  {\n";
  shader += "     gl_FragDepth = 1.0;\n";
  shader += "     return;\n";
  shader += "  }\n";

  shader += "  gl_FragDepth = dtex.w;\n";
  
  
  shader += "  float surfId = dtex.z;\n";

  
  //----------------------
  // specular highlights
  shader += "  vec3 shine = vec3(0.0);\n";
  shader += "  {\n";
  shader += "    float dx = 0.0;\n";
  shader += "    float dy = 0.0;\n";
  shader += "    for(int i=0; i<10; i++)\n";
  shader += "      {\n";
  shader += "        dx += texture2DRect(depthTex, spos.xy+vec2(1+i,0.0)).x -\n";
  shader += "              texture2DRect(depthTex, spos.xy-vec2(1+i,0.0)).x;\n";
  shader += "        dy += texture2DRect(depthTex, spos.xy+vec2(0.0,1+i)).x -\n";
  shader += "              texture2DRect(depthTex, spos.xy-vec2(0.0,1+i)).x;\n";
  shader += "      }\n";
  shader += "    vec3 N = normalize(vec3(5*dx*(1.0+0.05*depth), 5*dy*(1.0+0.05*depth), 0.25*(1.0-0.5*roughness)));\n";
  shader += "    float specCoeff = 512.0/mix(0.1, 64.0, roughness*roughness);\n";
  shader += "    shine = 0.5 * specular * vec3(pow(N.z, specCoeff));\n";
  shader += "    color.rgb = pow(color.rgb,vec3(1.0)-0.25*shine);\n";
  shader += "  }\n";
  //----------------------
  
  
  
  //----------------------
  shader += "  float dedge = 1.0;\n";
  //reduce the edges if glowing
  //shader += "  if (edges*step(dtex.y,0.001) > 0.0)\n";
  shader += "  if (edges > 0.0)\n";
  shader += "  {\n"; 
  shader += "    float response = 0.0;\n";

  // find edges on surfaces
  shader += "    for(int i=0; i<8; i++)\n";
  shader += "      {\n";
  shader += "        float x = sin(radians(i*45));\n";
  shader += "        float y = cos(radians(i*45));\n";
  shader += "        vec2 pos = spos + vec2(x,y);\n";
  shader += "        float adepth = texture2DRect(depthTex, pos).x;\n";
  shader += "        float od = depth - adepth;\n";
  shader += "        response += od;\n";
  shader += "      } \n";
  shader += "    response = max(0.0, response);\n";
  shader += "    dedge *= max(1.0-1.0/(1.0+edges), exp(-response*pow(1000*edges,1.0/(1.0+0.05*depth))));\n";

  
  // find border between different surfaces
  shader += "    response = 0.0;\n";
  shader += "    for(int i=0; i<8; i++)\n";
  shader += "      {\n";
  shader += "        float x = sin(radians(i*45));\n";
  shader += "        float y = cos(radians(i*45));\n";
  shader += "        vec2 pos = spos + vec2(x,y);\n";
  shader += "        float od = step(0.001, abs(surfId - texture2DRect(depthTex, pos).z));\n";
  shader += "        response += max(0.0, od);\n";
  shader += "      } \n";
  shader += "    dedge *= exp(-min(0.1, response/8.0));\n";
  shader += "  }\n";

  
  // find bright edges on surfaces
  shader += "  float bedge = 0.0;\n";
  shader += "  if (peakIntensity > 0.0)\n";
  shader += "  {\n"; 
  shader += "    float response = 0.0;\n";
  shader += "    for(int i=0; i<8; i++)\n";
  shader += "      {\n";
  shader += "        float x = sin(radians(i*45));\n";
  shader += "        float y = cos(radians(i*45));\n";
  shader += "        vec2 pos = spos + vec2(x,y);\n";
  shader += "        float adepth = texture2DRect(depthTex, pos).x;\n";
  shader += "        float od = adepth - depth;\n";
  shader += "        response += od;\n";
  shader += "      } \n";
  shader += "    bedge = 1.0 - exp(-abs(response)*pow(300*peakIntensity,1.0/(1.0+0.05*depth)));\n";
  shader += "  }\n";  

  
  shader += "  {\n";  

  shader += "    int shadowSamples = 25;\n";
  shader += "    float vstart = 0.001;\n";
  shader += "    float vend = 0.07 + valleyIntensity*0.05;\n";
  shader += "    float vint = vend-vstart;\n";

  
  // shadows
  shader += "    float sumS = 0.0;\n";
  shader += "    for(int i=0; i<shadowSamples; i++)\n";
  shader += "      {\n";  
  shader += "    	 float r = 1.0+i*0.11;\n";
  shader += "            float x = r*sin(radians(i*23));\n";
  shader += "            float y = r*cos(radians(i*23));\n";
  shader += "    	 vec2 pos = spos + vec2(x,y);\n";
  shader += "    	 vec4 tmp = texture2DRect(colorTex, pos);\n";
  shader += "            float inShadow = float(int(mod(tmp.a,10000.0))/1000);\n";
  shader += "    	 sumS += pow(1.0-float(i)/float(shadowSamples), 0.25)*step(0.001,inShadow);\n";
  shader += "      }\n";  
  

  shader += "    float sumG = 0.0;\n";
  shader += "    vec3 clrG = vec3(0.0);\n";
  shader += "    float nValley = 0.0;\n";

  shader += "    int nsteps = int(40.0*softshadow + 10.0);\n";

  //---------------
  // fibonacci spiral sampling
  shader += "    float shift = 1.0;\n";
  shader += "    float ga = 180*(3.0-sqrt(5.0));\n";
  shader += "    float boundary = round(2*sqrt(nsteps));\n";
  shader += "    for(int i=0; i<nsteps; i++)\n";
  shader += "      {\n";  
  shader += "        float r = sqrt(i+0.5)/(nsteps-0.5*(boundary+1));\n";
  shader += "        r *= 30*softshadow;\n";
  shader += "        float phi = ga*(i+shift);\n";
  shader += "        float x = r*sin(radians(phi));\n";
  shader += "        float y = r*cos(radians(phi));\n";

  shader += "        vec2 pos = spos + vec2(x,y);\n";
  shader += "        vec3 adepth = texture2DRect(depthTex, pos).xyz;\n";
  
  shader += "        float zdif = depth-adepth.x;\n";

  shader += "        float vnearEnough = clamp(zdif, vstart, vend);\n";
  shader += "        vnearEnough = 1.0 - (vnearEnough-vstart)/vint;\n";
  shader += "        nValley += max(0.0,zdif)*vnearEnough*(1.0-0.9*float(i)/float(nsteps));\n";
  
  // get contributions for glowing surfaces
  shader += "        float sr = step(0.001, adepth.y);\n";
  shader += "        sumG += sr;\n";
  shader += "        vec4 tmp = texture2DRect(colorTex, pos);\n";
  shader += "        clrG += sr*tmp.rgb;\n";
  shader += "      }\n";  
  //---------------

  
//  //---------------
//  // add glow
  shader += "    clrG /= vec3(max(1.0,sumG));\n";
  shader += "    float gl = sumG/float(nsteps);\n";
  shader += "    color.rgb = mix(color.rgb, pow(clrG, vec3(1.0/max(0.1,gl))), 0.1*gl);\n";
//  //---------------


  //---------------
  // add valley-1
  shader += "    float valley1 = pow(exp(-0.5*nValley), valleyIntensity*valleyIntensity);\n";
  shader += "    color.rgb *= max(0.75, valley1);\n";
  //---------------
  
 
  //----------------------
  // add edges-1
  shader += "    color.rgb = mix(color.rgb, dedge*color.rgb, pow(edges,0.5));\n";
  shader += "    color.rgb = pow(color.rgb,vec3(1.0-0.75*bedge));\n";
  //----------------------


  //----------------------
  // add shadow
  shader += "    sumS /= float(shadowSamples);\n";
  shader += "    float shadow = sumS;\n";
  shader += "    float shadows = exp(-shadow*shadowIntensity*shadowIntensity);\n";
  shader += "    color.rgb *= max(0.5,shadows);\n";
  //----------------------


  //---------------
  // add valley-2
  shader += "    float valley2 = pow(exp(-0.5*nValley), valleyIntensity*valleyIntensity);\n";
  shader += "    color.rgb *= max(0.5, valley2);\n";
  //---------------
  

  //----------------------
  // add edges-2
  //shader += "    color.rgb = mix(color.rgb, pow(color.rgb,vec3(1.0-0.5*bedge)), pow(bedge, 2.0));\n";
  //shader += "    color.rgb = mix(color.rgb, dedge*color.rgb, pow(edges,2.0));\n";
  //----------------------


  
  //---------------
  // add reflected glow to non glowing region
  shader += "    color.rgb = mix(color.rgb, clrG, step(dtex.y, 0.001)*pow(gl,0.5));\n";
  //---------------
 
  
  shader += "  }\n";

  
  //----------------------
  // specular highlights on top
  shader += "    color.rgb = mix(color.rgb, color.rgb+shine, color.rgb);\n";
  //----------------------
  

  //----------------------
  // darken if required
  shader += "  if (darken < 1.0)\n";
  shader += "  {\n";
  shader += "    vec3 outputColor = rgb2hsv(color.rgb);\n";
  shader += "    outputColor.yz *= darken;\n";
  shader += "    color.rgb = hsv2rgb(outputColor.xyz);\n";
  shader += "  }\n";
  //----------------------

  shader += " color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));\n";

  shader += " color.rgb = pow(color.rgb, vec3(1.0/gamma));\n";


  shader += "}\n";

  return shader;
}


//----------------------------
//----------------------------


GLint ShaderFactory::m_outlineShaderParm[30];
GLint* ShaderFactory::outlineShaderParm() { return &m_outlineShaderParm[0]; }

GLuint ShaderFactory::m_outlineShader = 0;
GLuint ShaderFactory::outlineShader()
{
  if (!m_outlineShader)
    {
      m_outlineShader = glCreateProgram();
      QString vertShaderString = meshShadowShaderV();
      QString fragShaderString = outlineShaderF();
  
      bool ok = loadShader(m_outlineShader,
			   vertShaderString,
			   fragShaderString);  

      if (!ok)
	{
	  QMessageBox::information(0, "", "Cannot load outline shaders");
	  return 0;
	}
	
//	m_outlineShaderParm[0] = glGetUniformLocation(m_outlineShader, "MVP");
//	m_outlineShaderParm[1] = glGetUniformLocation(m_outlineShader, "colorTex");
//	m_outlineShaderParm[2] = glGetUniformLocation(m_outlineShader, "depthTex");
//	m_outlineShaderParm[3] = glGetUniformLocation(m_outlineShader, "gamma");
//	m_outlineShaderParm[4] = glGetUniformLocation(m_outlineShader, "roughness");	
//	m_outlineShaderParm[5] = glGetUniformLocation(m_outlineShader, "specular");	
//	m_outlineShaderParm[6] = glGetUniformLocation(m_outlineShader, "softshadow");	
//	m_outlineShaderParm[7] = glGetUniformLocation(m_outlineShader, "edges");	

    	m_outlineShaderParm[0] = glGetUniformLocation(m_outlineShader, "MVP");
	m_outlineShaderParm[1] = glGetUniformLocation(m_outlineShader, "colorTex");
	m_outlineShaderParm[2] = glGetUniformLocation(m_outlineShader, "depthTex");
	m_outlineShaderParm[3] = glGetUniformLocation(m_outlineShader, "softshadow");
	m_outlineShaderParm[4] = glGetUniformLocation(m_outlineShader, "edges");
	m_outlineShaderParm[5] = glGetUniformLocation(m_outlineShader, "gamma");
	m_outlineShaderParm[6] = glGetUniformLocation(m_outlineShader, "roughness");	
	m_outlineShaderParm[7] = glGetUniformLocation(m_outlineShader, "specular");	
	m_outlineShaderParm[10]= glGetUniformLocation(m_outlineShader, "shadowIntensity");
	m_outlineShaderParm[11]= glGetUniformLocation(m_outlineShader, "valleyIntensity");
	m_outlineShaderParm[12]= glGetUniformLocation(m_outlineShader, "peakIntensity");
	m_outlineShaderParm[13]= glGetUniformLocation(m_outlineShader, "grabbedColor");
    }

  return m_outlineShader;
}

QString
ShaderFactory::outlineShaderF()
{
  QString shader;

  shader += "#version 420 core\n";
 
  // Ouput data
  shader += "layout(location=0) out vec4 color;\n";
  shader += "layout(depth_greater) out float gl_FragDepth;\n";
		      
  shader += "uniform sampler2DRect colorTex;\n";
  shader += "uniform sampler2DRect depthTex;\n";
  shader += "uniform float softshadow;\n";
  shader += "uniform float edges;\n";
  shader += "uniform float gamma;\n";
  shader += "uniform float roughness;\n";
  shader += "uniform float specular;\n";

  shader += "uniform float shadowIntensity;\n";
  shader += "uniform float valleyIntensity;\n";
  shader += "uniform float peakIntensity;\n";
  shader += "uniform vec3 grabbedColor;\n";

  shader += rgb2hsv();
  shader += hsv2rgb();
  shader += ggxShader();
  
  shader += "void main()\n";
  shader += "{\n";
  
  shader += "  vec2 spos = gl_FragCoord.xy;\n";

  shader += "  color = texture2DRect(colorTex, spos.xy);\n";
  shader += "  if (color.a < 0.001)\n";
  shader += "    discard;\n";
  
  
  shader += "  float darken = 0.1*(int(color.a)/10000.0);\n";
  shader += "  darken = 1.0-0.8*darken;\n";
  
  //shader += "  color.a = 1.0;\n";
  
  
  shader += "  vec4 dtex = texture2DRect(depthTex, spos.xy);\n";
  shader += "  float depth = dtex.x;\n";

  
  //------------------------
  // dtex.x - shadow depth for original camera
  // dtex.y - glow
  // dtex.z - surface id
  // dtex.w - fragDepth
  //------------------------
  
  shader += "  if (depth < 0.001) \n";
  shader += "  {\n";
  shader += "     gl_FragDepth = 1.0;\n";
  shader += "     return;\n";
  shader += "  }\n";

  shader += "  gl_FragDepth = dtex.w;\n";
  
  
  shader += "  float surfId = dtex.z;\n";
  
  shader += "  float opout = float(mod(mod(color.a,10000.0),1000.0));\n";
  shader += "  float outline = floor(mod(opout,11.0));\n"; 
  shader += "  float opacity = 1.0-float(int(opout)/11)/10.0;\n";
  shader += "  vec3 ecolor = color.rgb;\n";
  shader += "  vec3 bcolor = color.rgb;\n";
  shader += "  if (any(greaterThan(grabbedColor, vec3(0,0,0)))) bcolor = grabbedColor;\n";
  

  //----------------------
  shader += "  float dedge = 1.0;\n";
  //reduce the edges if glowing
  //shader += "  if (edges*step(dtex.y,0.001) > 0.0)\n";
  shader += "  if (edges > 0.0)\n";
  shader += "  {\n"; 
  shader += "    float response = 0.0;\n";

  // find edges on surfaces
  shader += "    for(int i=0; i<8; i++)\n";
  shader += "      {\n";
  shader += "        float x = sin(radians(i*45));\n";
  shader += "        float y = cos(radians(i*45));\n";
  shader += "        vec2 pos = spos + vec2(x,y);\n";
  shader += "        float adepth = texture2DRect(depthTex, pos).x;\n";
  shader += "        float od = depth - adepth;\n";
  shader += "        response += od;\n";
  shader += "      } \n";
  shader += "    response = max(0.0, response);\n";
  shader += "    dedge *= max(1.0-1.0/(1.0+edges), exp(-response*pow(1000*edges,1.0/(1.0+0.05*depth))));\n";
  shader += "  }\n";

  
  shader += "  {\n";
  // find border between different surfaces
  shader += "    float response = 0.0;\n";
  shader += "    int bsteps = int(8.0*outline);\n";
  shader += "    for(int i=0; i<bsteps; i++)\n";
  shader += "      {\n";
  shader += "        float r = 1.0 + i*0.13;\n";
  shader += "        float x = r*sin(radians(i*43));\n";
  shader += "        float y = r*cos(radians(i*43));\n";
  shader += "        vec2 pos = spos + vec2(x,y);\n";
  shader += "        vec3 adepth = texture2DRect(depthTex, pos).xyz;\n";
  // different depths
  shader += "        float od = depth - adepth.x;\n";
  // border between different surfaces
  shader += "        od += step(0.001, abs(surfId - adepth.z));\n"; 
  shader += "        response += max(0.0, od);\n";
  shader += "      } \n";
  shader += "    response /= float(bsteps);\n"; 
  shader += "   float border = clamp(response,0.0,1.0);\n";
  shader += "   border = exp(-border/gamma);\n";
  // outline for selections on dark background
  shader += "   float fadein = mix(border, 1.0, step(19.0, softshadow));\n";
  //shader += "   color = mix(vec4(bcolor.rgb*fadein,1.0), vec4(color.rgb, 1.0)*opacity, pow(border, 3.0));\n";
  shader += "   color = mix(vec4(bcolor,1.0), vec4(color.rgb, 1.0)*opacity, border);\n";
  shader += "  }\n";

  
  // find bright edges on surfaces
  shader += "  float bedge = 0.0;\n";
  shader += "  if (peakIntensity > 0.0)\n";
  shader += "  {\n"; 
  shader += "    float response = 0.0;\n";
  shader += "    for(int i=0; i<8; i++)\n";
  shader += "      {\n";
  shader += "        float x = sin(radians(i*45));\n";
  shader += "        float y = cos(radians(i*45));\n";
  shader += "        vec2 pos = spos + vec2(x,y);\n";
  shader += "        float adepth = texture2DRect(depthTex, pos).x;\n";
  shader += "        float od = adepth - depth;\n";
  shader += "        response += od;\n";
  shader += "      } \n";
  shader += "    bedge = 1.0 - exp(-abs(response)*pow(300*peakIntensity,1.0/(1.0+0.05*depth)));\n";
  shader += "  }\n";  

  
  shader += "  {\n";  

  shader += "    int shadowSamples = 25;\n";
  shader += "    float vstart = 0.01;\n";
  shader += "    float vend = 0.07 + valleyIntensity*0.05;\n";
  shader += "    float vint = vend-vstart;\n";

  
//  // shadows
//  shader += "    float sumS = 0.0;\n";
//  shader += "    for(int i=0; i<shadowSamples; i++)\n";
//  shader += "      {\n";  
//  shader += "    	 float r = 1.0+i*0.11;\n";
//  shader += "            float x = r*sin(radians(i*23));\n";
//  shader += "            float y = r*cos(radians(i*23));\n";
//  shader += "    	 vec2 pos = spos + vec2(x,y);\n";
//  shader += "    	 vec4 tmp = texture2DRect(colorTex, pos);\n";
//  shader += "            float inShadow = float(int(mod(tmp.a,10000.0))/1000);\n";
//  shader += "    	 sumS += pow(1.0-float(i)/float(shadowSamples), 0.25)*step(0.001,inShadow);\n";
//  shader += "      }\n";  
  

  shader += "    float sumG = 0.0;\n";
  shader += "    vec3 clrG = vec3(0.0);\n";
  shader += "    float nValley = 0.0;\n";

  shader += "    int nsteps = int(40.0*min(10.0,softshadow) + 10.0);\n";

  //---------------
  // fibonacci spiral sampling
  shader += "    float shift = 1.0;\n";
  shader += "    float ga = 180*(3.0-sqrt(5.0));\n";
  shader += "    float boundary = round(2*sqrt(nsteps));\n";
  shader += "    for(int i=0; i<nsteps; i++)\n";
  shader += "      {\n";  
  shader += "        float r = sqrt(i+0.5)/(nsteps-0.5*(boundary+1));\n";
  shader += "        r *= 30*min(10.0,softshadow);\n";
  shader += "        float phi = ga*(i+shift);\n";
  shader += "        float x = r*sin(radians(phi));\n";
  shader += "        float y = r*cos(radians(phi));\n";

  shader += "        vec2 pos = spos + vec2(x,y);\n";
  shader += "        vec3 adepth = texture2DRect(depthTex, pos).xyz;\n";
  
  shader += "        float zdif = depth-adepth.x;\n";

  shader += "        float vnearEnough = clamp(zdif, vstart, vend);\n";
  shader += "        vnearEnough = 1.0 - (vnearEnough-vstart)/vint;\n";
  shader += "        nValley += max(0.0,zdif)*vnearEnough*(1.0-0.9*float(i)/float(nsteps));\n";
  
  // get contributions for glowing surfaces
  shader += "        float sr = step(0.001, adepth.y);\n";
  shader += "        sumG += sr;\n";
  shader += "        vec4 tmp = texture2DRect(colorTex, pos);\n";
  shader += "        clrG += sr*tmp.rgb;\n";
  shader += "      }\n";  
  //---------------

  
//  //---------------
//  // add glow
  shader += "    clrG /= vec3(max(1.0,sumG));\n";
  shader += "    float gl = sumG/float(nsteps);\n";
  shader += "    color.rgb = mix(color.rgb, pow(clrG, vec3(1.0/max(0.1,gl))), 0.1*gl);\n";
//  //---------------


  //---------------
  // add valley-1
  shader += "    float valley1 = pow(exp(-0.5*nValley), valleyIntensity*valleyIntensity);\n";
  shader += "    color.rgb *= max(0.75, valley1);\n";
  //---------------
  
 
  //----------------------
  // add edges-1
  //shader += "    color.rgb = mix(color.rgb, dedge*color.rgb, pow(edges,0.5));\n";
  shader += "    color.rgb = pow(color.rgb,vec3(1.0-0.75*bedge));\n";
  //----------------------


//  //----------------------
//  // add shadow
//  shader += "    sumS /= float(shadowSamples);\n";
//  shader += "    float shadow = sumS;\n";
//  shader += "    float shadows = exp(-shadow*shadowIntensity*shadowIntensity);\n";
//  shader += "    color.rgb *= max(0.5,shadows);\n";
//  //----------------------


  //---------------
  // add valley-2
  shader += "    float valley2 = pow(exp(-0.5*nValley), valleyIntensity*valleyIntensity);\n";
  shader += "    color.rgb *= max(0.5, valley2);\n";
  //---------------
  

  
  //---------------
  // add reflected glow to non glowing region
  shader += "    color.rgb = mix(color.rgb, clrG, step(dtex.y, 0.001)*pow(gl,0.5));\n";
  //---------------
 
  
  shader += "  }\n";

  
  //----------------------
  // specular highlights on top
  //----------------------
  // specular highlights
  shader += "  vec3 shine = vec3(0.0);\n";
  shader += "  {\n";
  shader += "    float dx = 0.0;\n";
  shader += "    float dy = 0.0;\n";
  shader += "    for(int i=0; i<2; i++)\n";
  shader += "      {\n";
  shader += "        dx += texture2DRect(depthTex, spos.xy+vec2(1+i,0.0)).x -\n";
  shader += "              texture2DRect(depthTex, spos.xy-vec2(1+i,0.0)).x;\n";
  shader += "        dy += texture2DRect(depthTex, spos.xy+vec2(0.0,1+i)).x -\n";
  shader += "              texture2DRect(depthTex, spos.xy-vec2(0.0,1+i)).x;\n";
  shader += "      }\n";
  shader += "    vec3 N = normalize(vec3(dx*100*(1.0+0.05*depth), dy*100*(1.0+0.05*depth), 0.25*(1.0-0.5*roughness)));\n";
  shader += "    float specCoeff = 512.0/mix(0.1, 64.0, roughness*roughness);\n";
  shader += "    shine = 0.5 * specular * vec3(pow(N.z, specCoeff));\n";
  //shader += "    color.rgb = pow(color.rgb,vec3(1.0)-0.25*shine);\n";
  shader += "  }\n";
  //----------------------
   
  shader += "    color.rgb = mix(color.rgb, color.rgb+shine, color.rgb);\n";
  //----------------------
  

  //----------------------
  // darken if required
  shader += "  if (darken < 1.0)\n";
  shader += "  {\n";
  shader += "    vec3 outputColor = rgb2hsv(color.rgb);\n";
  shader += "    outputColor.yz *= darken;\n";
  shader += "    color.rgb = hsv2rgb(outputColor.xyz);\n";
  shader += "  }\n";
  //----------------------

  shader += " color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));\n";

  shader += " color.rgb = pow(color.rgb, vec3(1.0/gamma));\n";


  shader += "}\n";

  return shader;
}



//QString
//ShaderFactory::outlineShaderF()
//{
//  QString shader;
//
//  shader += "#version 420 core\n";
// 
//  // Ouput data
//  shader += "layout(location=0) out vec4 color;\n";
//  shader += "layout(depth_greater) out float gl_FragDepth;\n";
//		      
//  shader += "uniform sampler2DRect colorTex;\n";
//  shader += "uniform sampler2DRect depthTex;\n";
//  shader += "uniform float gamma;\n";
//  shader += "uniform float roughness;\n";
//  shader += "uniform float specular;\n";
//  shader += "uniform float softshadow;\n";
//  shader += "uniform float edges;\n";
//
//
//  shader += rgb2hsv();
//  shader += hsv2rgb();
//  shader += ggxShader();
//  
//  shader += "void main()\n";
//  shader += "{\n";
//  
//  shader += "  vec2 spos = gl_FragCoord.xy;\n";
//
//  shader += "  color = texture2DRect(colorTex, spos.xy);\n";
//  shader += "  if (color.a < 0.001)\n";
//  shader += "    discard;\n";
//
//  
//  shader += "  vec4 dtex = texture2DRect(depthTex, spos.xy);\n";
//  shader += "  float depth = dtex.x;\n";
//
//  //------------------------
//  // dtex.x - shadow depth for original camera
//  // dtex.y - glow
//  // dtex.z - surface id
//  // dtex.w - fragDepth
//  //------------------------
//  
//  shader += "  if (depth < 0.001) \n";
//  shader += "  {\n";
//  shader += "     gl_FragDepth = 1.0;\n";
//  shader += "     return;\n";
//  shader += "  }\n";
//
//
//  shader += "  gl_FragDepth = dtex.w;\n";
//
//  
//  shader += "  float surfId = dtex.z;\n";
//  
//  shader += "  float response = 0.0;\n";
//
//
//  shader += "  float opout = float(mod(mod(color.a,10000.0),1000.0));\n";
//  shader += "  float outline = floor(mod(opout,11.0));\n";
//  shader += "  float opacity = 1.0-float(int(opout)/11)/10.0;\n";
//
//  shader += "  vec3 ecolor = color.rgb;\n";
//
//  shader += "  vec3 bcolor = color.rgb;\n";
////  shader += "  vec3 beColor = rgb2hsv(bcolor.rgb);\n";
////  shader += "  beColor.x += 0.5;\n";
////  shader += "  beColor.x = mix(beColor.x, 1.0-beColor.x, step(1.0, beColor.x));\n";
////  shader += "  beColor.yz = vec2(1.0);\n";
////  shader += "  beColor.rgb = hsv2rgb(beColor.xyz);\n";
////  shader += "  bcolor = mix(bcolor, beColor, step(19.0, softshadow));\n";
//
//  
//  // surface edges
//  shader += "    int nsteps = int(8.0*outline);\n";
//  shader += "    vec3 clrG = vec3(0.0);\n";
//  shader += "    float sumG = 0.0;\n";
//  shader += "    for(int i=0; i<nsteps; i++)\n";
//  shader += "      {\n";
//  shader += "        float r = 1.0 + i*0.13;\n";
//  shader += "        float x = r*sin(radians(i*43));\n";
//  shader += "        float y = r*cos(radians(i*43));\n";
//  shader += "        vec2 pos = spos + vec2(x,y);\n";
//  shader += "        vec3 adepth = texture2DRect(depthTex, pos).xyz;\n";
//  // different depths
//  shader += "        float od = depth - adepth.x;\n";
//  // border between different surfaces
//  shader += "        od += step(0.001, abs(surfId - adepth.z));\n"; 
//  shader += "        response += max(0.0, od);\n";
//
//  // get contributions for glowing surfaces
//  shader += "        float sr = step(0.05, adepth.y);\n";
//  shader += "        sumG += sr;\n";
//  shader += "        clrG += sr*texture2DRect(colorTex, pos).rgb;\n";
//
//  shader += "      } \n";
//  shader += "    response /= float(nsteps);\n";
// 
//  shader += "   float border = clamp(response,0.0,1.0);\n";
//  shader += "   border = exp(-border/gamma);\n";
//  // outline for selections on dark background
//  shader += "   float fadein = mix(border, 1.0, step(19.0, softshadow));\n";
//  shader += "   color = mix(vec4(bcolor.rgb*fadein,1.0), vec4(color.rgb, 1.0)*opacity, pow(border, 3.0));\n";
//  
//  // add glow
//  shader += "   clrG /= vec3(max(1.0,sumG));\n";
//  shader += "   float gl = pow(sumG/float(nsteps), gamma);\n";
//  shader += "   gl = smoothstep(0.0, 1.0, gl);\n";
//  shader += "   color.rgb = mix(color.rgb, clrG, gl);\n";
//
//  // add edges on surfaces
//  shader += "  if (edges > 0.0)\n";
//  shader += "  {\n"; 
//  shader += "    response = 0.0;\n";
//  shader += "    for(int i=0; i<8; i++)\n";
//  shader += "      {\n";
//  shader += "        float r = 1.0;\n";
//  shader += "        float x = r*sin(radians(i*45));\n";
//  shader += "        float y = r*cos(radians(i*45));\n";
//  shader += "        vec2 pos = spos + vec2(x,y);\n";
//  shader += "        float adepth = texture2DRect(depthTex, pos).x;\n";
//  shader += "        float od = depth - adepth;\n";
//  shader += "        response += max(0.0, od);\n";
//  shader += "      } \n";
//  shader += "    float eth = exp(-response*pow(40*(edges+0.05), 1.2*gamma));\n";
//  shader += "    ecolor.rgb *= eth;\n";
//  shader += "    color.rgb = mix(color.rgb, ecolor*color.rgb, pow(border, 3.0));\n";
//  shader += "  }\n";
//
//
//
//  //----------------------
//  // specular highlights
//  shader += "  if (roughness >= 0.0)\n";
//  shader += "  {\n";
//  shader += "    float dx = 0.0;\n";
//  shader += "    float dy = 0.0;\n";
//  shader += "    for(int i=0; i<2; i++)\n";
//  shader += "      {\n";
//  shader += "        dx += texture2DRect(depthTex, spos.xy+vec2(1+i,0.0)).x -\n";
//  shader += "              texture2DRect(depthTex, spos.xy-vec2(1+i,0.0)).x;\n";
//  shader += "        dy += texture2DRect(depthTex, spos.xy+vec2(0.0,1+i)).x -\n";
//  shader += "              texture2DRect(depthTex, spos.xy-vec2(0.0,1+i)).x;\n";
//  shader += "      }\n";
//  shader += "    vec3 N = normalize(vec3(dx*100, dy*100, 0.25*(1.0-0.5*roughness)));\n";
//  shader += "    float specCoeff = 512.0/mix(0.1, 64.0, roughness*roughness);\n";
//  shader += "    color.rgb += specular * vec3(pow(N.z, specCoeff));\n";
//  shader += "  }\n";
//  
//
//  shader += " color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));\n";
//  
//  
//  shader += "}\n";
//
//  return shader;
//}




//----------------------------
//----------------------------

//----------------------------
//----------------------------


GLint ShaderFactory::m_planeShadowShaderParm[10];
GLint* ShaderFactory::planeShadowShaderParm() { return &m_planeShadowShaderParm[0]; }

GLuint ShaderFactory::m_planeShadowShader = 0;
GLuint ShaderFactory::planeShadowShader()
{
  if (!m_planeShadowShader)
    {
      m_planeShadowShader = glCreateProgram();
      QString vertShaderString = planeShadowShaderV();
      QString fragShaderString = planeShadowShaderF();
  
      bool ok = loadShader(m_planeShadowShader,
			   vertShaderString,
			   fragShaderString);  

      if (!ok)
	{
	  QMessageBox::information(0, "", "Cannot load clip shadow shaders");
	  return 0;
	}
	
	m_planeShadowShaderParm[0] = glGetUniformLocation(m_planeShadowShader, "gamma");
	m_planeShadowShaderParm[1] = glGetUniformLocation(m_planeShadowShader, "depthTexS");
	m_planeShadowShaderParm[2] = glGetUniformLocation(m_planeShadowShader, "shadowIntensity");
	m_planeShadowShaderParm[3] = glGetUniformLocation(m_planeShadowShader, "shadowCam");
	m_planeShadowShaderParm[4] = glGetUniformLocation(m_planeShadowShader, "sceneRadius");
    	m_planeShadowShaderParm[5] = glGetUniformLocation(m_planeShadowShader, "MVPShadow");
    	m_planeShadowShaderParm[6] = glGetUniformLocation(m_planeShadowShader, "screenSize");
    	m_planeShadowShaderParm[7] = glGetUniformLocation(m_planeShadowShader, "lightDir");
    }

  return m_planeShadowShader;
}

QString
ShaderFactory::planeShadowShaderV()
{
  QString shader;
  shader += "#version 120\n";
  shader += "uniform mat4 MVPShadow;\n";
  shader += "uniform vec3 shadowCam;\n";
  shader += "uniform vec2 screenSize;\n";
  shader += "uniform float sceneRadius;\n";
  shader += "uniform vec3 lightDir;\n";

  shader += "varying float zdepthS;\n";
  shader += "varying vec4 vcolor;\n";
  shader += "varying vec3 shadowInfo;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "   gl_FrontColor = gl_Color;\n";
  shader += "   gl_BackColor = gl_Color;\n";
  shader += "   gl_Position = ftransform();\n";
  shader += "   vcolor = gl_Color;\n";
  shader += "   vec3 v = gl_Vertex.xyz;\n";
  //shader += "   zdepthS = length(v-shadowCam)/sceneRadius;\n";
  shader += "   zdepthS = dot(lightDir,(v-shadowCam))/sceneRadius;\n"; // ORTHOGRAPIC
  shader += "   vec4 shadowPos = MVPShadow * vec4(v, 1);\n";
  shader += "   vec2 xy = shadowPos.xy/vec2(shadowPos.w);\n";
  shader += "   xy = (xy + vec2(1.0)) * vec2(0.5);\n";
  shader += "   xy = xy * screenSize;\n";
  shader += "   shadowInfo = vec3(xy, zdepthS);\n";
  shader += "}\n";
  return shader;
}

QString
ShaderFactory::planeShadowShaderF()
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform float gamma;\n";
  shader += "uniform sampler2DRect depthTexS;\n";
  shader += "uniform float shadowIntensity;\n";

  shader += "in vec4 vcolor;\n";
  shader += "in float zdepthS;\n";
  shader += "in vec3 shadowInfo;\n";

  shader += "void main()\n";
  shader += "{\n";
  
  shader += "  vec2 spos = shadowInfo.xy;\n";
  shader += "  vec4 color = vcolor;\n";

  
  shader += "  int shadowSamples = 10;\n";
  
  // shadows
  shader += "  float sumS = 0.0;\n";
  shader += "  for(int i=0; i<shadowSamples; i++)\n";
  shader += "    {\n";  
  shader += "  	 float r = 1.0+i*0.2;\n";
  shader += "    float x = r*sin(radians(i*37.0));\n";
  shader += "    float y = r*cos(radians(i*37.0));\n";
  shader += "    vec2 pos = spos + vec2(x,y);\n";
  shader += "    float tdepth = texture2DRect(depthTexS, pos).x;\n";
  shader += "    float inShadow = 1.0-step(tdepth, 0.01);\n";
  shader += "    inShadow *= step(1.0, zdepthS);\n";
  shader += "  	 sumS += inShadow;\n";
  shader += "    }\n";  
  shader += "  sumS /= float(shadowSamples);\n";
  shader += "  float shadow = sumS;\n";
  shader += "  shadow = exp(-shadow*shadowIntensity);\n";
  shader += "  color.rgb *= shadow;\n";

  shader += "  color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));\n";

  shader += "  color.rgb = pow(color.rgb, vec3(1.0/gamma));\n";

  shader += "  gl_FragColor = color;\n";

  shader += "}\n";

  return shader;
}

//----------------------------
//----------------------------

GLint ShaderFactory::m_ptShaderParm[20];
GLint* ShaderFactory::ptShaderParm() { return &m_ptShaderParm[0]; }

GLuint ShaderFactory::m_ptShader = 0;
GLuint ShaderFactory::ptShader()
{
  if (!m_ptShader)
    {
      m_ptShader = glCreateProgram();
      QString vertShaderString = ptShaderV();
      QString fragShaderString = ptShaderF();
  
      bool ok = loadShader(m_ptShader,
			   vertShaderString,
			   fragShaderString);  

      if (!ok)
	{
	  QMessageBox::information(0, "", "Cannot load pt shaders");
	  return 0;
	}
	
	m_ptShaderParm[0] = glGetUniformLocation(m_ptShader, "MVP");
	m_ptShaderParm[1] = glGetUniformLocation(m_ptShader, "diffuseTex");
	m_ptShaderParm[2] = glGetUniformLocation(m_ptShader, "pn");
	m_ptShaderParm[3] = glGetUniformLocation(m_ptShader, "pnear");
	m_ptShaderParm[4] = glGetUniformLocation(m_ptShader, "pfar");
	m_ptShaderParm[5] = glGetUniformLocation(m_ptShader, "opacity");
    }

  return m_ptShader;
}

QString
ShaderFactory::ptShaderV() // position, normal, texture
{
  QString shader;

  shader += "#version 410\n";
  shader += "uniform mat4 MVP;\n";
  shader += "layout(location = 0) in vec3 position;\n";
  shader += "layout(location = 1) in vec2 texIn;\n";
  shader += "out vec2 v2Tex;\n";
  shader += "out vec3 pointPos;\n";
  shader += "void main()\n";
  shader += "{\n";
  shader += "   pointPos = position;\n";
  shader += "   v2Tex = texIn;\n";
  shader += "   gl_Position = MVP * vec4(position, 1);\n";
  shader += "}\n";

  return shader;
}
QString
ShaderFactory::ptShaderF()
{
  QString shader;

  shader += "#version 410 core\n";
  shader += "uniform sampler2D diffuseTex;\n";
  shader += "uniform vec3 pn;\n";
  shader += "uniform float pnear;\n";
  shader += "uniform float pfar;\n";
  shader += "uniform float opacity;\n";
  shader += "in vec3 pointPos;\n";
  shader += "\n";
  shader += "in vec2 v2Tex;\n";
  shader += "out vec4 outputColor;\n";
  shader += "void main()\n";
  shader += "{\n";

  shader += "   float d = dot(pn, pointPos);\n";
  shader += "   if (pnear < pfar && (d < pnear || d > pfar))\n";
  shader += "     discard;\n";

  shader += "  outputColor = texture(diffuseTex, v2Tex);\n";
  //shader += "  outputColor.rgb *= outputColor.a;\n";
  shader += "  outputColor *= opacity;\n";
  shader += "}\n";

  return shader;
}

//----------------------------
//----------------------------

GLint ShaderFactory::m_pnShaderParm[20];
GLint* ShaderFactory::pnShaderParm() { return &m_pnShaderParm[0]; }

GLuint ShaderFactory::m_pnShader = 0;
GLuint ShaderFactory::pnShader()
{
  if (!m_pnShader)
    {
      m_pnShader = glCreateProgram();
      QString vertShaderString = pnShaderV();
      QString fragShaderString = pnShaderF();
  
      bool ok = loadShader(m_pnShader,
			   vertShaderString,
			   fragShaderString);  

      if (!ok)
	{
	  QMessageBox::information(0, "", "Cannot load pt shaders");
	  return 0;
	}
	
	m_pnShaderParm[0] = glGetUniformLocation(m_pnShader, "MVP");
	m_pnShaderParm[1] = glGetUniformLocation(m_pnShader, "pn");
	m_pnShaderParm[2] = glGetUniformLocation(m_pnShader, "pnear");
	m_pnShaderParm[3] = glGetUniformLocation(m_pnShader, "pfar");
	m_pnShaderParm[4] = glGetUniformLocation(m_pnShader, "opacity");
	m_pnShaderParm[5] = glGetUniformLocation(m_pnShader, "color");
	m_pnShaderParm[6] = glGetUniformLocation(m_pnShader, "lightvec");
    }

  return m_pnShader;
}

QString
ShaderFactory::pnShaderV() // position, normal, texture
{
  QString shader;

  shader += "#version 410\n";
  shader += "uniform mat4 MVP;\n";
  shader += "layout(location = 0) in vec3 position;\n";
  shader += "layout(location = 1) in vec3 normalIn;\n";
  shader += "out vec3 normal;\n";
  shader += "out vec3 pointPos;\n";
  shader += "void main()\n";
  shader += "{\n";
  shader += "   pointPos = position;\n";
  shader += "   normal = normalIn;\n";
  shader += "   gl_Position = MVP * vec4(position, 1);\n";
  shader += "}\n";

  return shader;
}
QString
ShaderFactory::pnShaderF()
{
  QString shader;

  shader += "#version 410 core\n";
  shader += "uniform vec3 pn;\n";
  shader += "uniform float pnear;\n";
  shader += "uniform float pfar;\n";
  shader += "uniform float opacity;\n";
  shader += "uniform vec3 color;\n";
  shader += "uniform vec3 lightvec;\n";
  shader += "in vec3 normal;\n";
  shader += "in vec3 pointPos;\n";
  shader += "\n";
  shader += "out vec4 outputColor;\n";
  shader += "void main()\n";
  shader += "{\n";

  shader += "   float d = dot(pn, pointPos);\n";
  shader += "   if (pnear < pfar && (d < pnear || d > pfar))\n";
  shader += "     discard;\n";

  shader += "  outputColor = vec4(color,1.0);\n";
  shader += "  outputColor *= opacity;\n";

  shader += "  vec3 reflecvec = reflect(lightvec, normal);\n";
  shader += "  float DiffMag = abs(dot(normal, lightvec));\n";
  shader += "  vec3 Diff = (0.8*DiffMag)*outputColor.rgb;\n";
  shader += "  float Spec = pow(abs(dot(normal, reflecvec)), 32.0);\n";
  shader += "  Spec *= 1.0*outputColor.a;\n";
  shader += "  vec3 Amb = 0.2*outputColor.rgb;\n";
  shader += "  outputColor.rgb = Amb + Diff + Spec;\n";
  shader += "  outputColor = clamp(outputColor, vec4(0.0), vec4(1.0));\n";

  shader += "}\n";

  return shader;
}


//----------------------------
//----------------------------

QString
ShaderFactory::rgb2hsv()
{
  QString shader;
  shader += "vec3 rgb2hsv(vec3 c)\n";
  shader += "{\n";
  shader += "    vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);\n";
  shader += "    vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));\n";
  shader += "    vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));\n";
  shader += "\n";
  shader += "    float d = q.x - min(q.w, q.y);\n";
  shader += "    float e = 1.0e-10;\n";
  shader += "    return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);\n";
  shader += "}\n";
  return shader;
}

QString
ShaderFactory::hsv2rgb()
{
  QString shader;
  shader += "vec3 hsv2rgb(vec3 c)\n";
  shader += "{\n";
  shader += "    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);\n";
  shader += "    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);\n";
  shader += "    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);\n";
  shader += "}\n";
  return shader;
}

QString
ShaderFactory::noise2d()
{
  QString shader;

  shader += "float random (in vec2 st) {\n";
  shader += "    return fract(sin(dot(st.xy,\n";
  shader += "                         vec2(12.9898,78.233)))*\n";
  shader += "        43758.5453123);\n";
  shader += "}\n";
  shader += "\n";
  shader += "// Based on Morgan McGuire @morgan3d\n";
  shader += "// https://www.shadertoy.com/view/4dS3Wd\n";
  shader += "float noise (in vec2 st) {\n";
  shader += "    vec2 i = floor(st);\n";
  shader += "    vec2 f = fract(st);\n";
  shader += "\n";
  shader += "    // Four corners in 2D of a tile\n";
  shader += "    float a = random(i);\n";
  shader += "    float b = random(i + vec2(1.0, 0.0));\n";
  shader += "    float c = random(i + vec2(0.0, 1.0));\n";
  shader += "    float d = random(i + vec2(1.0, 1.0));\n";
  shader += "\n";
  shader += "    vec2 u = f * f * (3.0 - 2.0 * f);\n";
  shader += "\n";
  shader += "    return mix(a, b, u.x) +\n";
  shader += "            (c - a)* u.y * (1.0 - u.x) +\n";
  shader += "            (d - b) * u.x * u.y;\n";
  shader += "}\n";
  shader += "\n";
  shader += "#define OCTAVES 6\n";
  shader += "float fbm (in vec2 st) {\n";
  shader += "    // Initial values\n";
  shader += "    float value = 0.0;\n";
  shader += "    float amplitude = .5;\n";
  shader += "    float frequency = 0.;\n";
  shader += "    //\n";
  shader += "    // Loop of octaves\n";
  shader += "    for (int i = 0; i < OCTAVES; i++) {\n";
  shader += "        value += amplitude * noise(st);\n";
  shader += "        st *= 2.;\n";
  shader += "        amplitude *= .5;\n";
  shader += "    }\n";
  shader += "    return value;\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::noise3d()
{
  QString shader;

  shader += "/* discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3 */\n";
  shader += "vec3 random3(vec3 c) {\n";
  shader += "	float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));\n";
  shader += "	vec3 r;\n";
  shader += "	r.z = fract(512.0*j);\n";
  shader += "	j *= .125;\n";
  shader += "	r.x = fract(512.0*j);\n";
  shader += "	j *= .125;\n";
  shader += "	r.y = fract(512.0*j);\n";
  shader += "	return r-0.5;\n";
  shader += "}\n";
  shader += "\n";
  shader += "/* skew constants for 3d simplex functions */\n";
  shader += "const float F3 =  0.3333333;\n";
  shader += "const float G3 =  0.1666667;\n";
  shader += "\n";
  shader += "/* 3d simplex noise */\n";
  shader += "float simplex3d(vec3 p) {\n";
  shader += "	 /* 1. find current tetrahedron T and it's four vertices */\n";
  shader += "	 /* s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices */\n";
  shader += "	 /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/\n";
  shader += "	 \n";
  shader += "	 /* calculate s and x */\n";
  shader += "	 vec3 s = floor(p + dot(p, vec3(F3)));\n";
  shader += "	 vec3 x = p - s + dot(s, vec3(G3));\n";
  shader += "	 \n";
  shader += "	 /* calculate i1 and i2 */\n";
  shader += "	 vec3 e = step(vec3(0.0), x - x.yzx);\n";
  shader += "	 vec3 i1 = e*(1.0 - e.zxy);\n";
  shader += "	 vec3 i2 = 1.0 - e.zxy*(1.0 - e);\n";
  shader += "	 	\n";
  shader += "	 /* x1, x2, x3 */\n";
  shader += "	 vec3 x1 = x - i1 + G3;\n";
  shader += "	 vec3 x2 = x - i2 + 2.0*G3;\n";
  shader += "	 vec3 x3 = x - 1.0 + 3.0*G3;\n";
  shader += "	 \n";
  shader += "	 /* 2. find four surflets and store them in d */\n";
  shader += "	 vec4 w, d;\n";
  shader += "	 \n";
  shader += "	 /* calculate surflet weights */\n";
  shader += "	 w.x = dot(x, x);\n";
  shader += "	 w.y = dot(x1, x1);\n";
  shader += "	 w.z = dot(x2, x2);\n";
  shader += "	 w.w = dot(x3, x3);\n";
  shader += "	 \n";
  shader += "	 /* w fades from 0.6 at the center of the surflet to 0.0 at the margin */\n";
  shader += "	 w = max(0.6 - w, 0.0);\n";
  shader += "	 \n";
  shader += "	 /* calculate surflet components */\n";
  shader += "	 d.x = dot(random3(s), x);\n";
  shader += "	 d.y = dot(random3(s + i1), x1);\n";
  shader += "	 d.z = dot(random3(s + i2), x2);\n";
  shader += "	 d.w = dot(random3(s + 1.0), x3);\n";
  shader += "	 \n";
  shader += "	 /* multiply d by w^4 */\n";
  shader += "	 w *= w;\n";
  shader += "	 w *= w;\n";
  shader += "	 d *= w;\n";
  shader += "	 \n";
  shader += "	 /* 3. return the sum of the four surflets */\n";
  shader += "	 return dot(d, vec4(52.0));\n";
  shader += "}\n";
  shader += "\n";
  shader += "/* const matrices for 3d rotation */\n";
  shader += "const mat3 rot1 = mat3(-0.37, 0.36, 0.85,-0.14,-0.93, 0.34,0.92, 0.01,0.4);\n";
  shader += "const mat3 rot2 = mat3(-0.55,-0.39, 0.74, 0.33,-0.91,-0.24,0.77, 0.12,0.63);\n";
  shader += "const mat3 rot3 = mat3(-0.71, 0.52,-0.47,-0.08,-0.72,-0.68,-0.7,-0.45,0.56);\n";
  shader += "\n";
  shader += "/* directional artifacts can be reduced by rotating each octave */\n";
  shader += "float simplex3d_fractal(vec3 m) {\n";
  shader += "    return   0.5333333*simplex3d(m*rot1)\n";
  shader += "			+0.2666667*simplex3d(2.0*m*rot2)\n";
  shader += "			+0.1333333*simplex3d(4.0*m*rot3)\n";
  shader += "			+0.0666667*simplex3d(8.0*m);\n";
  shader += "}\n";

  return shader;
}


bool
ShaderFactory::addShader(GLuint shaderProg,
			 GLenum shaderType,
			 QString shaderString)
{
  GLuint shaderObj = glCreateShader(shaderType);  

  if (shaderObj == 0) {
    QMessageBox::information(0, "Error", QString("Error creating shader type %1").\
			     arg(shaderType));
    return false;
  }

  int len = shaderString.length();
  char *tbuffer = new char[len+1];
  sprintf(tbuffer, shaderString.toLatin1().data());
  const char *sstr = tbuffer;
  glShaderSource(shaderObj, 1, &sstr, NULL);
  delete [] tbuffer;

  glCompileShader(shaderObj);

  GLint success;
  glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);

  if (!success)
    {
	GLcharARB str[1024];
	GLsizei len;
	glGetInfoLogARB(shaderObj,
			(GLsizei) 1024,
			&len,
			str);
	
	QString estr;
	QStringList slist = shaderString.split("\n");
	for(int i=0; i<slist.count(); i++)
	  estr += QString("%1 : %2\n").arg(i+1).arg(slist[i]);
	
	QTextEdit *tedit = new QTextEdit();
	tedit->insertPlainText("-------------Error----------------\n\n");
	tedit->insertPlainText(str);
	tedit->insertPlainText("\n-----------Shader---------------\n\n");
	tedit->insertPlainText(estr);
	
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(tedit);
	
	QDialog *showError = new QDialog();
	showError->setWindowTitle("Error in Shader");
	showError->setSizeGripEnabled(true);
	showError->setModal(true);
	showError->setLayout(layout);
	showError->exec();
	return false;
    }

  glAttachShader(shaderProg, shaderObj);

  m_shaderList << shaderObj;
  
  return true;
}

bool
ShaderFactory::finalize(GLuint shaderProg)
{
  GLint linked = -1;

  glLinkProgram(shaderProg);

  glGetProgramiv(shaderProg, GL_LINK_STATUS, &linked);

  if (!linked)
    {
      GLcharARB str[1024];
      GLsizei len;
      QMessageBox::information(0,
			       "ProgObj",
			       "error linking texProgObj");
      glGetInfoLogARB(shaderProg,
		      (GLsizei) 1024,
		      &len,
		      str);
      QMessageBox::information(0,
			       "Error",
			       QString("%1\n%2").arg(len).arg(str));
      return false;
    }


  for(int i=0; i<m_shaderList.count(); i++)
    glDeleteShader(m_shaderList[i]);

  m_shaderList.clear();

  return true;
}


bool
ShaderFactory::loadShaderFromFile(GLuint obj, QString filename)
{
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;

  QByteArray lines = file.readAll();

  int len = lines.count(); 
  char *source = new char[len+1];
  sprintf(source, lines.constData());
 
  const char *sstr = source;
  glShaderSourceARB(obj, 1, &sstr, NULL);

  delete [] source;

  return true;
}


bool
ShaderFactory::loadShader(GLuint &shaderProg,
			  QString vertShaderString,
			  QString fragShaderString)
{
  if (!addShader(shaderProg,
		 GL_VERTEX_SHADER,
		 vertShaderString))
    return false;

  if (!addShader(shaderProg,
		 GL_FRAGMENT_SHADER,
		 fragShaderString))
    return false;

  return finalize(shaderProg);
}

bool
ShaderFactory::loadShadersFromFile(GLuint &progObj,
				   QString vertShader,
				   QString fragShader)
{
  QFile vfile(vertShader);
  if (!vfile.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;
  QString vertShaderString = QString::fromLatin1(vfile.readAll());


  QFile ffile(fragShader);
  if (!ffile.open(QIODevice::ReadOnly | QIODevice::Text))
    return false;
  QString fragShaderString = QString::fromLatin1(ffile.readAll());
  
  return loadShader(progObj,
		    vertShaderString,
		    fragShaderString);  
}
//-----------------


//---------------
GLuint ShaderFactory::m_rcShader = 0;
GLuint ShaderFactory::rcShader()
{
  if (!m_rcShader)
    createTextureShader();

  return m_rcShader;
}

GLint ShaderFactory::m_rcShaderParm[10];
GLint* ShaderFactory::rcShaderParm() { return &m_rcShaderParm[0]; }

void
ShaderFactory::createTextureShader()
{
  m_rcShader = glCreateProgram();
  if (!ShaderFactory::loadShadersFromFile(m_rcShader,
					  qApp->applicationDirPath() + QDir::separator() + "assets/shaders/controllershader.vert",
					  qApp->applicationDirPath() + QDir::separator() + "assets/shaders/controllershader.frag"))
    {
      QMessageBox::information(0, "", "Cannot load controller shaders");
    }

  m_rcShaderParm[0] = glGetUniformLocation(m_rcShader, "MVP");
  m_rcShaderParm[1] = glGetUniformLocation(m_rcShader, "diffuse");
  m_rcShaderParm[2] = glGetUniformLocation(m_rcShader, "color");
  m_rcShaderParm[3] = glGetUniformLocation(m_rcShader, "viewDir");
  m_rcShaderParm[4] = glGetUniformLocation(m_rcShader, "opmod");
  m_rcShaderParm[5] = glGetUniformLocation(m_rcShader, "applytexture");
  m_rcShaderParm[6] = glGetUniformLocation(m_rcShader, "ptsz");
  m_rcShaderParm[7] = glGetUniformLocation(m_rcShader, "mixcolor");
}
//---------------


//---------------
GLuint ShaderFactory::m_cubemapShader = 0;
GLuint ShaderFactory::cubemapShader()
{
  if (!m_cubemapShader)
    createCubeMapShader();
  
  return m_cubemapShader;
}

GLint ShaderFactory::m_cubemapShaderParm[10];
GLint* ShaderFactory::cubemapShaderParm() { return &m_cubemapShaderParm[0]; }

void
ShaderFactory::createCubeMapShader()
{
  m_cubemapShader = glCreateProgram();
  if (!ShaderFactory::loadShadersFromFile(m_cubemapShader,
					  qApp->applicationDirPath() + QDir::separator() + "assets/shaders/cubemap.vert",
					  qApp->applicationDirPath() + QDir::separator() + "assets/shaders/cubemap.frag"))
    {
      QMessageBox::information(0, "", "Cannot load cubemap shaders");
    }

  m_cubemapShaderParm[0] = glGetUniformLocation(m_cubemapShader, "MVP");
  m_cubemapShaderParm[1] = glGetUniformLocation(m_cubemapShader, "hmdPos");
  m_cubemapShaderParm[2] = glGetUniformLocation(m_cubemapShader, "scale");
  m_cubemapShaderParm[3] = glGetUniformLocation(m_cubemapShader, "skybox");
}
//---------------


//---------------
GLint ShaderFactory::m_copyShaderParm[10];
GLint* ShaderFactory::copyShaderParm() { return &m_copyShaderParm[0]; }

GLuint ShaderFactory::m_copyShader = 0;
GLuint ShaderFactory::copyShader()
{
  if (!m_copyShader)
    {
      QString shaderString;
      shaderString = genCopyShaderString();
      m_copyShader = glCreateProgramObjectARB();
      if (! loadShader(m_copyShader, shaderString))
	exit(0);
      m_copyShaderParm[0] = glGetUniformLocationARB(m_copyShader, "shadowTex");
    }
  
  return m_copyShader;
}
//---------------


//---------------
GLint ShaderFactory::m_dilateShaderParm[10];
GLint* ShaderFactory::dilateShaderParm() { return &m_dilateShaderParm[0]; }

GLuint ShaderFactory::m_dilateShader = 0;
GLuint ShaderFactory::dilateShader()
{
  if (!m_dilateShader)
    {
      QString shaderString;
      shaderString = genDilateShaderString();
      m_dilateShader = glCreateProgramObjectARB();
      if (! loadShader(m_dilateShader, shaderString))
	exit(0);
      m_dilateShaderParm[0] = glGetUniformLocationARB(m_dilateShader, "tex");
      m_dilateShaderParm[1] = glGetUniformLocationARB(m_dilateShader, "radius");
      m_dilateShaderParm[2] = glGetUniformLocationARB(m_dilateShader, "direc");
    }
  
  return m_dilateShader;
}
//---------------


//---------------
GLint ShaderFactory::m_blurShaderParm[10];
GLint* ShaderFactory::blurShaderParm() { return &m_blurShaderParm[0]; }

GLuint ShaderFactory::m_blurShader = 0;
GLuint ShaderFactory::blurShader()
{
  if (!m_blurShader)
    {
      QString shaderString;
      shaderString = genSmoothDilatedShaderString();
      m_blurShader = glCreateProgramObjectARB();
      if (! loadShader(m_blurShader, shaderString))
	exit(0);
      m_blurShaderParm[0] = glGetUniformLocationARB(m_blurShader, "blurTex");
    }
  
  return m_blurShader;
}
//---------------
