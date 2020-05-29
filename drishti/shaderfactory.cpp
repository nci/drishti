#include "staticfunctions.h"
#include "shaderfactory.h"
#include "cropshaderfactory.h"
#include "tearshaderfactory.h"
#include "blendshaderfactory.h"
#include "glowshaderfactory.h"
#include "pathshaderfactory.h"
#include "global.h"
#include "prunehandler.h"

#include <QTextEdit>
#include <QVBoxLayout>

QString
ShaderFactory::tagVolume()
{
  // use buffer values to apply tag colors
  QString shader;
  shader += "  float ptx;\n";
  shader += "  if (!mixTag)\n";
  shader += "    ptx = prunefeather.z;\n";
  shader += "  else\n";
  shader += "    ptx = vg.x;\n"; // -- take voxel value

  //shader += "  float ptx = prunefeather.z;\n";
  //shader += "  vec4 paintColor = texture1D(paintTex, ptx);\n";
  shader += "  vec4 paintColor = texture(paintTex, ptx);\n";

  //  shader += "  glFragColor = vec4(paintColor.a,0.1,0.1,0.1);\n";

  shader += "  ptx *= 255.0;\n";

  shader += "  if (ptx > 0.0 || mixTag) \n";
  shader += "  {\n";
  shader += "    paintColor.rgb *= glFragColor.a;\n";
 // if paintColor is black then change only the transparency
  shader += "    if (paintColor.r+paintColor.g+paintColor.b > 0.01)\n";
  shader += "      glFragColor.rgb = mix(glFragColor.rgb, paintColor.rgb, paintColor.a);\n";
  shader += "    else\n";
  shader += "      glFragColor *= paintColor.a;\n";
  shader += "  }\n";
  shader += "  else\n";
  shader += "    glFragColor *= paintColor.a;\n";

  return shader;
}

QString
ShaderFactory::blendVolume()
{
  // use buffer values for blending transfer function
  QString shader;
  shader += "  float ptx = prunefeather.z;\n";
  shader += "  ptx *= 255.0;\n";
  shader += QString("  if (ptx > 0.0 && ptx < float(%1)) \n").	\
    arg(float(Global::lutSize()));
  shader += "  {\n";
  shader += QString("    vec2 vgc = vec2(vg.x, vg.y+ptx/float(%1));\n"). \
    arg(float(Global::lutSize()));
  shader += "    glFragColor = texture2D(lutTex, vgc);\n";
  shader += "  }\n";

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

int
ShaderFactory::loadShaderFromFile(GLhandleARB obj, const char *filename)
{
 FILE *fd = fopen(filename, "rb");
  if (fd == NULL)
    return 0;

  char c;
  int len = 0;
  while(feof(fd) == 0)
    {
      fread(&c, sizeof(char), 1, fd);
      len++;
    }

  rewind(fd);
  char *str = new char[len];
  fread(str, sizeof(char), len-1, fd);
  str[len-1] = '\0';

  const char* source = (const char*)str;
  glShaderSourceARB(obj, 1, &source, NULL);

  delete [] str;
  fclose(fd);

 return 1;
}


bool
ShaderFactory::loadShader(GLhandleARB &progObj,
			  QString shaderString)
{
  GLhandleARB fragObj = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);  
  glAttachObjectARB(progObj, fragObj);

  GLhandleARB vertObj = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);  
  glAttachObjectARB(progObj, vertObj);

  {  // vertObj
    QString qstr;
#ifndef Q_OS_MACX
    qstr += "#version 130\n";
    qstr += "out float gl_ClipDistance[2];\n";
    qstr += "uniform vec4 ClipPlane0;\n";
    qstr += "uniform vec4 ClipPlane1;\n";
#endif
    qstr += "out vec3 pointpos;\n";
    qstr += "out vec3 glTexCoord0;\n";
    qstr += "void main(void)\n";
    qstr += "{\n";
    qstr += "  // Transform vertex position into homogenous clip-space.\n";
    qstr += "  gl_FrontColor = gl_Color;\n";
    qstr += "  gl_BackColor = gl_Color;\n";
    qstr += "  gl_Position = ftransform();\n";
    qstr += "  gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n";
    qstr += "  gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;\n";
    qstr += "  gl_TexCoord[2] = gl_TextureMatrix[2] * gl_MultiTexCoord2;\n";
    qstr += "  pointpos = gl_Vertex.xyz;\n";
    qstr += "  glTexCoord0 = gl_TexCoord[0].xyz;\n";
#ifndef Q_OS_MACX
    qstr += "  gl_ClipDistance[0] = dot(gl_Vertex, ClipPlane0);\n";
    qstr += "  gl_ClipDistance[1] = dot(gl_Vertex, ClipPlane1);\n";
#else
    qstr += "  gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n";
#endif
    qstr += "}\n";
    
    int len = qstr.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, qstr.toLatin1().data());
    const char *sstr = tbuffer;
    glShaderSourceARB(vertObj, 1, &sstr, NULL);
    delete [] tbuffer;

    GLint compiled = -1;
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
  
    GLint compiled = -1;
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
	
	//-----------------------------------
	// display error
	
	//qApp->beep();
	
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
	showError->setWindowTitle("Error in Fragment Shader");
	showError->setSizeGripEnabled(true);
	showError->setModal(true);
	showError->setLayout(layout);
	showError->exec();
	//-----------------------------------
	
	return false;
      }
  }

  
  //----------- link program shader ----------------------
  GLint linked = -1;
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
ShaderFactory::genDefaultShaderString(bool bit16,
				      bool emissive,
				      int nvol)
{
  QString shader;

  shader = "varying vec3 pointpos;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform float tfSet;\n";
  shader += "uniform vec3 delta;\n";
  shader += "void main(void)\n";
  shader += "{\n";

  shader += " if (max(gl_TexCoord[0].x, max(gl_TexCoord[0].y, gl_TexCoord[0].z)) > 1.0)\n";
  shader += "  discard;\n";

  shader += "  vec4 vg;\n";

  if (nvol == 1) shader += "  vg.x = (texture3D(dataTex, gl_TexCoord[0].xyz)).x;\n";
  else if (nvol == 2) shader += "  vg.xy = (texture3D(dataTex, gl_TexCoord[0].xyz)).xw;\n";
  else if (nvol == 3) shader += "  vg.xyz = (texture3D(dataTex, gl_TexCoord[0].xyz)).xyz;\n";
  else if (nvol == 4) shader += "  vg = texture3D(dataTex, gl_TexCoord[0].xyz);\n";

  if (bit16)
    {
      shader += "int h0, h1;\n";
      shader += "float fh0, fh1;\n";

      for(int i=1; i<=nvol; i++)
	{	  
	  QString c;
	  if (i == 1) c = "x";
	  else if (i == 2) c = "y";
	  else if (i == 3) c = "z";
	  else if (i == 4) c = "w";
	  
	  shader += QString("h0 = int(65535.0*vg.%1);\n").arg(c);
	  shader += "h1 = h0 / 256;\n";
	  shader += "h0 = int(mod(float(h0),256.0));\n";
	  shader += "fh0 = float(h0)/256.0;\n";
	  shader += "fh1 = float(h1)/256.0;\n";


	  shader += QString("  vec2 vol%1 = vec2(fh0, tfSet+float(%2)+(fh1/float(%3)));\n"). \
	    arg(i).							\
	    arg((float)(i-1)/Global::lutSize()).			\
	    arg(Global::lutSize());
	}
    }
  else
    {
      if (Global::use1D())
	{
	  for(int i=1; i<=nvol; i++)
	    {
	      QString c;
	      if (i == 1) c = "x";
	      else if (i == 2) c = "y";
	      else if (i == 3) c = "z";
	      else if (i == 4) c = "w";
	      
	      shader += QString("  vec2 vol%1 = vec2(vg.%2, tfSet+float(%3));\n"). \
		arg(i).arg(c).arg((float)(i-1)/Global::lutSize());
	    }
	}
      else
	{
	  QString xyzw;
	  if (nvol == 1) xyzw = "x";
	  else if (nvol == 2) xyzw = "xw";
	  else if (nvol == 3) xyzw = "xyz";
	  else if (nvol == 4) xyzw = "xyzw";
	  
	  QString vecstr;
	  if (nvol == 1) vecstr = "  float";
	  else if (nvol == 2) vecstr = "  vec2";
	  else if (nvol == 3) vecstr = "  vec3";
	  else if (nvol == 4) vecstr = "  vec4";
	  
	  shader += vecstr + "  samplex1, samplex2;\n";
	  shader += vecstr + "  sampley1, sampley2;\n";
	  shader += vecstr + "  samplez1, samplez2;\n";
	  
	  shader += "  vec4 dlt;\n";
	  shader += "  dlt.xyz = delta;\n";
	  shader += "  dlt.w = 0.0;\n";
	  
	  shader += "  vec3 normal;\n";
	  
	  shader += "  samplex1 = texture3D(dataTex, gl_TexCoord[0].xyz+dlt.xww)." + xyzw + ";\n";
	  shader += "  samplex2 = texture3D(dataTex, gl_TexCoord[0].xyz-dlt.xww)." + xyzw + ";\n";
	  shader += "  sampley1 = texture3D(dataTex, gl_TexCoord[0].xyz+dlt.wyw)." + xyzw + ";\n";
	  shader += "  sampley2 = texture3D(dataTex, gl_TexCoord[0].xyz-dlt.wyw)." + xyzw + ";\n";
	  shader += "  samplez1 = texture3D(dataTex, gl_TexCoord[0].xyz+dlt.wwz)." + xyzw + ";\n";
	  shader += "  samplez2 = texture3D(dataTex, gl_TexCoord[0].xyz-dlt.wwz)." + xyzw + ";\n";
	  
	  shader += "  vec3 sample1, sample2;\n";
	  for(int i=1; i<=nvol; i++)
	    {
	      QString c;
	      if (i == 1) c = "x";
	      else if (i == 2) c = "y";
	      else if (i == 3) c = "z";
	      else if (i == 4) c = "w";
	      
	      if(nvol == 1)
		{
		  shader += QString("  sample1 = vec3(samplex1, sampley1, samplez1);\n");
		  shader += QString("  sample2 = vec3(samplex2, sampley2, samplez2);\n");
		}	  
	      else
		{
		  shader += QString("  sample1 = vec3(samplex1.%1, sampley1.%1, samplez1.%1);\n").arg(c);
		  shader += QString("  sample2 = vec3(samplex2.%1, sampley2.%1, samplez2.%1);\n").arg(c);
		}
	      shader += QString("  vec3 normal%1 = (sample1-sample2);\n").arg(i);
	      shader += QString("  float grad%1 = clamp(length(normal%1), 0.0, 0.996);\n").arg(i);
	      shader += QString("  vec2 vol%1 = vec2(vg.%2, tfSet+float(%3)+(grad%1/float(%4)));\n"). \
		arg(i).arg(c).						\
		arg((float)(i-1)/Global::lutSize()).			\
		arg(Global::lutSize());
	    }
	}
    }

  if (nvol > 1)
    {
      shader += "  float alpha = 0.0;\n";
      shader += "  float totalpha = 0.0;\n";
      shader += "  vec3 rgb = vec3(0.0,0.0,0.0);\n";
      for(int i=1; i<=nvol; i++)
	{
	  shader += QString("  vec4 color%1 = texture2D(lutTex, vol%1);\n").arg(i);
	  shader += QString("  rgb += color%1.rgb;\n").arg(i);
	  shader += QString("  totalpha += color%1.a;\n").arg(i);
	  shader += QString("  alpha = max(alpha, color%1.a);\n").arg(i);
	}
      shader += "  gl_FragColor.a = alpha;\n";
      shader += "  gl_FragColor.rgb = alpha*rgb/totalpha;\n";
    }
  else
    shader += "  gl_FragColor = texture2D(lutTex, vol1);\n";

  shader += "\n";

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";

  shader += "  if (gl_FragColor.a < 0.005)\n";
  shader += "	discard;\n";


  shader += "}\n";

  return shader;
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
      shader += "  gl_FragColor.rgba = color/16.0;\n";
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

      shader += "  gl_FragColor.rgba = color/8.0;\n";
    }
  else
    shader += "  color = texture2DRect(blurTex, spos.xy);\n";

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";

  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genLutShaderString(bool bit16)
{
  QString shader;
  shader += "uniform sampler2D lutTex;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec2 vg;\n";
  shader += "  vg = gl_TexCoord[0].xy;\n";
  shader += QString("  vg.y /= float(%1);\n").arg(Global::lutSize());

  shader += "  if (vg.x < 0.0)\n";
  shader += "    gl_FragColor = gl_Color;\n";

  shader += "  else if (vg.x <= 1.0)\n"; // plot color
  shader += "    {\n";
  if (bit16)
    {
      shader += "  int h0, h1;\n";
      shader += "  float fh0, fh1;\n";
      shader += "  h0 = int(65535.0*vg.x);\n";
      shader += "  h1 = h0 / 256;\n";
      shader += "  h0 = int(mod(float(h0),256.0));\n";
      shader += "  fh0 = float(h0)/256.0;\n";
      shader += "  fh1 = float(h1)/256.0;\n";
      shader += QString("  vg = vec2(fh0, vg.y+(fh1/float(%1)));\n"). \
	arg(Global::lutSize());
    }
  shader += "      gl_FragColor = texture2D(lutTex, vg.xy);\n";
  shader += "      if (gl_FragColor.a > 0.004)\n";
  shader += "        {\n";
  shader += "           gl_FragColor.rgb /= gl_FragColor.a;\n";
  shader += "           gl_FragColor.a = 1.0;\n";
  shader += "        }\n";
  shader += "      else gl_FragColor = vec4(0.0,0.0,0.0,0.5);\n";

  shader += "    }\n";
  shader += "  else\n"; // vg.x > 1.0 - plot opacity curve
  shader += "    {\n";
  shader += "      float t = gl_TexCoord[1].x;\n";
  shader += "      vg.x -= 2.0;\n";
  if (bit16)
    {
      shader += "  int h0, h1;\n";
      shader += "  float fh0, fh1;\n";
      shader += "  h0 = int(65535.0*vg.x);\n";
      shader += "  h1 = h0 / 256;\n";
      shader += "  h0 = int(mod(float(h0),256.0));\n";
      shader += "  fh0 = float(h0)/256.0;\n";
      shader += "  fh1 = float(h1)/256.0;\n";
      shader += QString("  vg = vec2(fh0, vg.y+(fh1/float(%1)));\n"). \
	arg(Global::lutSize());
    }
  shader += "      float a = texture2D(lutTex, vg.xy).a;\n";
  shader += "      float t1 = smoothstep(a-0.05, a, t);\n";
  shader += "      t = t1 * (1.0-smoothstep(a, a+0.05, t));\n";
  shader += "      t = clamp(0.0, 2.0*t, 1.0);\n";
  shader += "      gl_FragColor = vec4(t,t,t,1.0)*0.5;\n";
  shader += "    }\n";

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
ShaderFactory::genReduceShaderString()
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect rtex;\n";
  shader += "uniform int lod;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec2 spos = gl_TexCoord[0].xy*vec2(lod);\n";
  shader += "  vec4 sum = vec4(0,0,0,0);\n";
  shader += "  sum += texture2DRect(rtex, spos.xy);\n";
  shader += "  sum += texture2DRect(rtex, spos.xy + vec2(lod)*vec2(0,1));\n";
  shader += "  sum += texture2DRect(rtex, spos.xy + vec2(lod)*vec2(1,0));\n";
  shader += "  sum += texture2DRect(rtex, spos.xy - vec2(lod)*vec2(0,1));\n";
  shader += "  sum += texture2DRect(rtex, spos.xy - vec2(lod)*vec2(1,0));\n";
  shader += "  sum += texture2DRect(rtex, spos.xy + vec2(lod)*vec2(1,1));\n";
  shader += "  sum += texture2DRect(rtex, spos.xy + vec2(lod)*vec2(1,-1));\n";
  shader += "  sum += texture2DRect(rtex, spos.xy - vec2(lod)*vec2(1,1));\n";
  shader += "  sum += texture2DRect(rtex, spos.xy - vec2(lod)*vec2(1,-1));\n";
  shader += "  gl_FragColor = sum/vec4(9);\n";
  //shader += "  gl_FragColor = sum;\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genExtractSliceShaderString()
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect atex;\n";
  shader += "uniform sampler2DRect btex;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec2 spos = gl_TexCoord[0].xy;\n";
  shader += "  vec4 a = texture2DRect(atex, spos.xy);\n";
  shader += "  vec4 b = texture2DRect(btex, spos.xy);\n";
  shader += "  gl_FragColor = (a-b)/(vec4(1)-b);\n";
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

QString
ShaderFactory::genPeelShader(bool peel, int peelType,
			     float peelMin, float peelMax, float peelMix,
			     bool lighting)
{
  QString shader;

  if (peel && peelType!=2)
    {
      if (!lighting) 
	{
	  shader += "  normal = normalize(sample1 - sample2);\n";
	  shader += "  normal = mix(vec3(0.0,0.0,0.0), normal, step(0.0, grad));"; 
	  shader += "  vec3 voxpos = pointpos;\n";
	  shader += "  vec3 I = voxpos - eyepos;\n";
	  shader += "  I = normalize(I);\n";
	}
      shader += "  float IdotN = dot(I, normal);\n";

      if (peelType == 0)
	{
	  if (peelMin < peelMax)
	    shader += QString("  val1 = smoothstep(float(%1), float(%2), IdotN);\n").arg(peelMin).arg(peelMax);
	  else
	    shader += QString("  val1 = smoothstep(float(%1), float(%2), -IdotN);\n").arg(peelMax).arg(peelMin);
	}
      else if (peelType == 1)
	{ //---- keep inside
	  shader += QString("  val1 = 1.0-smoothstep(float(%1)-0.1, float(%1), IdotN);\n").arg(peelMin);
	  shader += QString("  val1 *= smoothstep(float(%1), float(%1)+0.1, IdotN);\n").arg(peelMax);
	}

      shader += QString("  IdotN = mix(val1, 1.0, float(%1));\n").arg(peelMix);
      shader += "  glFragColor.rgba *= IdotN;\n";
    }

  return shader;
}

QString
ShaderFactory::genTextureCoordinate()
{
  QString shader;
  shader = "vec2 getTextureCoordinate(int slice, int gridx,\n";
  shader += "                         int tsizex, int tsizey,\n";
  shader += "                         vec2 texcoord)\n";
  shader += "{\n";
  shader += "  int row = slice/gridx;\n";
  shader += "  int col = slice - row*gridx;\n";
  shader += "  vec2 tc = vec2(texcoord.x + float(col*tsizex),\n";
  shader += "                 texcoord.y + float(row*tsizey));\n";
  shader += "  return tc;\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::getNormal()
{
  QString shader;
  // using tetrahedron technique - iquilezles.org (normalsSDF) - Paul Malin in ShaderToy
  shader += "vec4 getNormal(vec3 voxelCoord)\n";
  shader += "{\n";
  shader += "  vec2 k = vec2(1.0, -1.0);\n";
  shader += "  vec4 gval = vec4(getVal(voxelCoord+k.xyy)[0].x,\n";
  shader += "                   getVal(voxelCoord+k.yyx)[0].x,\n";
  shader += "                   getVal(voxelCoord+k.yxy)[0].x,\n";
  shader += "                   getVal(voxelCoord+k.xxx)[0].x);\n";
  shader += "  vec3 grad = (k.xyy * gval.x +\n";
  shader += "               k.yyx * gval.y +\n";
  shader += "               k.yxy * gval.z +\n";
  shader += "               k.xxx * gval.w);\n";
  shader += "  float gmax = max(gval.x,max(gval.y,max(gval.z,gval.w)));\n";
  shader += "  float gmin = min(gval.x,min(gval.y,min(gval.z,gval.w)));\n";
  shader += "  return vec4(grad, gmax-gmin);\n";
  
  //shader += "  vec3 grad = vec3(0.0);\n";
  //shader += "  vec4 gnval = vec4(0.0);\n";
  //shader += "  vec2 k = vec2(1.0, -1.0);\n";
  //shader += "  for(int gn=1; gn<5; gn++)\n";
  //shader += "  {\n";
  //shader += "    vec2 kgn = k*vec2(gn);\n";
  //shader += "    vec4 gval = vec4(getVal(voxelCoord+kgn.xyy)[0].x,\n";
  //shader += "                     getVal(voxelCoord+kgn.yyx)[0].x,\n";
  //shader += "                     getVal(voxelCoord+kgn.yxy)[0].x,\n";
  //shader += "                     getVal(voxelCoord+kgn.xxx)[0].x);\n";
  //shader += "    grad += (k.xyy * gval.x +\n";
  //shader += "             k.yyx * gval.y +\n";
  //shader += "             k.yxy * gval.z +\n";
  //shader += "             k.xxx * gval.w);\n";
  //shader += "    gnval += gval;\n";
  //shader += "  }\n";
  //shader += "  vec4 gval = gnval/vec4(4.0);\n";
  //shader += "  float gmax = max(gval.x,max(gval.y,max(gval.z,gval.w)));\n";
  //shader += "  float gmin = min(gval.x,min(gval.y,min(gval.z,gval.w)));\n";
  //shader += "  return vec4(normalize(grad), gmax-gmin);\n";
  //shader += "  vec3 grad = (k.xyy * getVal(voxelCoord+k.xyy)[0].x +\n";
  //shader += "               k.yyx * getVal(voxelCoord+k.yyx)[0].x +\n";
  //shader += "               k.yxy * getVal(voxelCoord+k.yxy)[0].x +\n";
  //shader += "               k.xxx * getVal(voxelCoord+k.xxx)[0].x);\n";
  //shader += "  grad = grad/2.0;\n";  // should be actually divided by 4
  //shader += "  grad = grad/4.0;\n";  // should be actually divided by 4
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::addLighting()
{
  QString shader;

  shader += "  normal = normalize(normal);\n";
  shader += "  normal = mix(vec3(0.0,0.0,0.0), normal, step(0.0, grad));"; 

  shader += "  vec3 voxpos = pointpos;\n";
  shader += "  vec3 I = voxpos - eyepos;\n";
  shader += "  vec3 lightvec = voxpos - lightpos;\n";
  shader += "  I = normalize(I);\n";
  shader += "  lightvec = normalize(lightvec);\n";

  shader += "  vec3 reflecvec = reflect(lightvec, normal);\n";
  shader += "  float DiffMag = abs(dot(normal, lightvec));\n";
  shader += "  vec3 Diff = (diffuse*DiffMag)*glFragColor.rgb;\n";
  shader += "  float Spec = pow(abs(dot(normal, reflecvec)), speccoeff);\n";
  shader += "  Spec *= specular*glFragColor.a;\n";

  shader += "  vec3 Amb = ambient*glFragColor.rgb;\n";

  shader += "  float litfrac;\n";
  shader += "  litfrac = smoothstep(0.05, 0.1, grad);\n";
  shader += "  glFragColor.rgb = mix(glFragColor.rgb, Amb, litfrac);\n";
  shader += "  if (litfrac > 0.0)\n";
  shader += "   {\n";
  shader += "     vec3 frgb = glFragColor.rgb + litfrac*(Diff + Spec);\n";
  shader += "     if (any(greaterThan(frgb,vec3(1.0,1.0,1.0)))) \n";
  shader += "        frgb = vec3(1.0,1.0,1.0);\n";
  shader += "     if (any(greaterThan(frgb,glFragColor.aaa))) \n";  
  shader += "        frgb = glFragColor.aaa;\n";
  shader += "     glFragColor.rgb = frgb;\n";
  shader += "   }\n";

  shader += "  glFragColor.rgb *= lightcol;\n";

  return shader;
}

QString
ShaderFactory::genPreVgx()
{
  QString shader;

  //shader += "  vec2 vg, vg1;\n";
  shader += "  vec4 vg, vg1;\n";
  shader += "  float val0, val1;\n";
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

      shader += "  int pZslc = int(float(zoffset+slice)/float(prunelod));\n";
      shader += "  float pZslcf = fract(float(zoffset+slice)/float(prunelod));\n";

      shader += "  vec2 pvg0 = getTextureCoordinate(pZslc, ";
      shader += "              prunegridx, prunetsizex, prunetsizey, pvg);\n";      
      shader += "  vec2 pvg1 = getTextureCoordinate(pZslc+1, ";
      shader += "              prunegridx, prunetsizex, prunetsizey, pvg);\n";

      shader += "  vec4 pf0 = texture2DRect(pruneTex, pvg0);\n";
      shader += "  vec4 pf1 = texture2DRect(pruneTex, pvg1);\n";
      shader += "  pf0 = mix(pf0, pf1, pZslcf);\n";

      shader += "  vec4 prunefeather = pf0;\n";
      shader += "  if (prunefeather.x < 0.005) discard;\n";

      // delta condition added for reslicing/ option
      //shader += "  if (delta.x < 1.0 && prunefeather.x < 0.005) discard;\n";

      // tag values should be non interpolated - nearest neighbour
      shader += "  prunefeather.z = texture2DRect(pruneTex, vec2(floor(pvg0.xy)+vec2(0.5))).z;\n";
    }
  //---------------------------------------------------------------------


  return shader;
}


QString
ShaderFactory::genVgx()
{
  QString shader;

  shader += "  vec4 voxValues[3] = getVal(vtexCoord);\n"; // interpolated

  shader += "  if (linearInterpolation && !mixTag)\n";
  shader += "    vg.x = voxValues[0].x;\n"; // interpolated
  shader += "  else\n";
  shader += "    vg.x = voxValues[1].x;\n"; // nearest neighbour

  return shader;
}

QString
ShaderFactory::getVal()
{
  QString shader;
  shader += "vec4[3] getVal(vec3 voxelCoord)\n";  
  shader += "{\n";
  shader += "  float layer = voxelCoord.z;\n";
  shader += "  float layer0 = min(vsize.z-1.0,floor(voxelCoord.z));\n";
  shader += "  float layer1 = layer0+1.0;\n";
  shader += "  float frc = layer-layer0;\n";
  shader += "  vec3 vcrd0 = vec3(voxelCoord.xy/vsize.xy, layer0);\n";
  shader += "  vec3 vcrd1 = vec3(voxelCoord.xy/vsize.xy, layer1);\n";

  shader += "  vec4 values[3];\n";
  shader += "  values[1] = texture(dataTexAT, vcrd0);\n";
  shader += "  values[2] = texture(dataTexAT, vcrd1);\n";
  shader += "  values[0] = mix(values[1], values[2], frc);\n";
  
  
  shader += "  return values;\n";
  shader += "}\n";

  return shader;
}


QString
ShaderFactory::genDefaultSliceShaderString(bool bit16,
					   bool lighting,
					   bool emissive,
					   QList<CropObject> crops,
					   bool peel, int peelType,
					   float peelMin, float peelMax, float peelMix)
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

  for(int i=0; i<crops.count(); i++)
    if (crops[i].cropType() >= CropObject::View_Tear &&
	crops[i].cropType() <= CropObject::View_Block &&
	crops[i].magnify() > 1.0)
      tearPresent = true;

  bool pathCropPresent = PathShaderFactory::cropPresent();
  bool pathViewPresent = PathShaderFactory::blendPresent();

  float lastSet = (Global::lutSize()-1.0)/Global::lutSize();
  QString shader;

  shader = "#version 450 core\n";
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "in vec3 pointpos;\n";
  shader += "in vec3 glTexCoord0;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect dataTex;\n";
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

  shader += genTextureCoordinate();

  //---------------------
  // get voxel value from array texture
  shader += getVal();
  shader += getNormal();
  //---------------------


  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (glowPresent) shader += GlowShaderFactory::generateGlow(crops);  
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();
  if (pathViewPresent) shader += PathShaderFactory::applyPathBlend();


  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec3 lightcol = vec3(1.0,1.0,1.0);\n";

  shader += "  vec3 texCoord = glTexCoord0.xyz;\n";

  shader += "  if (any(lessThan(texCoord,brickMin)) || ";
  shader += "  any(greaterThan(texCoord, brickMax)))\n";
  shader += "    discard;\n";

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

  //------
  shader += "vec3 vtexCoord = (texCoord-vmin)/lod;\n";
  // for nearest neighbour interpolation
  shader += "if (!linearInterpolation || mixTag)\n";
  shader += "{\n";
  shader += "  vtexCoord = vec3(floor(vtexCoord)+vec3(0.5));\n";
  shader += "}\n";
  //------

  shader += "texCoord.xy = vec2(tsizex,tsizey)*(vtexCoord.xy/vsize.xy);\n";
  shader += "texCoord.z = vtexCoord.z;\n";


  shader += genPreVgx();
  shader += genVgx();

  shader += "  float value = vg.x;\n";

  //----------------------------------
  // for upscaling or volume and surface area calculations
  // use nearest neighbour interpolation
  shader += "  if (depthcue > 1.0) vg.x = voxValues[1].x;\n";
  //----------------------------------

  //----------------------------------
  //------------------------------------
  shader += "if (lightlod > 0)\n";
  shader += "  {\n"; // calculate light color
  shader += "    float llod = prunelod*float(lightlod);\n";
  shader += "    vec2 pvg = texCoord.xy/llod;\n";
  shader += "    int lbZslc = int((zoffset+texCoord.z)/llod);\n";
  shader += "    float lbZslcf = fract((zoffset+texCoord.z)/llod);\n";
  shader += "    vec2 pvg0 = getTextureCoordinate(lbZslc, ";
  shader += "                  lightncols, lightgridx, lightgridy, pvg);\n";
  shader += "    vec2 pvg1 = getTextureCoordinate(lbZslc+1, ";
  shader += "                  lightncols, lightgridx, lightgridy, pvg);\n";	       
  shader += "    vec3 lc0 = texture2DRect(lightTex, pvg0).xyz;\n";
  shader += "    vec3 lc1 = texture2DRect(lightTex, pvg1).xyz;\n";
  shader += "    lightcol = mix(lc0, lc1, lbZslcf);\n";
  shader += "    lightcol = 1.0-pow((vec3(1,1,1)-lightcol),vec3(lod,lod,lod));\n";
  shader += "  }\n";
  shader += "else\n";
  shader += "  lightcol = vec3(1.0,1.0,1.0);\n";

  shader += "if (shdlod > 0)\n";
  shader += "  {\n";
  shader += "     float sa = texture2DRect(shdTex, gl_FragCoord.xy*vec2(dofscale)/vec2(shdlod)).a;\n";
  shader += "     sa = 1.0-smoothstep(0.0, shdIntensity, sa);\n";
  shader += "     sa = clamp(0.1, sa, 1.0);\n";
  shader += "     lightcol *= sa;\n";
  shader += "  }\n";
  //----------------------------------

  if (peel || lighting || !Global::use1D())
    {
      //shader += "  vec3 normal = getNormal(vtexCoord);\n";
      //shader += "  float grad = clamp(length(normal), 0.0, 0.996);\n";
      shader += "  vec4 normalG = getNormal(vtexCoord);\n";
      shader += "  vec3 normal = normalG.xyz;\n";
      shader += "  float grad = clamp(length(normalG.w), 0.0, 0.996);\n";
    }

  if (bit16)
    {
      shader += "  int h0 = int(65535.0*vg.x);\n";
      shader += "  int h1 = h0 / 256;\n";
      shader += "  h0 = int(mod(float(h0),256.0));\n";
      shader += "  float fh0 = float(h0)/256.0;\n";
      shader += "  float fh1 = float(h1)/256.0;\n";

      shader += QString("  vg.xy = vec2(fh0, fh1*%1);\n").arg(1.0/Global::lutSize());
    }
  else
    {
      if (Global::use1D())
	shader += "  vg.y = 0.0;\n";
      else
	shader += QString("  vg.y = grad*%1;\n").arg(1.0/Global::lutSize());
    }

  shader += "  vg1 = vg;\n";
  shader += "  vg.y += tfSet;\n";
  shader += "  glFragColor = texture2D(lutTex, vg.xy);\n";
  

  if (Global::emptySpaceSkip())
    {
      shader += "  glFragColor.rgba = mix(vec4(0.0,0.0,0.0,0.0), glFragColor.rgba, prunefeather.x);\n";
      if (PruneHandler::blend())
	shader += blendVolume();
      else
	shader += tagVolume();
    }


  if (tearPresent || cropPresent || pathCropPresent)
    shader += "  glFragColor.rgba = mix(glFragColor.rgba, vec4(0.0,0.0,0.0,0.0), feather);\n";

  if (viewPresent) shader += "  blend(false, otexCoord, vg.xy, glFragColor);\n";
  
  if (pathViewPresent) shader += "pathblend(otexCoord, vg.xy, glFragColor);\n";
  

//---------------------------------
  if (Global::emptySpaceSkip())
    {
      shader += "if (delta.x > 1.0)\n";
      shader += "  { glFragColor = vec4(value*step(0.001,glFragColor.a),";
      shader += "glFragColor.a, prunefeather.z, 1.0); return; }\n";
    }
  else
    {
      shader += "if (delta.x > 1.0)\n";
      shader += "  { glFragColor = vec4(value*step(0.001,glFragColor.a),";
      shader += "glFragColor.a, 0.0, 1.0); return; }\n";
    }
//---------------------------------

//------------------------------------
  shader += "glFragColor = 1.0-pow((vec4(1,1,1,1)-glFragColor),";
  shader += "vec4(lod,lod,lod,lod));\n";
//------------------------------------

  shader += "\n";
  shader += "  if (glFragColor.a < 0.005)\n";
  shader += "	discard;\n";

  if (lighting)
    shader += addLighting();

  shader += genPeelShader(peel, peelType,
			  peelMin, peelMax, peelMix,
			  lighting);

  if (emissive)
    {
      shader += QString("  vg1.y += float(%1);\n").arg(lastSet);
      shader += "  glFragColor.rgb += texture2D(lutTex, vg1.xy).rgb;\n";
    }

  // -- depth cueing
  shader += "  glFragColor.rgb *= min(1.0,depthcue);\n";

  shader += "  glFragColor *= opmod;\n";

  if (glowPresent) shader += "  glFragColor.rgb += glow(otexCoord);\n";

  shader += "  glFragColor = clamp(glFragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";

  shader += "  glFragColor.rgb = pow(glFragColor.rgb, vec3(gamma));\n";

  shader += "}\n";

  return shader;
}

//----------------------------
//----------------------------

GLint ShaderFactory::m_meshShaderParm[20];
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
	m_meshShaderParm[4] = glGetUniformLocation(m_meshShader, "MV");
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
	m_meshShaderParm[16] = glGetUniformLocation(m_meshShader, "MVShadow");
	m_meshShaderParm[17] = glGetUniformLocation(m_meshShader, "idx");
	
    }

  return m_meshShader;
}
  
QString
ShaderFactory::meshShaderV()
{
  QString shader;

  shader += "#version 420 core\n";
  shader += "uniform mat4 MV;\n";
  shader += "uniform mat4 MVShadow;\n";
  shader += "uniform mat4 MVP;\n";
  shader += "uniform mat4 localXform;\n";
  shader += "uniform float idx;\n";
  shader += "layout(location = 0) in vec3 position;\n";
  shader += "layout(location = 1) in vec3 normalIn;\n";
  shader += "layout(location = 2) in vec3 colorIn;\n";
  shader += "out vec3 v3Normal;\n";
  shader += "out vec3 v3Color;\n";
  shader += "out vec3 pointPos;\n";
  shader += "out float zdepth;\n";
  shader += "out float zdepthS;\n";
  shader += "out float surfId;\n";
  shader += "out vec3 ;\n";
  shader += "void main()\n";
  shader += "{\n";
  shader += "   pointPos = (localXform * vec4(position, 1)).xyz;\n";
  shader += "   v3Color = colorIn;\n";
  shader += "   v3Normal = normalIn;\n";
  shader += "   gl_Position = MVP * vec4(position, 1);\n";
  shader += "   zdepth = ((gl_DepthRange.diff * gl_Position.z/gl_Position.w) +\n";
  shader += "              gl_DepthRange.near + gl_DepthRange.far) / 2.0;\n";
  shader += "   zdepthS = -(MVShadow * vec4(pointPos, 1)).z;\n";
  shader += "   surfId = idx;\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::meshShaderF()
{
  QString shader;

  shader += "#version 420 core\n";
  shader += "uniform sampler2D diffuseTex;\n";
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
  
  shader += "\n";
  shader += "in vec3 v3Color;\n";
  shader += "in vec3 v3Normal;\n";
  shader += "in vec3 pointPos;\n";
  shader += "in float zdepth;\n";
  shader += "in float zdepthS;\n";
  shader += "in float surfId;\n";

  // Ouput data
  shader += "layout (location=0) out vec4 outputColor;\n";
  shader += "layout (location=1) out vec4 depth;\n";
  shader += "layout (depth_greater) out float gl_FragDepth;\n";
  
  shader += rgb2hsv();
  shader += hsv2rgb();

  shader += "void main()\n";
  shader += "{\n";

  shader += "  if (dot(v3Normal,-viewDir) > extras.y) discard;\n";

  shader += "gl_FragDepth = zdepth;\n";

  //-------------------  store depth values
  shader += "depth = vec4(zdepthS/sceneRadius, extras.z, surfId, gl_FragDepth);\n";
  //-------------------  Camera depth,  glow,  surface id, fragment depth
  
  shader += "  if (hasUV)\n";
  shader += "     outputColor = texture(diffuseTex, vec2(v3Color.x, 1-v3Color.y));\n";
  shader += "  else\n";
  shader += "     outputColor = vec4(v3Color, 1.0);\n";

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

  shader += "    vec3 glowColor = rgb2hsv(outputColor.rgb);\n";

  shader += "\n";
  shader += "  vec3 Amb = ambient*outputColor.rgb;\n";
  shader += "  float diffMag = abs(dot(v3Normal, viewDir));\n";
  shader += "  vec3 Diff = diffuse*diffMag*outputColor.rgb;\n";
  shader += "  outputColor.rgb = Amb + Diff;\n";

  // glow when active or has glow switched on
  shader += "    glowColor = pow(glowColor, vec3(1.1, 0.5, 1.0));\n";
  shader += "    glowColor.z = min(glowColor.z+extras.z, 1.0);\n";
  shader += "    float addGlow = glowColor.z;\n";
  shader += "    glowColor = hsv2rgb(glowColor);\n";
  shader += "    outputColor.rgb += 0.5*max(extras.z,extras.x)*glowColor.rgb;\n";

  // darken if required
  shader += "    outputColor.rgb *= extras.w;\n";

  //  shader += "  float metallic = 1.0;\n";
//  shader += "  vec3 specularColor = mix(vec3(0.04), outputColor.rgb, metallic);\n";
//  shader += "  vec3 spec = shadingSpecularGGX(v3Normal, -viewDir,  -viewDir, roughness, specularColor);\n";
//  shader += "  outputColor.rgb = Amb + Diff + specular*spec;\n";

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
	m_meshShadowShaderParm[8] = glGetUniformLocation(m_meshShadowShader, "colorTexS");
	m_meshShadowShaderParm[9] = glGetUniformLocation(m_meshShadowShader, "depthTexS");
    }

  return m_meshShadowShader;
}

QString
ShaderFactory::meshShadowShaderV()
{
  QString shader;
  shader += "#version 420 core\n";
  shader += "\n";
  shader += "layout(location = 0) in vec3 vertex;\n";
  shader += "uniform mat4 MVP;\n";
  shader += "\n";
  shader += "void main()\n";
  shader += "{\n";
  shader += "  gl_Position =  MVP * vec4(vertex,1);\n";
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
  shader += "uniform sampler2DRect colorTexS;\n";
  shader += "uniform sampler2DRect depthTexS;\n";


  shader += rgb2hsv();
  shader += hsv2rgb();
  shader += ggxShader();
  
  shader += "void main()\n";
  shader += "{\n";

  shader += "  vec2 spos = gl_FragCoord.xy;\n";

  shader += "  color = texture2DRect(colorTex, spos.xy);\n";
  shader += "  if (color.a < 0.001)\n";
  shader += "    discard;\n";

  
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
  
  
  shader += "  vec3 ecolor = color.rgb;\n";
  shader += "  if (edges > 0.0)\n";
  shader += "  {\n";
  shader += "    float cx[8] = float[](-1.0, 0.0, 1.0, 0.0, -1.0,-1.0, 1.0, 1.0);\n";
  shader += "    float cy[8] = float[]( 0.0,-1.0, 0.0, 1.0, -1.0, 1.0,-1.0, 1.0);\n";
  shader += "    float response = 0.0;\n";

  // find edges on surfaces
  shader += "    for(int i=0; i<8; i++)\n";
  shader += "      {\n";
  shader += "        vec2 pos = spos + vec2(cx[i],cy[i]);\n";  
  shader += "        float adepth = texture2DRect(depthTex, pos).x;\n";
  shader += "        float od = depth - adepth;\n";
  shader += "        response += max(0.0, od);\n";
  shader += "      } \n";
  shader += "    ecolor.rgb *= exp(-response*pow(20*(edges+0.05), 2*gamma));\n";

  // find border between different surfaces
  shader += "    response = 0.0;\n";
  shader += "    for(int i=0; i<8; i++)\n";
  shader += "      {\n";
  shader += "        vec2 pos = spos + vec2(cx[i],cy[i]);\n";  
  shader += "        float od = step(0.001, abs(surfId - texture2DRect(depthTex, pos).z));\n";
  shader += "        response += max(0.0, od);\n";
  shader += "      } \n";
  shader += "    ecolor.rgb *= pow((1.0-response/8.0), gamma);\n";
  shader += "  }\n";

  shader += "  color.rgb = mix(color.rgb, ecolor, step(softshadow, 0.01));\n";

  shader += "  if (softshadow > 0.0)\n";
  shader += "  {\n";  
  shader += "    vec3 clrO = vec3(0.0);\n";
  shader += "    vec3 clrRL = vec3(0.0);\n";
  shader += "    vec3 clrG = vec3(0.0);\n";
  shader += "    float sumO = 0.0;\n";
  shader += "    float sumRL = 0.0;\n";
  shader += "    float sumS = 0.0;\n";
  shader += "    float sumG = 0.0;\n";
  shader += "    float nRidge = 0.0;\n";
  shader += "    float nValley = 0.0;\n";
  shader += "    int nsteps = int(30.0*softshadow);\n";
  shader += "    int nstepsS = int(nsteps*0.75);\n";
  shader += "    for(int i=0; i<nstepsS; i++)\n";
  shader += "      {\n";
  shader += "    	 float r = 1.0+i*0.15;\n";
  shader += "            float x = r*sin(radians(i*23));\n";
  shader += "            float y = r*cos(radians(i*23));\n";
  shader += "    	 vec2 pos = spos + vec2(x,y);\n";
  shader += "    	 vec3 sdepth = texture2DRect(depthTexS, pos).xyz;\n";
  shader += "    	 vec3 adepth = texture2DRect(depthTex, pos).xyz;\n";

  shader += "    	 sumS += step(r*0.001, depth-sdepth.x);\n";
  shader += "    	 nRidge += step(depth, adepth.x-0.01);\n";
  shader += "    	 nValley += max(0.0,adepth.x-depth);\n";

  // get contributions from other surfaces
  shader += "    	 float sr = step(0.5, abs(surfId-adepth.z))*step(0.0, adepth.z);\n";
  shader += "    	 sumO += sr;\n";
  shader += "    	 clrO += sr*texture2DRect(colorTexS, pos).rgb;\n";

  shader += "    	 vec3 clrR = texture2DRect(colorTex, pos).rgb;\n";
  // get contributions from same surface
  shader += "    	 sr = step(abs(surfId-adepth.z), 0.5);\n";
  shader += "    	 sumRL += sr;\n";
  shader += "    	 clrRL += sr*clrR;\n";

  // get contributions for glowing surfaces
  shader += "    	 sr = step(0.05, adepth.y);\n";
  shader += "    	 sumG += sr;\n";
  shader += "    	 clrG += sr*clrR;\n";
  shader += "      } \n";
  
  shader += "    for(int i=nstepsS; i<nsteps; i++)\n";
  shader += "      {\n";
  shader += "    	 float r = i*0.17;\n";
  shader += "            float x = r*sin(radians(i*27));\n";
  shader += "            float y = r*cos(radians(i*27));\n";
  shader += "    	 vec2 pos = spos + vec2(x,y);\n";
  
  shader += "    	 vec3 adepth = texture2DRect(depthTex, pos).xyz;\n";

  // get contributions from other surfaces
  shader += "    	 float sr = step(0.5, abs(surfId-adepth.z))*step(0.0, adepth.z);\n";
  shader += "    	 sumO += sr;\n";
  shader += "    	 clrO += sr*texture2DRect(colorTexS, pos).rgb;\n";

  shader += "    	 vec3 clrR = texture2DRect(colorTex, pos).rgb;\n";
  // get contributions from same surface
  shader += "    	 sr = step(abs(surfId-adepth.z), 0.5);\n";
  shader += "    	 sumRL += sr;\n";
  shader += "    	 clrRL += sr*clrR;\n";

  // get contributions for glowing surfaces
  shader += "    	 sr = step(0.05, adepth.y);\n";
  shader += "    	 sumG += sr;\n";
  shader += "    	 clrG += sr*clrR;\n";
  shader += "      } \n";

  // add valley
  shader += "    vec3 colorV = rgb2hsv(color.rgb);\n";
  shader += "    colorV *= vec3(1, 2, 0.5);\n";
  shader += "    colorV.x += 0.01;\n";
  shader += "    colorV.x = mix(colorV.x, 1.0-colorV.x, step(1.0, colorV.x));\n";
  shader += "    colorV = clamp(colorV, vec3(0.0), vec3(1.0));\n";
  shader += "    colorV = hsv2rgb(colorV);\n";
  shader += "    colorV = pow(colorV, vec3(1.0/gamma));\n";
  shader += "    float valley = exp(-5*nValley/gamma);\n";
  shader += "    valley = 1.0-smoothstep(0.0, 1.0, valley);\n";
  shader += "    color.rgb = mix(color.rgb, colorV, valley);\n";

  // add surface smoothing
  shader += "    clrRL /= vec3(max(1.0,sumRL));\n";
  shader += "    float lgr = pow(sumRL/float(nsteps), 2*gamma);\n";
  shader += "    lgr = smoothstep(0.0, 1.0, lgr);\n";
  shader += "    color.rgb = mix(color.rgb, clrRL, lgr);\n";

  // add glow
  shader += "    clrG /= vec3(max(1.0,sumG));\n";
  shader += "    float gl = pow(sumG/float(nsteps), gamma);\n";
  shader += "    gl = smoothstep(0.0, 1.0, gl);\n";
  shader += "    color.rgb = mix(color.rgb, clrG, gl);\n";

   // add ridge
  shader += "    float ridge = nRidge/float(nstepsS);\n";
  shader += "    ridge = smoothstep(0.0, 1.0, pow(ridge,gamma));\n";
  shader += "    color.rgb = mix(color.rgb, vec3(1.0), ridge);\n";

  // add reflections from other surfaces
  shader += "    clrO /= vec3(max(1.0,sumO));\n";
  shader += "    float ugr = pow(sumO/float(nsteps), gamma);\n";
  shader += "    ugr = smoothstep(0.0, 1.0, sqrt(ugr));\n";
  shader += "    color.rgb = mix(color.rgb, clrO, ugr);\n";

  // add edges
  shader += "    color.rgb = mix(color.rgb, ecolor, 0.5);\n";

  // add shadow
  shader += "    sumS /= float(nstepsS);\n";
  shader += "    float shadow = clamp(sumS,0.0,1.0);\n";
  shader += "    float shadows = max(0.3, exp(-shadow/gamma));\n";
  shader += "    color.rgb *= shadows;\n";
  
  // lighten shadows a bit
  shader += "    color.rgb = mix(color.rgb, clrO, pow(1.0-shadows, 3.0));\n";


  shader += "  }\n";

  shader += "  if (roughness > 0.0)\n";
  shader += "  {\n";
  shader += "    float dx = 0.0;\n";
  shader += "    float dy = 0.0;\n";
  shader += "    for(int i=0; i<10*gamma; i++)\n";
  shader += "      {\n";
  shader += "        float offset = 1-(1+i)%3;\n";
  shader += "        dx += texture2DRect(depthTex, spos.xy+vec2(1+i,offset)).x -\n";
  shader += "              texture2DRect(depthTex, spos.xy-vec2(1+i,offset)).x;\n";
  shader += "        dy += texture2DRect(depthTex, spos.xy+vec2(offset,1+i)).x -\n";
  shader += "              texture2DRect(depthTex, spos.xy-vec2(offset,1+i)).x;\n";
  shader += "      }\n";
  shader += "    vec3 N = normalize(vec3(dx*50, dy*50, 1.0+roughness));\n";
  shader += "    vec3 spec = shadingSpecularGGX(N, vec3(0,0,1),  vec3(0,0,1), roughness*0.2, color.rgb);\n";
  shader += "    color.rgb += 0.5*specular*spec;\n";
  shader += "  }\n";
  
  
  shader += " color.rgb = clamp(color.rgb, vec3(0.0), vec3(1.0));\n";

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


bool
ShaderFactory::loadShader(GLhandleARB &progObj,
			  QString vertShaderString,
			  QString fragShaderString)
{
  GLhandleARB fragObj = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);  
  glAttachObjectARB(progObj, fragObj);

  GLhandleARB vertObj = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);  
  glAttachObjectARB(progObj, vertObj);

  {  // vertObj   
    int len = vertShaderString.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, vertShaderString.toLatin1().data());
    const char *sstr = tbuffer;
    glShaderSourceARB(vertObj, 1, &sstr, NULL);
    delete [] tbuffer;

    GLint compiled = -1;
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
	
	QString estr;
	QStringList slist = vertShaderString.split("\n");
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
	showError->setWindowTitle("Error in Vertex Shader");
	showError->setSizeGripEnabled(true);
	showError->setModal(true);
	showError->setLayout(layout);
	showError->exec();
	return false;
    }
  }
    
    
  { // fragObj
    int len = fragShaderString.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, fragShaderString.toLatin1().data());
    const char *sstr = tbuffer;
    glShaderSourceARB(fragObj, 1, &sstr, NULL);
    delete [] tbuffer;
  
    GLint compiled = -1;
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
	
	//-----------------------------------
	// display error
	
	//qApp->beep();
	
	QString estr;
	QStringList slist = fragShaderString.split("\n");
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
	showError->setWindowTitle("Error in Fragment Shader");
	showError->setSizeGripEnabled(true);
	showError->setModal(true);
	showError->setLayout(layout);
	showError->exec();
	//-----------------------------------
	
	return false;
      }
  }

  
  //----------- link program shader ----------------------
  GLint linked = -1;
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


