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
  shader += "  vec4 paintColor = texture1D(paintTex, ptx);\n";

  //  shader += "  gl_FragColor = vec4(paintColor.a,0.1,0.1,0.1);\n";

  shader += "  ptx *= 255.0;\n";

  shader += "  if (ptx > 0.0 || mixTag) \n";
  shader += "  {\n";
  shader += "    paintColor.rgb *= gl_FragColor.a;\n";
 // if paintColor is black then change only the transparency
  shader += "    if (paintColor.r+paintColor.g+paintColor.b > 0.01)\n";
  shader += "      gl_FragColor.rgb = mix(gl_FragColor.rgb, paintColor.rgb, paintColor.a);\n";
  shader += "    else\n";
  shader += "      gl_FragColor *= paintColor.a;\n";
  shader += "  }\n";
  shader += "  else\n";
  shader += "    gl_FragColor *= paintColor.a;\n";

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
  shader += "    gl_FragColor = texture2D(lutTex, vgc);\n";
  shader += "  }\n";

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
    qstr += "varying vec3 pointpos;\n";
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
ShaderFactory::getNormal()
{
  QString shader;

  shader += "  vec3 sample1, sample2;\n";
  shader += "  float grad;\n";
  shader += "  vec3 normal;\n";

  shader += "  float tv;\n";
  shader += "  sample1.x = texture2DRect(dataTex, t0+vec2(1,0)).x;\n";
  shader += "         tv = texture2DRect(dataTex, t1+vec2(1,0)).x;\n";
  shader += "  sample1.x = mix(sample1.x, tv, slicef);\n";

  shader += "  sample2.x = texture2DRect(dataTex, t0-vec2(1,0)).x;\n";
  shader += "         tv = texture2DRect(dataTex, t1-vec2(1,0)).x;\n";
  shader += "  sample2.x = mix(sample2.x, tv, slicef);\n";

  shader += "  sample1.y = texture2DRect(dataTex, t0+vec2(0,1)).x;\n";
  shader += "         tv = texture2DRect(dataTex, t1+vec2(0,1)).x;\n";
  shader += "  sample1.y = mix(sample1.y, tv, slicef);\n";

  shader += "  sample2.y = texture2DRect(dataTex, t0-vec2(0,1)).x;\n";
  shader += "         tv = texture2DRect(dataTex, t1-vec2(0,1)).x;\n";
  shader += "  sample2.y = mix(sample2.y, tv, slicef);\n";

  shader += "  t1 = getTextureCoordinate(slice+2, gridx, tsizex, tsizey, texCoord.xy);";
  shader += "  tv = (texture2DRect(dataTex, t1)).x;\n";
  shader += "  sample1.z = mix(val1, tv, slicef);\n";

  shader += "  t1 = getTextureCoordinate(slice-1, gridx, tsizex, tsizey, texCoord.xy);";
  shader += "  sample2.z = (texture2DRect(dataTex, t1)).x;\n";
  shader += "  sample2.z = mix(sample2.z, val0, slicef);\n";

  shader += "  grad = clamp(distance(sample1, sample2), 0.0, 0.996);\n";

  return shader;
}

QString
ShaderFactory::addLighting()
{
  QString shader;

  // grad, sample1, sample2 defined in getNormal()
  shader += "  normal = normalize(sample1 - sample2);\n";
  shader += "  normal = mix(vec3(0.0,0.0,0.0), normal, step(0.0, grad));"; 

  shader += "  vec3 voxpos = pointpos;\n";
  shader += "  vec3 I = voxpos - eyepos;\n";
  shader += "  vec3 lightvec = voxpos - lightpos;\n";
  shader += "  I = normalize(I);\n";
  shader += "  lightvec = normalize(lightvec);\n";

  shader += "  vec3 reflecvec = reflect(lightvec, normal);\n";
  shader += "  float DiffMag = abs(dot(normal, lightvec));\n";
  shader += "  vec3 Diff = (diffuse*DiffMag)*gl_FragColor.rgb;\n";
  shader += "  float Spec = pow(abs(dot(normal, reflecvec)), speccoeff);\n";
  shader += "  Spec *= specular*gl_FragColor.a;\n";
  shader += "  vec3 Amb = ambient*gl_FragColor.rgb;\n";
  shader += "  float litfrac;\n";
  shader += "  litfrac = smoothstep(0.05, 0.1, grad);\n";
  shader += "  gl_FragColor.rgb = mix(gl_FragColor.rgb, Amb, litfrac);\n";
  shader += "  if (litfrac > 0.0)\n";
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
      shader += "  gl_FragColor.rgba *= IdotN;\n";
    }

  return shader;
}

QString
ShaderFactory::genVgx()
{
  QString shader;

  shader += "  vec2 vg, vg1;\n";
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
      shader += "  prunefeather.z = texture2DRect(pruneTex, vec2(floor(pvg0.x)+0.5,floor(pvg0.y)+0.5)).z;\n";
    }
  //---------------------------------------------------------------------

  shader += "  t0 = getTextureCoordinate(slice, gridx, tsizex, tsizey, texCoord.xy);\n";
  shader += "  if (linearInterpolation && !mixTag)\n";
  shader += "   {\n";
  shader += "     val0 = (texture2DRect(dataTex, t0)).x;\n";
  shader += "     t1 = getTextureCoordinate(slice+1, gridx, tsizex, tsizey, texCoord.xy);\n";
  shader += "     val1 = (texture2DRect(dataTex, t1)).x;\n";
  shader += "     vg.x = mix(val0, val1, slicef);\n";
  shader += "   }\n";
  shader += "   else\n"; // nearest neighbour interpolation
  shader += "   {\n";
  shader += "     val0 = (texture2DRect(dataTex, floor(t0)+vec2(0.5))).x;\n";
  shader += "     vg.x = val0;\n";
  shader += "   }\n";


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

  shader += "uniform bool mixTag;\n";

  shader += "uniform vec3 brickMin;\n";
  shader += "uniform vec3 brickMax;\n";

  shader += "uniform int shdlod;\n";
  shader += "uniform sampler2DRect shdTex;\n";
  shader += "uniform float shdIntensity;\n";

  shader += "uniform float opmod;\n";
  shader += "uniform bool linearInterpolation;\n";

  shader += "uniform float dofscale;\n";

  shader += genTextureCoordinate();

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (glowPresent) shader += GlowShaderFactory::generateGlow(crops);  
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();
  if (pathViewPresent) shader += PathShaderFactory::applyPathBlend();

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec3 lightcol = vec3(1.0,1.0,1.0);\n";

  shader += "  vec3 texCoord = gl_TexCoord[0].xyz;\n";

  shader += "if (any(lessThan(texCoord,brickMin)) || ";
  shader += "any(greaterThan(texCoord, brickMax)))\n";
  //shader += "    if (any(lessThan(texCoord,brickMin-vec3(0.5,0.5,0.5))) || ";
  //shader += "    any(greaterThan(texCoord, brickMax+vec3(0.5,0.5,0.5))))\n";
  //shader += "    if (any(lessThan(texCoord,brickMin-vec3(1,1,1))) || ";
  //shader += "    any(greaterThan(texCoord, brickMax+vec3(1,1,1))))\n";
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

  shader += genVgx();

  shader += "float value = vg.x;\n";

  //----------------------------------
  // this is specifically for nearest neighbour interpolating when upscaling
  // or volume and surface area calculations
  shader += "  if (depthcue > 1.0) vg.x = val0;\n";
  //----------------------------------

  //----------------------------------
  //------------------------------------
  shader += "if (lightlod > 0)\n";
  shader += "  {\n"; // calculate light color
  shader += "    vec2 pvg = texCoord.xy/(prunelod*float(lightlod));\n";	       
  shader += "    int lbZslc = int(float(zoffset+slice)/(prunelod*float(lightlod)));\n";
  shader += "    float lbZslcf = fract(float(zoffset+slice)/(prunelod*float(lightlod)));\n";
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
    shader += getNormal();


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
  shader += "  gl_FragColor = texture2D(lutTex, vg.xy);\n";


  if (Global::emptySpaceSkip())
    {
      shader += "  gl_FragColor.rgba = mix(vec4(0.0,0.0,0.0,0.0), gl_FragColor.rgba, prunefeather.x);\n";
      if (PruneHandler::blend())
	shader += blendVolume();
      else
	shader += tagVolume();
    }


  if (tearPresent || cropPresent || pathCropPresent)
    shader += "  gl_FragColor.rgba = mix(gl_FragColor.rgba, vec4(0.0,0.0,0.0,0.0), feather);\n";

  if (viewPresent) shader += "  blend(false, otexCoord, vg, gl_FragColor);\n";
  
  if (pathViewPresent) shader += "pathblend(otexCoord, vg, gl_FragColor);\n";
  

//---------------------------------
  if (Global::emptySpaceSkip())
    {
      shader += "if (delta.x > 1.0)\n";
      shader += "  { gl_FragColor = vec4(value*step(0.001,gl_FragColor.a),";
      shader += "gl_FragColor.a, prunefeather.z, 1.0); return; }\n";
    }
  else
    {
      shader += "if (delta.x > 1.0)\n";
      shader += "  { gl_FragColor = vec4(value*step(0.001,gl_FragColor.a),";
      shader += "gl_FragColor.a, 0.0, 1.0); return; }\n";
    }
//---------------------------------

//------------------------------------
  shader += "gl_FragColor = 1.0-pow((vec4(1,1,1,1)-gl_FragColor),";
  shader += "vec4(lod,lod,lod,lod));\n";
//------------------------------------

  shader += "\n";
  shader += "  if (gl_FragColor.a < 0.005)\n";
  shader += "	discard;\n";

  if (lighting)
    shader += addLighting();

  shader += genPeelShader(peel, peelType,
			  peelMin, peelMax, peelMix,
			  lighting);

  if (emissive)
    {
      shader += QString("  vg1.y += float(%1);\n").arg(lastSet);
      shader += "  gl_FragColor.rgb += texture2D(lutTex, vg1.xy).rgb;\n";
    }

  // -- depth cueing
  shader += "  gl_FragColor.rgb *= min(1.0,depthcue);\n";

  shader += "  gl_FragColor *= opmod;\n";

  if (glowPresent) shader += "  gl_FragColor.rgb += glow(otexCoord);\n";

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";

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
	m_meshShaderParm[2] = glGetUniformLocation(m_meshShader, "pn");
	m_meshShaderParm[3] = glGetUniformLocation(m_meshShader, "pnear");
	m_meshShaderParm[4] = glGetUniformLocation(m_meshShader, "pfar");
	m_meshShaderParm[5] = glGetUniformLocation(m_meshShader, "opacity");
	m_meshShaderParm[6] = glGetUniformLocation(m_meshShader, "ambient");
	m_meshShaderParm[7] = glGetUniformLocation(m_meshShader, "diffuse");
	m_meshShaderParm[8] = glGetUniformLocation(m_meshShader, "specular");
	m_meshShaderParm[9] = glGetUniformLocation(m_meshShader, "nclip");
	m_meshShaderParm[10] = glGetUniformLocation(m_meshShader,"clipPos");
	m_meshShaderParm[11] = glGetUniformLocation(m_meshShader,"clipNormal");

    }

  return m_meshShader;
}


QString
ShaderFactory::meshShaderV()
{
  QString shader;

  shader += "#version 410\n";
  shader += "uniform mat4 MVP;\n";
  shader += "layout(location = 0) in vec3 position;\n";
  shader += "layout(location = 1) in vec3 normalIn;\n";
  shader += "layout(location = 2) in vec3 colorIn;\n";
  shader += "out vec3 v3Normal;\n";
  shader += "out vec3 v3Color;\n";
  shader += "out vec3 pointPos;\n";
  shader += "void main()\n";
  shader += "{\n";
  shader += "   pointPos = position;\n";
  shader += "   v3Color = colorIn;\n";
  shader += "   v3Normal = normalIn;\n";
  shader += "   gl_Position = MVP * vec4(position, 1);\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::meshShaderF()
{
  QString shader;

  shader += "#version 410 core\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform vec3 pn;\n";
  shader += "uniform float pnear;\n";
  shader += "uniform float pfar;\n";
  shader += "uniform float opacity;\n";
  shader += "uniform float ambient;\n";
  shader += "uniform float diffuse;\n";
  shader += "uniform float specular;\n";
  shader += "uniform int nclip;\n";
  shader += "uniform vec3 clipPos[10];\n";
  shader += "uniform vec3 clipNormal[10];\n";
  shader += "\n";
  shader += "in vec3 v3Color;\n";
  shader += "in vec3 v3Normal;\n";
  shader += "in vec3 pointPos;\n";
  shader += "out vec4 outputColor;\n";
  shader += "void main()\n";
  shader += "{\n";
  shader += "   float d = dot(pn, pointPos);\n";
  shader += "   if (pnear < pfar && (d < pnear || d > pfar))\n";
  shader += "     discard;\n";

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
  shader += "          cfeather *= smoothstep(0.0, 3.0, -cp);\n";
  shader += "      }\n";
  shader += "    cfeather = 1.0 - cfeather;\n";
  shader += "  }\n";

  shader += "  outputColor = vec4(v3Color, 1)*opacity;\n";

  shader += "  if (nclip > 0)\n";
  shader += "    outputColor.rgb = mix(outputColor.rgb, vec3(1,1,1), cfeather);\n";

  shader += "\n";
  shader += "  if (length(viewDir) > 0.0)\n";
  shader += "    {\n";
  shader += "      vec3 reflecvec = reflect(viewDir, v3Normal);\n";
  shader += "      float diffMag = abs(dot(v3Normal, viewDir));\n";
  shader += "      vec3 Diff = diffuse*diffMag*outputColor.rgb;\n";
  shader += "      float Spec = pow(abs(dot(v3Normal, reflecvec)), 512);\n";
  shader += "      Spec *= specular*outputColor.a;\n";
  shader += "      vec3 Amb = ambient*outputColor.rgb;\n";
  shader += "\n";
  shader += "      outputColor.rgb = Amb + Diff + vec3(Spec,Spec,Spec);\n";
  shader += "    }\n";
  shader += "}\n";

  return shader;
}

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
