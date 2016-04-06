#include "geoshaderfactory.h"
#include "cropshaderfactory.h"
#include "tearshaderfactory.h"
#include "global.h"

#include <QMessageBox>

bool
GeoShaderFactory::loadShader(GLhandleARB &progObj,
			     QString shaderString)
{
  GLhandleARB fragObj = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);  
  glAttachObjectARB(progObj, fragObj);

  GLhandleARB vertObj = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);  
  glAttachObjectARB(progObj, vertObj);

  { // vertObj
    QString qstr;
    qstr += "#version 130\n";
    qstr += "varying vec3 pointPosition;\n";
    qstr += "varying vec3 normal;\n";
    qstr += "out float gl_ClipDistance[2];\n";
    qstr += "uniform vec4 ClipPlane0;\n";
    qstr += "uniform vec4 ClipPlane1;\n";
    qstr += "void main(void)\n";
    qstr += "{\n";
    qstr += "  // Transform vertex position into homogenous clip-space.\n";
    qstr += "  gl_FrontColor = gl_Color;\n";
    qstr += "  gl_BackColor = gl_Color;\n";
    qstr += "  gl_Position = ftransform();\n";
    qstr += "  pointPosition = gl_Vertex.xyz;\n";
    qstr += "  normal = gl_NormalMatrix * gl_Normal;\n";
    qstr += "  gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n";
    qstr += "  gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;\n";
    qstr += "  gl_TexCoord[2] = gl_TextureMatrix[2] * gl_MultiTexCoord2;\n";
    qstr += "  gl_ClipDistance[0] = dot(gl_Vertex, ClipPlane0);\n";
    qstr += "  gl_ClipDistance[1] = dot(gl_Vertex, ClipPlane1);\n";
    qstr += "}\n";

    int len = qstr.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, qstr.toLatin1().data());
    const char *sstr = tbuffer;
    glShaderSourceARB(vertObj, 1, &sstr, NULL);
    delete [] tbuffer;

    GLint compiled;
    glCompileShaderARB(vertObj);
    glGetObjectParameterivARB(vertObj,
			      GL_OBJECT_COMPILE_STATUS_ARB,
			      &compiled);
    if (!compiled)
      {
	GLcharARB str[1000];
	GLsizei len;
	glGetInfoLogARB(vertObj,
			(GLsizei) 1000,
			&len,
			str);

	QMessageBox::information(0,
				 "Error : Vertex Shader",
				 str);
	return false;
      }
  }
    
  { // fragObj
    int len = shaderString.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, shaderString.toLatin1().data());
    const char *sstr = tbuffer;
    glShaderSourceARB(fragObj, 1, &sstr, NULL);
    delete [] tbuffer;
  
    GLint compiled;
    glCompileShaderARB(fragObj);
    glGetObjectParameterivARB(fragObj,
			      GL_OBJECT_COMPILE_STATUS_ARB,
			      &compiled);
    if (!compiled)
      {
	GLcharARB str[1000];
	GLsizei len;
	glGetInfoLogARB(fragObj,
			(GLsizei) 1000,
			&len,
			str);
	
	QMessageBox::information(0,
				 "Error : Fragment Shader",
				 str);
	return false;
      }
  }

  
  //----------- link program shader ----------------------
  GLint linked;
  glLinkProgramARB(progObj);
  glGetObjectParameterivARB(progObj, GL_OBJECT_LINK_STATUS_ARB, &linked);
  if (!linked)
    {
      GLcharARB str[1000];
      GLsizei len;
      QMessageBox::information(0,
			       "ProgObj",
			       "error linking texProgObj");
      glGetInfoLogARB(progObj,
		      (GLsizei) 1000,
		      &len,
		      str);
      QMessageBox::information(0,
			       "Error",
			       QString("%1\n%2").arg(len).arg(str));
      return false;
    }

  glDeleteObjectARB(fragObj);
  glDeleteObjectARB(vertObj);

  return true;
}

QString
GeoShaderFactory::genDefaultShaderString(QList<CropObject> crops)
{
  bool cropPresent = false;
  bool tearPresent = false;
  bool viewPresent = false;
  bool glowPresent = false;
  for(int i=0; i<crops.count(); i++)
    if (crops[i].cropType() < CropObject::Tear_Tear)
      cropPresent = true;
    else if (crops[i].cropType() < CropObject::View_Tear)
      tearPresent = true;
    else if (crops[i].cropType() < CropObject::Glow_Ball)
      viewPresent = true;
    else
      glowPresent = true;

  QString shader;

  shader = "varying vec3 pointPosition;\n";
  shader += "varying vec3 normal;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform bool screenDoor;\n";
  shader += "uniform vec3 cropBorder;\n";
  shader += "uniform int nclip;\n";
  shader += "uniform vec3 clipPos[10];\n";
  shader += "uniform vec3 clipNormal[10];\n";

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "vec3 pointpos = pointPosition;\n";
  shader += "float cfeather = 1.0;\n";
  shader += "if (nclip > 0)\n";
  shader += "  {\n";
  shader += "    for(int c=0; c<nclip; c++)\n";
  shader += "      {\n";
  shader += "        vec3 cpos = clipPos[c];\n";
  shader += "        vec3 cnorm = clipNormal[c];\n";
  shader += "        float cp = dot(pointpos-cpos, cnorm);\n";
  shader += "        if (cp > 0.0)\n";
  shader += "          discard;\n";
  shader += "        else\n";
  shader += "          cfeather *= smoothstep(0.0, 3.0, -cp);\n";
  shader += "      }\n";
  shader += "    cfeather = 1.0 - cfeather;\n";
  shader += "  }\n";

  if (crops.count() > 0)
    {
      shader += " vec3 texCoord = pointpos;\n";
      shader += "  vec3 otexCoord = texCoord;\n";
      shader += "  float feather = 1.0;\n";
    }

  if (tearPresent)
    {
      shader += "vec4 tcf = dissect(texCoord);\n";
      shader += "texCoord = tcf.xyz;\n";
      shader += "feather *= tcf.w;\n";
      shader += " pointpos = texCoord;\n";
    }
  if (cropPresent) shader += "feather *= crop(texCoord, true);\n";


  shader += "  vec3 VP = pointpos - gl_LightSource[0].position.xyz;\n";
  shader += "  VP = normalize(VP);\n";
  shader += "  vec3 pnormal = normalize(normal);\n";

  shader += "  vec3 eye = pointpos - eyepos;\n";
  shader += "  eye = normalize(eye);\n";

  shader += "  float eyenorm = dot(VP, pnormal);\n";
  shader += "  if (screenDoor && eyenorm < 0.0)\n";
  shader += "   {\n";
  shader += "     if (mod(gl_FragCoord.x, 2.0) == mod(gl_FragCoord.y, 2.0)) ";
  shader += "       discard;\n";
  shader += "   }\n";

  shader += "  float DiffMag = abs(dot(pnormal, VP));\n";
  shader += "  vec3 halfVector = normalize(VP + eye);\n";
  shader += "  float nDotHV = abs(dot(pnormal, halfVector));\n";
  shader += "  float pf;\n";
  shader += "  if (DiffMag == 0.0) pf = 0.0;\n";
  shader += "    else pf = pow(nDotHV, gl_FrontMaterial.shininess);\n";
  shader += "  vec4 ambient = gl_LightSource[0].ambient;\n";
  shader += "  vec4 diffuse = gl_LightSource[0].diffuse * DiffMag;\n";
  shader += "  vec4 specular = gl_LightSource[0].specular * pf;\n";
  shader += "  gl_FragColor = gl_FrontLightModelProduct.sceneColor +\n";
  shader += "                 ambient*gl_Color +\n";
  shader += "                 diffuse*gl_Color +\n";
  shader += "                 specular*gl_FrontMaterial.specular;\n";
  shader += "  gl_FragColor.a = gl_Color.a;\n";

  
  shader += "  if (nclip > 0)\n";
  shader += "  {\n";
  shader += "    gl_FragColor = mix(gl_FragColor, vec4(cropBorder,0.9), cfeather);\n";
  
//  shader += "  if (cfeather < 0.5)\n"; // cap clipped region
//  shader += "    {\n";
//  shader += "      if (eyenorm > 0.0)\n";
//  shader += "        gl_FragColor = mix(gl_FragColor, vec4(cropBorder,0.9), 0.6);\n";
//  shader += "    }\n";

  shader += "  }\n";

  if (cropPresent || tearPresent)
      shader += "  gl_FragColor = mix(gl_FragColor, vec4(cropBorder,0.9), feather);\n";

  shader += "}\n";

  return shader;
}

QString
GeoShaderFactory::genHighQualityShaderString(bool shadows,
					     float shadowintensity,
					     QList<CropObject> crops)
{
  bool cropPresent = false;
  bool tearPresent = false;
  bool viewPresent = false;
  bool glowPresent = false;
  for(int i=0; i<crops.count(); i++)
    if (crops[i].cropType() < CropObject::Tear_Tear)
      cropPresent = true;
    else if (crops[i].cropType() < CropObject::View_Tear)
      tearPresent = true;
    else if (crops[i].cropType() < CropObject::Glow_Ball)
      viewPresent = true;
    else
      glowPresent = true;

  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointPosition;\n";
  shader += "varying vec3 normal;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform sampler2DRect shadowTex;\n";
  shader += "uniform bool screenDoor;\n";
  shader += "uniform vec3 cropBorder;\n";
  shader += "uniform int nclip;\n";
  shader += "uniform vec3 clipPos[10];\n";
  shader += "uniform vec3 clipNormal[10];\n";

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "vec3 pointpos = pointPosition;\n";
  shader += "float cfeather = 1.0;\n";
  shader += "if (nclip > 0)\n";
  shader += "  {\n";
  shader += "    for(int c=0; c<nclip; c++)\n";
  shader += "      {\n";
  shader += "        vec3 cpos = clipPos[c];\n";
  shader += "        vec3 cnorm = clipNormal[c];\n";
  shader += "        float cp = dot(pointpos-cpos, cnorm);\n";
  shader += "        if (cp > 0.0)\n";
  shader += "          discard;\n";
  shader += "        else\n";
  shader += "          cfeather *= smoothstep(0.0, 3.0, -cp);\n";
  shader += "      }\n";
  shader += "    cfeather = 1.0 - cfeather;\n";
  shader += "  }\n";

  if (crops.count() > 0)
    {
      shader += " vec3 texCoord = pointpos;\n";
      shader += "  vec3 otexCoord = texCoord;\n";
      shader += "  float feather = 1.0;\n";
    }

  if (tearPresent)
    {
      shader += "vec4 tcf = dissect(texCoord);\n";
      shader += "texCoord = tcf.xyz;\n";
      shader += "feather *= tcf.w;\n";
      shader += " pointpos = texCoord;\n";
    }
  if (cropPresent) shader += "feather *= crop(texCoord, true);\n";


  shader += "  vec3 VP = pointpos - gl_LightSource[0].position.xyz;\n";
  shader += "  VP = normalize(VP);\n";
  shader += "  vec3 pnormal = normalize(normal);\n";

  shader += "  vec3 eye = pointpos - eyepos;\n";
  shader += "  eye = normalize(eye);\n";

  shader += "  float eyenorm = dot(VP, pnormal);\n";
  shader += "  if (screenDoor && eyenorm < 0.2)\n";
  shader += "   {\n";
  shader += "     if (mod(gl_FragCoord.x, 2.0) == mod(gl_FragCoord.y, 2.0)) ";
  shader += "       discard;\n";
  shader += "   }\n";

  shader += "  float DiffMag = abs(dot(pnormal, VP));\n";
  shader += "  vec3 halfVector = normalize(VP + eye);\n";
  shader += "  float nDotHV = abs(dot(pnormal, halfVector));\n";
  shader += "  float pf;\n";
  shader += "  if (DiffMag == 0.0) pf = 0.0;\n";
  shader += "    else pf = pow(nDotHV, gl_FrontMaterial.shininess);\n";
  shader += "  vec4 ambient = gl_LightSource[0].ambient;\n";
  shader += "  vec4 diffuse = gl_LightSource[0].diffuse * DiffMag;\n";
  shader += "  vec4 specular = gl_LightSource[0].specular * pf;\n";
  shader += "  gl_FragColor = gl_FrontLightModelProduct.sceneColor +\n";
  shader += "                 ambient*gl_Color +\n";
  shader += "                 diffuse*gl_Color +\n";
  shader += "                 specular*gl_FrontMaterial.specular;\n";
  shader += "  gl_FragColor.a = gl_Color.a;\n";

  if (shadows)
    {
      //----------------------
      // shadows
      shader += "  vec4 incident_light = texture2DRect(shadowTex, gl_TexCoord[0].xy);\n";
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

  
  shader += "  if (nclip > 0)\n";
  shader += "  {\n";
  shader += "    gl_FragColor = mix(gl_FragColor, vec4(cropBorder,0.9), cfeather);\n";
  shader += "  }\n";

  if (cropPresent || tearPresent)
    shader += "  gl_FragColor = mix(gl_FragColor, vec4(cropBorder,0.9), feather);\n";

  shader += "}\n";

  return shader;
}

QString
GeoShaderFactory::genShadowShaderString(float r, float g, float b,
					QList<CropObject> crops)
{
  bool cropPresent = false;
  bool tearPresent = false;
  bool viewPresent = false;
  bool glowPresent = false;
  for(int i=0; i<crops.count(); i++)
    if (crops[i].cropType() < CropObject::Tear_Tear)
      cropPresent = true;
    else if (crops[i].cropType() < CropObject::View_Tear)
      tearPresent = true;
    else if (crops[i].cropType() < CropObject::Glow_Ball)
      viewPresent = true;
    else
      glowPresent = true;

  QString shader;
  float maxrgb = qMax(r, qMax(g, b));
  if (maxrgb > 1)
    maxrgb = 1.0/maxrgb;
  else
    maxrgb = 1.0;

  shader = "varying vec3 pointPosition;\n";
  shader += "varying vec3 normal;\n";
  shader += "uniform int nclip;\n";
  shader += "uniform vec3 clipPos[10];\n";
  shader += "uniform vec3 clipNormal[10];\n";

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "vec3 pointpos = pointPosition;\n";
  shader += "float cfeather = 1.0;\n";
  shader += "if (nclip > 0)\n";
  shader += "  {\n";
  shader += "    for(int c=0; c<nclip; c++)\n";
  shader += "      {\n";
  shader += "        vec3 cpos = clipPos[c];\n";
  shader += "        vec3 cnorm = clipNormal[c];\n";
  shader += "        float cp = dot(pointpos-cpos, cnorm);\n";
  shader += "        if (cp > 0.0)\n";
  shader += "          discard;\n";
  shader += "        else\n";
  shader += "          cfeather *= smoothstep(0.0, 3.0, -cp);\n";
  shader += "      }\n";
  shader += "    cfeather = 1.0 - cfeather;\n";
  shader += "  }\n";

  if (crops.count() > 0)
    {
      shader += "  vec3 texCoord = pointpos;\n";
      shader += "  vec3 otexCoord = texCoord;\n";
      shader += "  float feather = 1.0;\n";
    }

  if (tearPresent)
    {
      shader += "vec4 tcf = dissect(texCoord);\n";
      shader += "texCoord = tcf.xyz;\n";
      shader += "feather *= tcf.w;\n";
      shader += "if (feather > 0.9) discard;\n";
      shader += " pointpos = texCoord;\n";
    }
  if (cropPresent)
    {
      shader += "feather *= crop(texCoord, true);\n";
      shader += "  if (feather > 0.9) discard;\n";
    }

  shader += "  gl_FragColor = gl_Color;\n";

  shader += QString("  gl_FragColor.rgba *= vec4(%1, %2, %3, %4);\n").\
                                        arg(r).arg(g).arg(b).arg(maxrgb);

  
  shader += "  if (nclip > 0)\n";
  shader += "  {\n";
  shader += "  gl_FragColor = mix(gl_FragColor, vec4(0.0,0.0,0.0,0.0), cfeather);\n";
  shader += "  }\n";

  if (cropPresent)
    shader += "  gl_FragColor = mix(gl_FragColor, vec4(0.0,0.0,0.0,0.0), feather);\n";

  shader += "}\n";

  return shader;
}

QString
GeoShaderFactory::genSpriteShaderString()
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointPosition;\n";
  shader += "varying vec3 normal;\n";

  shader += "uniform sampler2D spriteTex;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  gl_FragColor = texture2D(spriteTex, gl_TexCoord[0].xy);\n";
  shader += "  gl_FragColor.rgba *= gl_Color.rgba;\n";
  shader += "}\n";
  
  return shader;
}

QString
GeoShaderFactory::genSpriteShadowShaderString(float r, float g, float b)
{
  QString shader;
  float maxrgb = qMax(r, qMax(g, b));
  if (maxrgb > 1)
    maxrgb = 1.0/maxrgb;
  else
    maxrgb = 1.0;

  shader += "uniform sampler2D spriteTex;\n";

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  gl_FragColor = texture2D(spriteTex, gl_TexCoord[0].xy);\n";
  shader += "  gl_FragColor.rgba *= gl_Color.rgba;\n";

  shader += QString("  gl_FragColor.rgba *= vec4(%1, %2, %3, %4);\n").\
                                        arg(r).arg(g).arg(b).arg(maxrgb);
  shader += "}\n";

  return shader;
}
