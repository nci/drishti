#include "shaderfactory.h"

#include <QTextEdit>
#include <QVBoxLayout>
#include <QMessageBox>


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
ShaderFactory::genSliceShader(bool bit16)
{
  QString shader;

  shader =  "#version 420 core\n";
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler3D maskTex;\n";
  shader += "uniform sampler1D tagTex;\n";
  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  if (any(greaterThan(gl_TexCoord[0].xyz,vec3(1.0,1.0,1.0)))) \n";
  shader += "    discard;\n";
  shader += "  if (any(lessThan(gl_TexCoord[0].xyz,vec3(0.0,0.0,0.0)))) \n";
  shader += "    discard;\n";

  shader += "  float val = texture(dataTex, gl_TexCoord[0].xyz).x;\n";

  shader += "  vec4 color;\n";

  if (!bit16)
    shader += "  color = texture(lutTex, vec2(val,0.0));\n";
  else
    {
      shader += "  int h0 = int(65535.0*val);\n";
      shader += "  int h1 = h0 / 256;\n";
      shader += "  h0 = int(mod(float(h0),256.0));\n";
      shader += "  float fh0 = float(h0)/256.0;\n";
      shader += "  float fh1 = float(h1)/256.0;\n";
      shader += "  color = texture(lutTex, vec2(fh0,fh1));\n";
    }

  shader += "  if (color.a < 0.001) discard;\n";

  shader += "  float tag = texture(maskTex, gl_TexCoord[0].xyz).x;\n";
  shader += "  vec4 tagcolor = texture(tagTex, tag);\n";
  shader += "  if (tag < 0.001) tagcolor.rgb = color.rgb;\n";
  
  shader += "  color.rgb = mix(color.rgb, tagcolor.rgb, 0.8);\n";
  
  // so that we can use tag opacity to hide certain tagged regions
  // tagcolor.a should either 0 or 1
  shader += "  color *= tagcolor.a;\n";

  shader += "  color.rgb *= color.a;\n";

  shader += "  gl_FragColor = color;\n";    

  shader += "}\n";

  return shader;
}


QString
ShaderFactory::genShadowSliceShader()
{
  QString shader;

  shader =  "#version 420 core\n";
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect sliceTex;\n";
  shader += "uniform sampler2DRect shadowTex;\n";
  shader += "uniform float darkness;\n";

  shader += "\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec2 spos = gl_FragCoord.xy;\n";
  shader += "  float alpha = texture2DRect(shadowTex, spos.xy).a;\n";  
  shader += "  alpha = min(1.0, alpha*darkness);\n";  
  shader += "  gl_FragColor = texture2DRect(sliceTex, spos.xy);\n";
  shader += "  gl_FragColor.rgb *= (1.0-alpha);\n";  
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genShadowBlurShader()
{
  QString shader;

  shader =  "#version 420 core\n";
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect sliceTex;\n";
  shader += "uniform sampler2DRect shadowTex;\n";
  shader += "uniform float rot;\n";
  shader += "uniform float stp;\n";

  shader += "\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 color;\n";
  shader += "  vec2 spos = gl_FragCoord.xy;\n";
  shader += "\n";

  shader += "  float a = 0.75 + 0.75*stp;\n";
  shader += "  float b = 0.25 + 0.25*stp;\n";
  shader += "  color  = texture2DRect(shadowTex, spos.xy);\n";
  shader += "  color += texture2DRect(shadowTex, spos.xy + vec2(rot,0.0)*vec2( a, b));\n";
  shader += "  color += texture2DRect(shadowTex, spos.xy + vec2(rot,0.0)*vec2( b,-a));\n";
  shader += "  color += texture2DRect(shadowTex, spos.xy + vec2(rot,0.0)*vec2(-a,-b));\n";
  shader += "  color += texture2DRect(shadowTex, spos.xy + vec2(rot,0.0)*vec2(-b, a));\n";

  shader += "  color /= 5.0;\n";

  shader += "  vec4 slicecolor = texture2DRect(sliceTex, spos.xy);\n";

  shader += "  gl_FragColor.rgba = color + (1.0-color.a)*slicecolor;\n";

  shader += "}\n";

  return shader;
}


QString
ShaderFactory::genDepthShader()
{
  QString shader;

  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointpos;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "void main(void)\n";
  shader += "{\n";

  shader += "  vec3 voxpos = pointpos;\n";
  shader += "  vec3 I = voxpos - eyepos;\n";
  shader += "  float z = dot(I, viewDir);\n";
  shader += "  z = (z-minZ)/(maxZ-minZ);\n";
  shader += "  z = clamp(z, 0.0, 1.0);\n";

  shader += " vec2 rdxy = 2.0*(gl_TexCoord[0].xy-vec2(0.5,0.5));\n";
  shader += " rdxy *= rdxy;\n";
  shader += " float rd = sqrt(rdxy.x + rdxy.y);\n";
  shader += " rd = clamp(rd, 0.0, 1.0);\n";
  
  shader += " gl_FragColor = vec4(z,z,z,smoothstep(rd, 0.9, 0.99));\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::genRectBlurShaderString(int filter)
{
  QString shader;

  shader =  "#version 420 core\n";
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2DRect blurTex;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";
  shader += "\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  vec4 color;\n";
  shader += "  vec2 spos = gl_FragCoord.xy;\n";
  shader += "\n";
  
  if (filter == 1) // bilateral filter
    {
      shader += "  color = texture2DRect(blurTex, spos.xy);\n";
      shader += "  gl_FragColor = color;\n";
      shader += "  if (color.a < 0.01) return;\n";

      float cx[8] = {-1.0,-1.0,-1.0, 0.0, 0.0, 1.0, 1.0, 1.0};
      float cy[8] = {-1.0, 0.0, 1.0,-1.0, 1.0,-1.0, 0.0, 1.0};
      shader += "  float cx[8];\n";
      shader += "  float cy[8];\n";
      for(int i=0; i<8; i++)
	shader += QString("  cx[%1] = float(%2);\n").arg(i).arg(cx[i]);
      for(int i=0; i<8; i++)
	shader += QString("  cy[%1] = float(%2);\n").arg(i).arg(cy[i]);

      shader += "  float depth = color.x;\n";
      shader += "  float odepth = minZ + depth*(maxZ-minZ);\n";
      shader += "  float sum = 1.0;\n";
      shader += "  for(int i=0; i<8; i++)\n";
      shader += "  {\n";
      shader += "    float z = texture2DRect(blurTex, spos.xy + vec2(cx[i],cy[i])).x;\n";
      shader += "    float oz = minZ + z*(maxZ-minZ);\n";
      shader += "    float fi = (odepth-oz)*0.2;\n";
      shader += "    fi = exp(-fi*fi);\n";
      shader += "    depth += z*fi;\n";
      shader += "    sum += fi;\n";
      shader += "  }\n";

      shader += "  gl_FragColor.rgba = vec4(depth/sum, color.gb, color.a);\n";
    }
  else if (filter == 2)
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
  else if (filter == 3)
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
ShaderFactory::getGrad(bool nearest)
{
  QString shader;

  shader += " vec3 gx, gy, gz;\n";
  shader += " gx = vec3(1.0/vsize.x,0,0);\n";
  shader += " gy = vec3(0,1.0/vsize.y,0);\n";
  shader += " gz = vec3(0,0,1.0/vsize.z);\n";
  if (nearest)
    {
      shader += " float vx = texture(dataTex, vC+gx).x - texture(dataTex, vC-gx).x;\n";
      shader += " float vy = texture(dataTex, vC+gy).x - texture(dataTex, vC-gy).x;\n";
      shader += " float vz = texture(dataTex, vC+gz).x - texture(dataTex, vC-gz).x;\n";
    }
  else
    {
      shader += " float vx = texture(dataTex, voxelCoord+gx).x - texture(dataTex, voxelCoord-gx).x;\n";
      shader += " float vy = texture(dataTex, voxelCoord+gy).x - texture(dataTex, voxelCoord-gy).x;\n";
      shader += " float vz = texture(dataTex, voxelCoord+gz).x - texture(dataTex, voxelCoord-gz).x;\n";
    }
  shader += " vec3 grad = vec3(vx, vy, vz);\n";

  shader += " float gradMag = length(grad);\n";
  shader += " gradMag = clamp(gradMag, 0.0, 1.0);\n";
  
  return shader;
}

QString
ShaderFactory::getGrad2(bool nearest)
{
  QString shader;

  shader += " float a[3] = float[](-1.0/vsize.x,0.0,1.0/vsize.x);\n";
  shader += " float b[3] = float[](-1.0/vsize.y,0.0,1.0/vsize.y);\n";
  shader += " float c[3] = float[](-1.0/vsize.z,0.0,1.0/vsize.z);\n";
  shader += " float sum = 0.0;\n";
  shader += " for(int i=0; i<=2; i++)\n";
  shader += " for(int j=0; j<=2; j++)\n";
  shader += " for(int k=0; k<=2; k++)\n";
  shader += " {\n";
  shader += "   vec3 g = vec3(a[i],b[j],c[k]);\n";

  if (nearest)
    shader += "   sum += texture(dataTex, vC+g).x;\n";
  else
    shader += "   sum += texture(dataTex, voxelCoord+g).x;\n";

  shader += " }\n";
  shader += " sum = (sum-val)/10.0;\n";
  shader += " float gradMag = abs(sum-val);\n";
  shader += " gradMag = clamp(gradMag, 0.0, 1.0);\n";
  
  return shader;
}

QString
ShaderFactory::getGrad3(bool nearest)
{
  QString shader;

  shader += " float a[5] = float[](-2.0/vsize.x,-1.0/vsize.x,0.0,1.0/vsize.x,2.0/vsize.x);\n";
  shader += " float b[5] = float[](-2.0/vsize.y,-1.0/vsize.y,0.0,1.0/vsize.y,2.0/vsize.y);\n";
  shader += " float c[5] = float[](-2.0/vsize.z,-1.0/vsize.z,0.0,1.0/vsize.z,2.0/vsize.z);\n";
  shader += " float sum = 0.0;\n";
  shader += " for(int i=0; i<=4; i++)\n";
  shader += " for(int j=0; j<=4; j++)\n";
  shader += " for(int k=0; k<=4; k++)\n";
  shader += " {\n";
  shader += "   vec3 g = vec3(a[i],b[j],c[k]);\n";

  if (nearest)
    shader += "   sum += texture(dataTex, vC+g).x;\n";
  else
    shader += "   sum += texture(dataTex, voxelCoord+g).x;\n";

  shader += " }\n";
  shader += " sum = (sum-val)/70.0;\n";
  shader += " float gradMag = abs(sum-val);\n";
  shader += " gradMag = clamp(gradMag, 0.0, 1.0);\n";
  
  return shader;
}

QString
ShaderFactory::addLighting()
{
  QString shader;

  shader += " if (length(grad) > 0.1)\n";
  shader += "  {\n";
  shader += "    grad = normalize(grad);\n";
  shader += "    vec3 lightVec = viewDir;\n";
  shader += "    float diff = abs(dot(lightVec, grad));\n";
  shader += "    vec3 reflecvec = reflect(lightVec, grad);\n";
  shader += "    float spec = pow(abs(dot(grad, reflecvec)), 1024.0);\n";
  shader += "    colorSample.rgb *= (0.6 + 0.4*diff + spec);\n";
  shader += "    if (any(greaterThan(colorSample.rgb,vec3(1.0,1.0,1.0)))) \n";
  shader += "      colorSample.rgb = vec3(1.0,1.0,1.0);\n";
  shader += "  }\n";

  return shader;
}

QString
ShaderFactory::genIsoRaycastShader(bool nearest,
				   bool useMask,
				   bool bit16,
				   int gradType)
{
  QString shader;
  shader  = "#version 420 core\n";
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D maskTex;\n";
  shader += "uniform sampler1D tagTex;\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect exitTex;\n";
  shader += "uniform float stepSize;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform vec3 vcorner;\n";
  shader += "uniform vec3 vsize;\n";
  shader += "uniform vec3 voxelScale;\n";
  shader += "uniform bool saveCoord;\n";
  shader += "uniform int skipLayers;\n";
  shader += "uniform sampler2DRect entryTex;\n";
  shader += "uniform vec3 bgcolor;\n";
  shader += "uniform int skipVoxels;\n";
  shader += "uniform int nclip;\n";
  shader += "uniform vec3 clipPos[15];\n";
  shader += "uniform vec3 clipNormal[15];\n";
  shader += "uniform float minGrad;\n";
  shader += "uniform float maxGrad;\n";


  shader += "layout (location=0) out vec4 glFragData;\n";
  
  //---------------------
  // apply clip planes to modify entry and exit points
  shader += "vec3 clip(vec3 pt0, vec3 dir)\n";
  shader += "{\n";
  shader += " vec3 pt = pt0;\n";
  shader += " vec3 nomod = vsize/max(vsize.x, max(vsize.y, vsize.z));\n";
  shader += " if (nclip > 0)\n";
  shader += "  {\n";
  shader += "    for(int c=0; c<nclip; c++)\n";
  shader += "      {\n";
  shader += "        vec3 cpos = clipPos[c];\n";
  shader += "        vec3 cnorm = clipNormal[c];\n";
  shader += "        cnorm *= nomod;\n";
  shader += "        float deno = dot(dir, cnorm);\n";
  shader += "        if (deno > 0.0)\n";
  shader += "          {\n";
  shader += "            float t = -dot((pt-cpos),cnorm)/deno;\n";
  shader += "            if (t >= 0.0)\n";
  shader += "             {\n";
  shader += "               pt = pt + t*dir;\n";
  shader += "             }\n";
  shader += "          }\n";
  shader += "      }\n";
  shader += "  }\n";
  shader += " return pt;\n";
  shader += "}\n";
  //---------------------

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "vec4 exP = texture2DRect(exitTex, gl_FragCoord.st);\n";
  shader += "vec4 enP = texture2DRect(entryTex, gl_FragCoord.st);\n";

  shader += "glFragData = vec4(0.0);\n";

  shader += "if (exP.a < 0.001 || enP.a < 0.001) discard;\n";

  shader += "vec3 exitPoint = exP.rgb;\n";
  shader += "vec3 entryPoint = enP.rgb;\n";

  //==========
  shader += "entryPoint =  entryPoint/voxelScale;\n";
  shader += "entryPoint = (entryPoint - vcorner)/vsize;\n";
  shader += "exitPoint =  exitPoint/voxelScale;\n";
  shader += "exitPoint = (exitPoint - vcorner)/vsize;\n";
  //==========

  shader += "vec3 dir = (exitPoint-entryPoint);\n";

  shader += "entryPoint = clip(entryPoint, dir);\n";
  shader += "exitPoint = clip(exitPoint, -dir);\n";
  shader += "vec3 dirN = (exitPoint-entryPoint);\n";
  shader += "if (dot(dir, dirN) <= 0.0) discard;\n";
  shader += "dir = dirN;\n";
  
  shader += "float totlen = length(dir);\n";
  shader += "if (totlen < 0.001) discard;\n";

  //shader += "vec3 normDir = normalize(dir);\n";
  shader += "vec3 deltaDir = normalize(dir)*stepSize;\n";
  shader += "float deltaDirLen = length(deltaDir);\n";

  shader += "vec3 voxelCoord = entryPoint;\n";
  shader += "vec4 colorAcum = vec4(0.0);\n"; // The dest color
  shader += "float lengthAcum = 0.0;\n";

  // backgroundColor
  shader += "vec4 bgColor = vec4(0.0, 0.0, 0.0, 0.0);\n";
  
  shader += "bool gotFirstHit = false;\n";
  shader += "int nskipped = 0;\n"; 
  shader += "bool solid = false;\n";

  shader += "vec3 skipVoxStart = vec3(0.0);\n";
  shader += "int iend = int(length(exitPoint-entryPoint)/stepSize);\n";
  shader += "for(int i=0; i<iend; i++)\n";
  shader += "{\n";

  // -- get exact texture coordinate so we don't get tag interpolation --
  shader += "  vec3 vC = voxelCoord*vsize;\n";
  shader += "  bvec3 vclt = lessThan(floor(vC+0.5), vC);\n";
  shader += "  vC += vec3(vclt)*vec3(0.5);\n";
  shader += "  vC -= vec3(not(vclt))*vec3(0.5);\n";
  shader += "  vC /= vsize;\n";

  if (nearest)
    shader += "  float val = texture(dataTex, vC).x;\n";
  else
    shader += "  float val = texture(dataTex, voxelCoord).x;\n";

  shader += "  vec4 colorSample = vec4(0.0);\n";
  
  
  if (!bit16)
    shader += "  colorSample = texture(lutTex, vec2(val,0.0));\n";
  else
    {
      shader += "  int h0 = int(65535.0*val);\n";
      shader += "  int h1 = h0 / 256;\n";
      shader += "  h0 = int(mod(float(h0),256.0));\n";
      shader += "  float fh0 = float(h0)/256.0;\n";
      shader += "  float fh1 = float(h1)/256.0;\n";
      shader += "  colorSample = texture(lutTex, vec2(fh0,fh1));\n";
    }

  shader += "  if (colorSample.a > 0.0)\n";
  shader += " {\n";
  if (gradType == 0)
    shader += getGrad(nearest);
  else if (gradType == 1)
    shader += getGrad2(nearest);
  else
    shader += getGrad3(nearest);

  shader += "  colorSample = mix(vec4(0.0), colorSample, step(gradMag, maxGrad)*step(minGrad, gradMag));\n";
  shader += " }\n";

  
  if (useMask)
    {
      shader += "  float tag = texture(maskTex, vC).x;\n";
      shader += "  vec4 tagcolor = texture(tagTex, tag);\n";
      shader += "  if (tag < 0.001) tagcolor.rgb = colorSample.rgb;\n";

      shader += "  colorSample.rgb = mix(colorSample.rgb, tagcolor.rgb, 0.5);\n";

      // so that we can use tag opacity to hide certain tagged regions
      // tagcolor.a should either 0 or 1
      shader += "  colorSample *= tagcolor.a;\n";
    }
  else
    shader += "  float tag = 0;\n";


  //shader += "  if (!gotFirstHit && colorSample.a > 0.001) gotFirstHit = true;\n";  
  shader += "  if (!gotFirstHit && colorSample.a > 0.001)\n";  
  shader += "  {\n";
  shader += "    gotFirstHit = true;\n";
  shader += "    skipVoxStart = voxelCoord*vsize;\n";
  shader += "  }\n";


  // ----------------------------
  shader += "  if (gotFirstHit)\n";
  shader += "    colorSample *= step(float(skipVoxels), length(voxelCoord*vsize-skipVoxStart));\n";
  // ----------------------------


  shader += "  if (gotFirstHit && nskipped > skipLayers)\n";
  shader += "  {\n";

  shader += "  if (saveCoord && colorSample.a > 0.001)";
  shader += "    {\n";
  shader += "      glFragData = vec4(vC,1.0);\n";
  shader += "      return;\n";
  shader += "    }\n";

  shader += "  if (colorSample.a > 0.001 )\n";
  shader += "    {\n";  
  shader += "      vec3 voxpos = vcorner + voxelCoord*vsize;";
  shader += "      float zLinear = length(vcorner+voxelCoord*vsize - eyepos);\n";
  shader += "      glFragData = vec4(zLinear,val,tag,1.0);\n";
  shader += "      return;\n";
  shader += "    }\n";
  shader += "  }\n"; // gotfirsthit && nskipped > skipLayers

  shader += "  if (lengthAcum >= totlen )\n";
  shader += "      break;\n";  // terminate if opacity > 1 or the ray is outside the volume	

  shader += "  voxelCoord += deltaDir;\n";
  shader += "  lengthAcum += deltaDirLen;\n";

  shader += "  if (gotFirstHit) \n";
  shader += "   {\n";
  shader += "     if (colorSample.a > 0.001)\n";
  shader += "      {\n";  
  shader += "         if (!solid)\n";
  shader += "           {\n";    
  shader += "             solid = true;\n";
  shader += "             nskipped++;\n";  
  shader += "           }\n";  
  shader += "      }\n";
  shader += "     else\n";
  shader += "      {\n";  
  shader += "         if (solid)\n";
  shader += "           {\n";    
  shader += "             solid = false;\n";
  shader += "           }\n";  
  shader += "      }\n";
  shader += "   }\n";

  shader += "}\n";

  shader += "}\n";

  return shader;
}


QString
ShaderFactory::genEdgeEnhanceShader(bool bit16)
{
  QString shader;

  shader = "#version 420 core\n";
  shader += "out vec4 glFragColor;\n";
  shader += "uniform sampler1D tagTex;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform float dzScale;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect pvtTex;\n";
  shader += "uniform vec3 lightparm;\n";
  shader += "uniform int isoshadow;\n";
  shader += "uniform vec3 shadowcolor;\n";
  shader += "uniform vec3 edgecolor;\n";
  shader += "uniform vec3 bgcolor;\n";
  shader += "uniform vec2 shdoffset;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  glFragColor = vec4(bgcolor,1.0);\n";
  
  shader += "  vec2 spos0 = gl_FragCoord.xy;\n";
  shader += "  vec2 spos = spos0 + vec2(shdoffset.x,shdoffset.y);\n";

  shader += "  vec4 dvt = texture2DRect(pvtTex, spos0);\n";

  //---------------------
  shader += "  float alpha = dvt.w;\n";
  shader += "  if (alpha < 0.01) return;\n";
  //---------------------

  //shader += "  float depth = dvt.x;\n";
  shader += "  float val = dvt.y;\n";
  shader += "  float tag = dvt.z;\n";

  //---------------------
  shader += "  vec4 color = vec4(0.0);\n";
  //shader += "  color = texture(tagTex, tag);\n";
  shader += "  color = texture(tagTex, tag);\n";
  // so that we can use tag opacity to hide certain tagged regions
  // tagcolor.a should either 0 or 1
  shader += "  if (color.a < 0.001) discard;\n";
  //---------------------

  shader += "  if (tag < 0.001)\n";
  shader += "   {\n";
  shader += "    val = texture2DRect(pvtTex, spos0).y;\n";
  if (!bit16)
    shader += "   color = texture(lutTex, vec2(val,0.0));\n";
  else
    {
      shader += "    int h0 = int(65535.0*val);\n";
      shader += "    int h1 = h0 / 256;\n";
      shader += "    h0 = int(mod(float(h0),256.0));\n";
      shader += "    float fh0 = float(h0)/256.0;\n";
      shader += "    float fh1 = float(h1)/256.0;\n";
      shader += "    color = texture(lutTex, vec2(fh0,fh1));\n";
    }
  shader += "   }\n";


  shader += "  if (dzScale > 0.0)\n"; // edges
  shader += "  {\n";
  shader += "    float dx = texture2DRect(pvtTex, spos0+vec2(1.0,0.0)).x - texture2DRect(pvtTex, spos0-vec2(1.0,0.0)).x;\n";
  shader += "    float dy = texture2DRect(pvtTex, spos0+vec2(0.0,1.0)).x - texture2DRect(pvtTex, spos0-vec2(0.0,1.0)).x;\n";
  shader += "    float zedge = (maxZ-minZ)*0.5/dzScale;\n";
  shader += "    vec3 norm = normalize(vec3(dx, dy, (zedge*zedge)/(maxZ-minZ)));\n";  
  shader += "    color.rgb *= norm.z;\n";
  shader += "  }\n";
 
  shader+= " if (isoshadow > 0.0)\n"; // soft shadows
  shader+= " {\n";
  shader+= "   float cx[8] = float[](-1.0, 0.0, 1.0, 0.0, -1.0,-1.0, 1.0, 1.0);\n";
  shader+= "   float cy[8] = float[]( 0.0,-1.0, 0.0, 1.0, -1.0, 1.0,-1.0, 1.0);\n";
  shader+= "   float depth = dvt.x;\n";
  shader+= "   float sum = 0.0;\n";
  shader+= "   float tele = 0.0;\n";
  shader+= "   int j = 0;\n";
  shader+= "   int nsteps = int(10.0*isoshadow);\n";
  shader+= "   for(int i=0; i<nsteps; i++)\n";
  shader+= "    {\n";
  shader+= "	  float r = 1.0 + float(i)/10.0;\n";
  shader+= "	  vec2 pos = spos + vec2(r*cx[int(mod(i,8))],r*cy[int(mod(i,8))]);\n";
  shader+= "	  float od = depth - texture2DRect(pvtTex, pos).x;\n";
  shader+= "	  sum += step(3.0, od);\n";
  shader+= "	 tele ++;\n";
  shader+= "    } \n";
  shader+= "   sum /= tele;\n";
  shader+= "   sum = 1.0-sum;\n";
  shader+= "   color.rgb *= sum;\n";
  shader+= " }\n";

  shader += "  vec4 colorSample = vec4(color.rgb, 1.0);\n";

  //shader += "   colorSample.rgb *= dot(lightparm, grad);\n";

  shader += " if (any(greaterThan(colorSample.rgb,vec3(1.0,1.0,1.0)))) \n";
  shader += "   colorSample.rgb = vec3(1.0,1.0,1.0);\n";

  shader += "  glFragColor = colorSample;\n";

  shader += "}\n";

  return shader;
}


//----------------------------
//----------------------------

GLint ShaderFactory::m_boxShaderParm[20];
GLint* ShaderFactory::boxShaderParm() { return &m_boxShaderParm[0]; }

GLuint ShaderFactory::m_boxShader = 0;
GLuint ShaderFactory::boxShader()
{
  if (!m_boxShader)
    {
      m_boxShader = glCreateProgram();
      QString vertShaderString = boxShaderV();
      QString fragShaderString = boxShaderF();
  
      bool ok = loadShader(m_boxShader,
			   vertShaderString,
			   fragShaderString);  

      if (!ok)
	{
	  QMessageBox::information(0, "", "Cannot load box shaders");
	  return 0;
	}
	
	m_boxShaderParm[0] = glGetUniformLocation(m_boxShader, "MVP");

    }

  return m_boxShader;
}


QString
ShaderFactory::boxShaderV()
{
  QString shader;

  shader += "#version 410\n";
  shader += "uniform mat4 MVP;\n";
  shader += "layout(location = 0) in vec3 position;\n";
  shader += "out vec3 v3Color;\n";
  shader += "void main()\n";
  shader += "{\n";
  shader += "   v3Color = position;\n";
  shader += "   gl_Position = MVP * vec4(position, 1);\n";
  shader += "}\n";

  return shader;
}

QString
ShaderFactory::boxShaderF()
{
  QString shader;

  shader += "#version 410 core\n";
  shader += "in vec3 v3Color;\n";
  shader += "out vec4 outputColor;\n";
  shader += "void main()\n";
  shader += "{\n";
  shader += "  outputColor = vec4(v3Color,1);\n";
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
