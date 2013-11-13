#include "staticfunctions.h"
#include "shaderfactory.h"
#include "cropshaderfactory.h"
#include "tearshaderfactory.h"
#include "blendshaderfactory.h"
#include "glowshaderfactory.h"
#include "pathshaderfactory.h"
#include "global.h"
#include "prunehandler.h"

QString
ShaderFactory::tagVolume()
{
  // use buffer values to apply tag colors
  QString shader;
  shader += "  float ptx = prunefeather.z;\n";
  shader += "  vec4 paintColor = texture1D(paintTex, ptx);\n";

  //  shader += "  gl_FragColor = vec4(paintColor.a,0.1,0.1,0.1);\n";

  shader += "  ptx *= 255.0;\n";
  shader += "  if (ptx > 0.0) \n";
  shader += "  {\n";
  shader += "    paintColor.rgb *= gl_FragColor.a;\n";
 // if paintColor is black then change only the transparency
  shader += "    if (paintColor.r+paintColor.g+paintColor.b > 0.01)\n";
  shader += "      gl_FragColor.rgb = mix(gl_FragColor.rgb, paintColor.rgb, paintColor.a);\n";
  shader += "    else\n";
  shader += "      gl_FragColor *= paintColor.a;\n";
  shader += "  }\n";

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
    qstr = "varying vec3 pointpos;\n";
    qstr += "void main(void)\n";
    qstr += "{\n";
    qstr += "  // Transform vertex position into homogenous clip-space.\n";
    qstr += "  gl_FrontColor = gl_Color;\n";
    qstr += "  gl_BackColor = gl_Color;\n";
    qstr += "  gl_Position = ftransform();\n";
    qstr += "  gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n";
    qstr += "  gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;\n";
    qstr += "  gl_TexCoord[2] = gl_TextureMatrix[2] * gl_MultiTexCoord2;\n";
    qstr += "  gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n";
    qstr += "  pointpos = gl_Vertex.xyz;\n";
    qstr += "}\n";
    
    int len = qstr.length();
    char *tbuffer = new char[len+1];
    sprintf(tbuffer, qstr.toAscii().data());
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
    sprintf(tbuffer, shaderString.toAscii().data());
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
	
	qApp->beep();
	
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
	      
	      shader += QString("  vec2 vol%1 = vec2(vg.%2, tfSet+%3);\n"). \
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

  shader += "  vec3 lightcol = vec3(1.0,1.0,1.0);\n";
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
  shader += "  Amb = ambient*gl_FragColor.rgb;\n";
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
      shader += "  if (prunefeather.x < 0.005) discard;\n";

      // delta condition added for reslicing/ option
      //shader += "  if (delta.x < 1.0 && prunefeather.x < 0.005) discard;\n";

      // tag values should be non interpolated - nearest neighbour
      shader += "  prunefeather.z = texture2DRect(pruneTex, vec2(floor(pvg0.x)+0.5,floor(pvg0.y)+0.5)).z;\n";
    }

  shader += "  t0 = getTextureCoordinate(slice, gridx, tsizex, tsizey, texCoord.xy);\n";
  shader += "  t1 = getTextureCoordinate(slice+1, gridx, tsizex, tsizey, texCoord.xy);\n";
  shader += "  val0 = (texture2DRect(dataTex, t0)).x;\n";
  shader += "  val1 = (texture2DRect(dataTex, t1)).x;\n";
  shader += "  vg.x = mix(val0, val1, slicef);\n";

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

  shader += "uniform vec3 brickMin;\n";
  shader += "uniform vec3 brickMax;\n";

  shader += genTextureCoordinate();

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (glowPresent) shader += GlowShaderFactory::generateGlow(crops);  
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();
  if (pathViewPresent) shader += PathShaderFactory::applyPathBlend();

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

//----------------------------------------------------------
// Manipulate z-coordinate for non linear depth levels
//  int levels = 4;
//  float origT[10], newT[10];
//  origT[0] = 00.00;  newT[0] = 00.0;
//  origT[1] = 01.20;  newT[1] = 11.0;
//  origT[2] = 04.00;  newT[2] = 23.0;
//  origT[3] = 37.62;  newT[3] = 56.0;
//  origT[4] = 62.00;  newT[4] = 62.0;
//
//  for(int o=0; o<levels; o++)
//    {
//      if (o > 0)
//	shader += "else ";
//      shader += QString("if (texCoord.z >= float(%1) && texCoord.z < float(%2))\n").\
//	arg(origT[o]).\
//	arg(origT[o+1]);
//      shader += "  {\n"; 
//
//      shader += QString("    float tz = (texCoord.z-float(%1))/(float(%2)-float(%1));\n").\
//	arg(origT[o]).							\
//	arg(origT[o+1]);
//
//      shader += QString("    texCoord.z = float(%1) + tz * (float(%2)-float(%1));\n").\
//	arg(newT[o]).\
//	arg(newT[o+1]);
//      shader += "  }\n";
//    }
//----------------------------------------------------------
  
  shader += "texCoord.z = 1.0 + (texCoord.z-float(tminz))/float(lod);\n";

  shader += genVgx();

  if (peel || lighting || !Global::use1D())
    shader += getNormal();


  if (bit16)
    {
      shader += "int h0 = int(65535.0*vg.x);\n";
      shader += "int h1 = h0 / 256;\n";
      shader += "h0 = int(mod(float(h0),256.0));\n";
      shader += "float fh0 = float(h0)/256.0;\n";
      shader += "float fh1 = float(h1)/256.0;\n";

      shader += QString("vg.xy = vec2(fh0, fh1*%1);\n").arg(1.0/Global::lutSize());
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

  if (viewPresent) shader += "  blend(otexCoord, vg, gl_FragColor);\n";
  
  if (pathViewPresent) shader += "pathblend(otexCoord, vg, gl_FragColor);\n";
  

//---------------------------------
  shader += "if (delta.x > 1.0)\n";
  shader += "  { gl_FragColor = vec4(vg.x, gl_FragColor.a, vg.x, 1.0); return; }\n";
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
  shader += "  gl_FragColor.rgb *= depthcue;\n";

  if (glowPresent) shader += "  gl_FragColor.rgb += glow(otexCoord);\n";

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";

  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genHighQualitySliceShaderString(bool bit16,
					       bool lighting,
					       bool emissive,
					       bool shadows,
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

  shader += "uniform vec3 brickMin;\n";
  shader += "uniform vec3 brickMax;\n";

  shader += genTextureCoordinate();

  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (glowPresent) shader += GlowShaderFactory::generateGlow(crops);  
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();
  if (pathViewPresent) shader += PathShaderFactory::applyPathBlend();

  shader += "void main(void)\n";
  shader += "{\n";

  //------------------------
  if (shadows && peel && peelType == 2)
    {
      float peelfeather = qBound(0.0f, (peelMax+1.0f)*0.5f, 1.0f);
      shader += "  float peelfeather = texture2DRect(shadowTex, gl_TexCoord[1].xy).a;\n";
      if (peelMix < 0.001)
	{
	  shader += QString("  if (peelfeather < float(%1)) discard;\n").arg(peelfeather);
	  shader += QString("  peelfeather = smoothstep(float(%1), peelfeather, float(%2));\n"). \
	    arg(peelfeather).arg(peelfeather+0.2);
	}
      else
	{
	  shader += QString("    peelfeather = smoothstep(float(%1), peelfeather, float(%2));\n"). \
	    arg(peelfeather).arg(peelfeather+0.2);
	  shader += QString("    peelfeather = max((1.0-peelfeather)*float(%1), peelfeather);\n"). \
	    arg(peelMix);
	}
	
    }
  //------------------------

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
  shader += "texCoord.z = 1.0 + (float(texCoord.z)-float(tminz))/float(lod);\n";

  shader += genVgx();


  if (peel || lighting || !Global::use1D())
    shader += getNormal();

  if (bit16)
    {
      shader += "int h0 = int(65535.0*vg.x);\n";
      shader += "int h1 = h0 / 256;\n";
      shader += "h0 = int(mod(float(h0),256.0));\n";
      shader += "float fh0 = float(h0)/256.0;\n";
      shader += "float fh1 = float(h1)/256.0;\n";

      shader += QString("vg.xy = vec2(fh0, fh1*%1);\n").arg(1.0/Global::lutSize());
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
  shader += "\n";


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

  if (shadows && peel && peelType == 2)
    {
      shader += QString("  vg1.y += float(%1);\n").arg(lastSet);
      shader += "  gl_FragColor = mix(texture2D(lutTex, vg1.xy), gl_FragColor, peelfeather);\n";
    }
  
  if (viewPresent) shader += "blend(otexCoord, vg, gl_FragColor);\n";
  if (pathViewPresent) shader += "pathblend(otexCoord, vg, gl_FragColor);\n";

//------------------------------------
  shader += "gl_FragColor = 1.0-pow((vec4(1,1,1,1)-gl_FragColor),";
  shader += "vec4(lod,lod,lod,lod));\n";
//------------------------------------

  shader += "  if (gl_FragColor.a < 0.005)\n";
  shader += "    discard;\n";
  shader += "\n";

  if (lighting)
    shader += addLighting();

  shader += genPeelShader(peel, peelType,
			  peelMin, peelMax, peelMix,
			  lighting);

  if (glowPresent) shader += "  vec3 glowColor = glow(otexCoord);\n";

  if (shadows)
    {
      //----------------------
      // shadows
      shader += "  vec4 incident_light = texture2DRect(shadowTex, gl_TexCoord[1].xy);\n";
      if (glowPresent)
	{
	  shader += "  float glowMix = max(glowColor.r, max(glowColor.g, glowColor.b));\n";
	  shader += "  incident_light.rgb *= (1.0 - glowColor);\n";
	  shader += "  incident_light.a *= (1.0 - glowMix);\n";
	}
      shader += "  float maxval = 1.0 - incident_light.a;\n";
      shader += "  gl_FragColor.rgb *= maxval*(incident_light.rgb + maxval);\n";
      //----------------------
    }

  if (emissive)
    {
      //----------------------
      // emissive lighting
      shader += QString("  vg1.y += float(%1);\n").arg(lastSet);
      shader += "  gl_FragColor.rgb += texture2D(lutTex, vg1.xy).rgb;\n";
      //----------------------
    }

  // -- depth cueing
  shader += "  gl_FragColor.rgb *= depthcue;\n";

  if (glowPresent) shader += "  gl_FragColor.rgb += glowColor;\n";

  shader += "  gl_FragColor = clamp(gl_FragColor, vec4(0.0,0.0,0.0,0.0), vec4(1.0,1.0,1.0,1.0));\n";

  shader += "}\n";

  return shader;
}
 
QString
ShaderFactory::genSliceShadowShaderString(bool bit16,
					  float shadowintensity,
					  float r, float g, float b,
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

  shader += "uniform vec3 eyepos;\n";
  shader += "uniform float tfSet;\n";
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

  shader += "uniform vec3 brickMin;\n";
  shader += "uniform vec3 brickMax;\n";

  shader += genTextureCoordinate();
  
  if (tearPresent) shader += TearShaderFactory::generateTear(crops);
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (glowPresent) shader += GlowShaderFactory::generateGlow(crops);  
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops);
  if (pathCropPresent) shader += PathShaderFactory::applyPathCrop();
  if (pathViewPresent) shader += PathShaderFactory::applyPathBlend();

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

  if (glowPresent)
    {
      shader += "  vec3 glowColor = glow(otexCoord);\n";
      shader += "  float glowMix = max(glowColor.r, max(glowColor.g, glowColor.b));\n";
      shader += "  if (glowMix > 0.05) discard;";
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
  shader += "texCoord.z = 1.0 + (float(texCoord.z)-float(tminz))/float(lod);\n";

  shader += genVgx();

  shader += getNormal();

  if (bit16)
    {
      shader += "int h0 = int(65535.0*vg.x);\n";
      shader += "int h1 = h0 / 256;\n";
      shader += "h0 = int(mod(float(h0),256.0));\n";
      shader += "float fh0 = float(h0)/256.0;\n";
      shader += "float fh1 = float(h1)/256.0;\n";

      shader += QString("vg.xy = vec2(fh0, fh1*%1 + tfSet);\n").arg(1.0/Global::lutSize());
    }
  else
    {
      if (Global::use1D())
	shader += "  vg.y = tfSet;\n";
      else
	{
	  shader += QString("  vg.y = grad*%1;\n").arg(1.0/Global::lutSize());
	  
	  shader += "  vg.y += tfSet;\n";
	}
    }

  shader += "  gl_FragColor = texture2D(lutTex, vg.xy);\n";

  shader += genPeelShader(peel, peelType,
			  peelMin, peelMax, peelMix,
			  false);
  
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

  if (peel && peelType==2)
    {
      float reduceShadow = 1.0f - qBound(0.0f, (peelMin+1.0f)*0.5f, 1.0f);
      shader += QString("  gl_FragColor.rgba = mix(gl_FragColor.rgba, vec4(0.0,0.0,0.0,0.0), float(%1));\n").\
	        arg(reduceShadow);
    }

  if (viewPresent) shader += "blend(otexCoord, vg, gl_FragColor);\n";
  if (pathViewPresent) shader += "pathblend(otexCoord, vg, gl_FragColor);\n";

//------------------------------------
  shader += "gl_FragColor = 1.0-pow((vec4(1,1,1,1)-gl_FragColor),";
  shader += "vec4(lod,lod,lod,lod));\n";
//------------------------------------

  // --- modulate shadow by homogenity
  shader += QString("  gl_FragColor.rgba *= 1.0-smoothstep(0.0, float(%1), grad);\n").arg(shadowintensity);

  shader += "  if (gl_FragColor.a < 0.01)\n";
  shader += "	discard;\n";
  shader += "\n";
  shader += QString("  gl_FragColor.rgba *= vec4(%1, %2, %3, %4);\n").\
                                        arg(r).arg(g).arg(b).arg(maxrgb);

//  shader += "  gl_FragColor.r = clamp(gl_FragColor.r, 0.0, 1.0);\n";
//  shader += "  gl_FragColor.g = clamp(gl_FragColor.g, 0.0, 1.0);\n";
//  shader += "  gl_FragColor.b = clamp(gl_FragColor.b, 0.0, 1.0);\n";
//  shader += "  gl_FragColor.a = clamp(gl_FragColor.a, 0.0, 1.0);\n";

  shader += "}\n";

  return shader;
}

