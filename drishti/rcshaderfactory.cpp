#include "rcshaderfactory.h"

#include <QTextEdit>
#include <QVBoxLayout>
#include <QMessageBox>


bool
RcShaderFactory::loadShader(GLhandleARB &progObj,
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
RcShaderFactory::genRectBlurShaderString(int filter)
{
  QString shader;

  shader = "#extension GL_ARB_texture_rectangle : enable\n";
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
RcShaderFactory::getGrad()
{
  QString shader;

  shader += " vec3 gx, gy, gz;\n";
  shader += " gx = vec3(1.0/vsize.x,0,0);\n";
  shader += " gy = vec3(0,1.0/vsize.y,0);\n";
  shader += " gz = vec3(0,0,1.0/vsize.z);\n";
  shader += " float vx = texture3D(dataTex, voxelCoord+gx).x - texture3D(dataTex, voxelCoord-gx).x;\n";
  shader += " float vy = texture3D(dataTex, voxelCoord+gy).x - texture3D(dataTex, voxelCoord-gy).x;\n";
  shader += " float vz = texture3D(dataTex, voxelCoord+gz).x - texture3D(dataTex, voxelCoord-gz).x;\n";
  shader += " vec3 grad = vec3(vx, vy, vz);\n";

  return shader;
}

QString
RcShaderFactory::addLighting()
{
  QString shader;

  shader += " if (length(grad) > 0.1)\n";
  shader += "  {\n";
  shader += "    grad = normalize(grad);\n";
  shader += "    vec3 lightVec = viewDir;\n";
  shader += "    float diff = abs(dot(lightVec, grad));\n";
  shader += "    vec3 reflecvec = reflect(lightVec, grad);\n";
  shader += "    float spec = pow(abs(dot(grad, reflecvec)), 512.0);\n";
  shader += "    colorSample.rgb *= (0.6 + 0.4*diff + spec);\n";
  shader += "    if (any(greaterThan(colorSample.rgb,vec3(1.0,1.0,1.0)))) \n";
  shader += "      colorSample.rgb = vec3(1.0,1.0,1.0);\n";
  shader += "  }\n";

  return shader;
}

QString
RcShaderFactory::genFirstHitShader(int maxSteps, bool nearest)
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect exitTex;\n";
  shader += "uniform sampler2DRect entryTex;\n";
  shader += "uniform float stepSize;\n";
  shader += "uniform vec3 vsize;\n";
  shader += "uniform int skipLayers;\n";

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "vec4 exP = texture2DRect(exitTex, gl_FragCoord.st);\n";
  shader += "vec4 enP = texture2DRect(entryTex, gl_FragCoord.st);\n";

  shader += "gl_FragColor = vec4(0.0);\n";
  shader += "if (exP.a < 0.001 || enP.a < 0.001) discard;\n";

  shader += "vec3 exitPoint = exP.rgb;\n";
  shader += "vec3 entryPoint = enP.rgb;\n";

  shader += "vec3 dir = (exitPoint-entryPoint);\n";
  shader += "float len = length(dir);\n";
  shader += "if (len < 0.001) discard;\n";

  shader += "vec3 deltaDir = normalize(dir)*stepSize;\n";
  shader += "float deltaDirLen = length(deltaDir);\n";

  shader += "vec3 voxelCoord = entryPoint;\n";
  shader += "vec4 colorAcum = vec4(0.0);\n"; // The dest color
  shader += "float lengthAcum = 0.0;\n";

  shader += "bool gotFirstHit = false;\n";
  shader += "int nskipped = 0;\n"; 
  shader += "bool solid = false;\n";
  shader += "for(int i=0; i<int(length(exitPoint-entryPoint)/stepSize); i++)\n";
  //shader += QString("for(int i=0; i<%1; i++)\n").arg(maxSteps);
  shader += "{\n";

  // -- get exact texture coordinate so we don't get tag interpolation --
  if (nearest)
    {
      shader += "  vec3 vC = voxelCoord*vsize;\n";
      shader += "  bvec3 vclt = lessThan(floor(vC+0.5), vC);\n";
      shader += "  vC += vec3(vclt)*vec3(0.5);\n";
      shader += "  vC -= vec3(not(vclt))*vec3(0.5);\n";
      shader += "  vC /= vsize;\n";
      shader += "  float val = texture3D(dataTex, vC).x;\n";
    }
  else
    shader += "  float val = texture3D(dataTex, voxelCoord).x;\n";

  shader += "  vec4 colorSample = vec4(1.0);\n";

  shader += getGrad();
  shader += "  float gradlen = length(grad);\n";
  shader += "  colorSample = texture2D(lutTex, vec2(val,gradlen));\n";

  shader += "  if (!gotFirstHit && colorSample.a > 0.001) gotFirstHit = true;\n";  

  shader += "  if (gotFirstHit && nskipped > skipLayers)\n";
  shader += "  {\n";
  shader += "    if (colorSample.a > 0.001 )\n";
  shader += "      {\n";  
  shader += "        gl_FragColor = vec4(voxelCoord,1.0);\n";
  shader += "        return;\n";
  shader += "      }\n";
  shader += "  }\n"; // gotfirsthit && nskipped > skipLayers

  shader += "  if (lengthAcum >= len )\n";
  shader += "    return;\n";  // terminate if opacity > 1 or the ray is outside the volume	

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
  shader += "           solid = false;\n";
  shader += "      }\n";
  shader += "   }\n"; // got first hit

  shader += " }\n"; // loop over maxSteps

  shader += "}\n";

  return shader;
}

QString
RcShaderFactory::genRaycastShader(int maxSteps, bool firstHit, bool nearest, float raylenFrac)
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect exitTex;\n";
  shader += "uniform float stepSize;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform vec3 vcorner;\n";
  shader += "uniform vec3 vsize;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";  
  shader += "uniform bool saveCoord;\n";
  shader += "uniform int skipLayers;\n";
  shader += "uniform sampler2DRect entryTex;\n";
  shader += "uniform vec3 bgcolor;\n";

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "vec4 exP = texture2DRect(exitTex, gl_FragCoord.st);\n";
  shader += "vec4 enP = texture2DRect(entryTex, gl_FragCoord.st);\n";

  shader += "if (exP.a < 0.001 || enP.a < 0.001) discard;\n";

  shader += "vec3 exitPoint = exP.rgb;\n";
  shader += "vec3 entryPoint = enP.rgb;\n";

  shader += "vec3 dir = (exitPoint-entryPoint);\n";
  shader += "float len = length(dir);\n";
  shader += "if (len < 0.001) discard;\n";

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
 

  if (!firstHit)
    shader += QString("for(int i=0; i<max(10,int(%1*length(exitPoint-entryPoint)/stepSize)); i++)\n").arg(raylenFrac);
  else
    shader += QString("for(int i=0; i<%1; i++)\n").arg(maxSteps);

  shader += "{\n";

  // -- get exact texture coordinate so we don't get tag interpolation --
  shader += "  vec3 vC = voxelCoord*vsize;\n";
  shader += "  bvec3 vclt = lessThan(floor(vC+0.5), vC);\n";
  shader += "  vC += vec3(vclt)*vec3(0.5);\n";
  shader += "  vC -= vec3(not(vclt))*vec3(0.5);\n";
  shader += "  vC /= vsize;\n";

  if (nearest)
    shader += "  float val = texture3D(dataTex, vC).x;\n";
  else
    shader += "  float val = texture3D(dataTex, voxelCoord).x;\n";

  shader += "  vec4 colorSample = vec4(1.0);\n";

  shader += getGrad();
  shader += "  float gradlen = length(grad);\n";
  shader += "  colorSample = texture2D(lutTex, vec2(val,gradlen));\n";

  shader += "  if (!gotFirstHit && colorSample.a > 0.001) gotFirstHit = true;\n";  

  shader += "  if (gotFirstHit && nskipped > skipLayers)\n";
  shader += "  {\n";

  if (firstHit)
    {
      shader += "  if (saveCoord && colorSample.a > 0.001)";
      shader += "    {\n";
      shader += "      gl_FragData[0] = vec4(vC,1.0);\n";
      shader += "      return;\n";
      shader += "    }\n";

      shader += "  if (colorSample.a > 0.001 )\n";
      shader += "    {\n";  
      shader += "      vec3 voxpos = vcorner + voxelCoord*vsize;";
      shader += "      vec3 I = voxpos - eyepos;\n";
      shader += "      float z = dot(I, normalize(viewDir));\n";
      shader += "      z = (z-minZ)/(maxZ-minZ);\n";
      shader += "      z = clamp(z, 0.0, 1.0);\n";
      shader += "      gl_FragData[0] = vec4(z,val,gradlen,1.0);\n";
      shader += "      if (gradlen > 0.2)\n";
      shader += "        {\n";
      shader += "           grad = normalize(grad);\n";
      shader += "           vec3 lightVec = viewDir;\n";
      shader += "           float diff = abs(dot(lightVec, grad));\n";
      shader += "           vec3 reflecvec = reflect(lightVec, grad);\n";
      shader += "           float spec = pow(abs(dot(grad, reflecvec)), 512.0);\n";
      shader += "           gl_FragData[1] = vec4(1.0,diff,spec,1.0);\n";
      shader += "        }\n";
      shader += "      else\n";
      shader += "        gl_FragData[1] = vec4(1.0,0.0,0.0,1.0);\n";
      shader += "      return;\n";
      shader += "    }\n";
    }
  else
    {
      shader += "    if (saveCoord && colorSample.a > 0.001)";
      shader += "      {\n";
      shader += "        gl_FragColor = vec4(vC,1.0);\n";
      shader += "        return;\n";
      shader += "      }\n";
    }

  shader += getGrad();
  shader += addLighting();

  shader += "    colorSample.rgb *= colorSample.a;\n";

  shader += "    colorAcum += (1.0 - colorAcum.a) * colorSample;\n";

  shader += "  }\n"; // gotfirsthit && nskipped > skipLayers

  shader += "  if (lengthAcum >= len )\n";
  shader += "    {\n";
  shader += "      colorAcum.rgb = colorAcum.rgb*colorAcum.a + (1.0 - colorAcum.a)*bgColor.rgb;\n";
  shader += "      break;\n";  // terminate if opacity > 1 or the ray is outside the volume	
  shader += "    }\n";
  shader += "  else if (colorAcum.a > 1.0)\n";
  shader += "    {\n";
  shader += "      colorAcum.a = 1.0;\n";
  shader += "      break;\n";
  shader += "    }\n";

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

  if (!firstHit)
    shader += "gl_FragColor = mix(colorAcum, vec4(bgcolor,1.0), 1.0-colorAcum.a);\n";
  else
    {
      shader += "gl_FragData[0] = colorAcum;\n";
      shader += "gl_FragData[1] = vec4(0.0,0.0,0.0,1.0);\n";
    }

  shader += "}\n";

  return shader;
}

QString
RcShaderFactory::genXRayShader(int maxSteps, bool firstHit, bool nearest, float raylenFrac)
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect exitTex;\n";
  shader += "uniform float stepSize;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform vec3 vcorner;\n";
  shader += "uniform vec3 vsize;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";  
  shader += "uniform bool saveCoord;\n";
  shader += "uniform int skipLayers;\n";
  shader += "uniform sampler2DRect entryTex;\n";
  shader += "uniform vec3 bgcolor;\n";

  shader += "void main(void)\n";
  shader += "{\n";


  shader += "vec4 exP = texture2DRect(exitTex, gl_FragCoord.st);\n";
  shader += "vec4 enP = texture2DRect(entryTex, gl_FragCoord.st);\n";

  shader += "if (exP.a < 0.001 || enP.a < 0.001) discard;\n";

  shader += "vec3 exitPoint = exP.rgb;\n";
  shader += "vec3 entryPoint = enP.rgb;\n";

  shader += "vec3 dir = (exitPoint-entryPoint);\n";
  shader += "float len = length(dir);\n";
  shader += "if (len < 0.001) discard;\n";

  shader += "vec3 normDir = normalize(dir);\n";
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

  if (!firstHit)
    shader += QString("for(int i=0; i<max(10,int(%1*length(exitPoint-entryPoint)/stepSize)); i++)\n").arg(raylenFrac);
  else
    shader += QString("for(int i=0; i<%1; i++)\n").arg(maxSteps);

  shader += "{\n";

  // -- get exact texture coordinate so we don't get tag interpolation --
  shader += "  vec3 vC = voxelCoord*vsize;\n";
  shader += "  bvec3 vclt = lessThan(floor(vC+0.5), vC);\n";
  shader += "  vC += vec3(vclt)*vec3(0.5);\n";
  shader += "  vC -= vec3(not(vclt))*vec3(0.5);\n";
  shader += "  vC /= vsize;\n";

  if (nearest)
    shader += "  float val = texture3D(dataTex, vC).x;\n";
  else
    shader += "  float val = texture3D(dataTex, voxelCoord).x;\n";


  shader += getGrad();
  shader += "  vec4 colorSample = texture2D(lutTex, vec2(val,length(grad)));\n";

  shader += "  if (!gotFirstHit && colorSample.a > 0.001) gotFirstHit = true;\n";  

  shader += "  if (gotFirstHit && nskipped > skipLayers)\n";
  shader += "  {\n";

  shader += "    if (saveCoord && colorSample.a > 0.001)";
  shader += "      {\n";
  shader += "        gl_FragColor = vec4(vC,1.0);\n";
  shader += "        return;\n";
  shader += "      }\n";

  if (firstHit)
    {
      shader += "  if (colorSample.a > 0.001 )\n";
      shader += "    {\n";  
      shader += "      vec3 voxpos = vcorner + voxelCoord*vsize;";
      shader += "      vec3 I = voxpos - eyepos;\n";
      shader += "      float z = dot(I, normalize(dir));\n";
      shader += "      z = (z-minZ)/(maxZ-minZ);\n";
      shader += "      z = clamp(z, 0.0, 1.0);\n";
      shader += "      gl_FragColor = vec4(z,val,0.0,1.0);\n";
      shader += "      return;\n";
      shader += "    }\n";
    }

  // modulate opacity by angle wrt view direction
  //shader += getGrad();
  shader += "if (length(grad) > 0.1)\n";
  shader += "  {\n";
  shader += "    grad = normalize(grad);\n";
  shader += "    colorSample.a *= (0.3+0.9*(1.0-abs(dot(grad,normDir))));\n";
  shader += "  }\n";
  shader += "else\n";
  shader += "  colorSample.a *= 0.2;\n";

  // reduce the opacity
  shader += "    colorSample.a *= 0.1;\n";
  shader += "    colorSample.rgb *= colorSample.a;\n";

  // just add up, don't composite
  shader += "    colorAcum += colorSample;\n";

  shader += "  }\n"; // gotfirsthit && nskipped > skipLayers

  shader += "  if (lengthAcum >= len )\n";
  shader += "    {\n";
  shader += "      colorAcum.rgb = colorAcum.rgb*colorAcum.a + (1.0 - colorAcum.a)*bgColor.rgb;\n";
  shader += "      break;\n";  // terminate if opacity > 1 or the ray is outside the volume	
  shader += "    }\n";
  shader += "  else if (colorAcum.a > 1.0)\n";
  shader += "    {\n";
  shader += "      colorAcum.a = 1.0;\n";
  shader += "      break;\n";
  shader += "    }\n";

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

  //shader += "gl_FragColor = colorAcum;\n";    
  shader += "gl_FragColor = mix(colorAcum, vec4(bgcolor,1.0), 1.0-colorAcum.a);\n";

  shader += "}\n";

  return shader;
}

QString
RcShaderFactory::genEdgeEnhanceShader()
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointpos;\n";
  shader += "uniform sampler2DRect normalTex;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";
  shader += "uniform float dzScale;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect pvtTex;\n";
  shader += "uniform vec3 lightparm;\n";
  shader += "uniform int isoshadow;\n";
  shader += "uniform vec3 shadowcolor;\n";
  shader += "uniform vec3 edgecolor;\n";
  shader += "uniform vec3 bgcolor;\n";
  shader += "uniform vec2 shdoffset;\n";
  shader += "uniform float edgethickness;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  gl_FragColor = vec4(bgcolor,1.0);\n";

  shader += "  vec2 spos0 = gl_FragCoord.xy;\n";
  shader += "  vec2 spos = spos0 + vec2(shdoffset.x,shdoffset.y);\n";

  shader += "  vec4 dvt = texture2DRect(pvtTex, spos0);\n";
  shader += "  vec3 grad = texture2DRect(normalTex, spos0).xyz;\n";

  //---------------------
  shader += "  float alpha = dvt.w;\n";
  shader += "  if (alpha < 0.01) return;\n";
  //---------------------

  shader += "  float depth = dvt.x;\n";

  //---------------------
  shader += "  vec4 color = vec4(0.0);\n";

  shader += "   {\n";
  float dx[9] = {-1.0,-1.0,-1.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
  float dy[9] = {-1.0, 0.0, 1.0,-1.0, 0.0, 1.0,-1.0, 0.0, 1.0};
  shader += "    float cx[9];\n";
  shader += "    float cy[9];\n";
  for(int i=0; i<9; i++)
    shader += QString("    cx[%1] = float(%2);\n").arg(i).arg(dx[i]);
  for(int i=0; i<9; i++)
    shader += QString("    cy[%1] = float(%2);\n").arg(i).arg(dy[i]);

  shader += "    vec3 rgb = vec3(0.0);\n";
  shader += "    for(int i=0; i<9; i++)\n";
  shader += "    {\n";
  shader += "       vec2 vg = texture2DRect(pvtTex, spos0+vec2(1.0,0.0)).yz;\n";
  shader += "       color = texture2D(lutTex,vec2(vg.x,vg.y));\n";
  shader += "       rgb += color.rgb;\n";
  shader += "    }\n";
  shader += "    color.a = 1.0;\n";
  shader += "    color.rgb = rgb/9.0;\n";
  shader += "   }\n";


//  // find depth gradient
  shader += "  float zedge = 1.0;\n";
  shader += "  if (dzScale > 0.0)\n";
  shader += "  {\n";
  shader += "    float r = edgethickness;\n";
  shader += "    float dx = texture2DRect(pvtTex, spos0+vec2(r,0.0)).x - texture2DRect(pvtTex, spos0-vec2(r,0.0)).x;\n";
  shader += "    float dy = texture2DRect(pvtTex, spos0+vec2(0.0,r)).x - texture2DRect(pvtTex, spos0-vec2(0.0,r)).x;\n";
  shader += "    zedge = 0.5+dzScale/2.0;\n";
  shader += "    vec3 norm = normalize(vec3(dx, dy, (zedge*zedge)/(maxZ-minZ)));\n";  
  shader += "    zedge = norm.z;\n";
  shader += "  }\n";

  
  // compute obscurance
  shader += "  float ege1 = 0.01;\n";
  shader += "  float ege2 = 0.03;\n";
  shader += "  float sum = 0.0;\n";
  shader += "  float od = 0.0;\n";
  shader += "  float shadow = 1.0;\n";

  shader += "  if (isoshadow > 0)\n";
  shader += "    {\n";
  shader += "      float tele = 0.0;\n";
  shader += "      float r = 1.0;\n";
  shader += "      float theta = 0.0;\n";
  shader += "      int cnt = 4;\n";
  shader += "      float ege = 0.0;\n";
  shader += "      int j = 0;\n";
  shader += "      for(int i=0; i<(20*isoshadow); i++)\n";
  shader += "      {\n";
  shader += "        int x = int(r*sin(theta));\n";
  shader += "        int y = int(r*cos(theta));\n";
  shader += "        vec2 pos = spos0 + vec2(i*0.05,i*0.05)*shdoffset + vec2(x,y);\n";
  shader += "        float od = depth - texture2DRect(pvtTex, pos).x;\n";
  shader += "        float wt = abs(spos0.x-pos.x)+abs(spos0.y-pos.y);\n";
  shader += "        float ege = (wt-1.0)*0.0005;\n";
  shader += "        sum += step(ege, od);\n";
  shader += "        tele ++;\n";
  shader += "        r += i/cnt;\n";
  shader += "        theta += 6.28/(r+3.0);\n";  
  shader += "        if (i>=cnt) cnt = cnt+int(r)+3;\n";
  shader += "      }\n"; 
  shader += "      shadow = 0.1 + 0.9*(1.0-sum/tele);\n";
  shader += "    }\n";


  shader += "  vec4 colorSample = vec4(color.rgb, 1.0);\n";
  shader += "  colorSample.rgb = mix(colorSample.rgb, shadowcolor, 1.0-shadow);\n";
  shader += "  colorSample.rgb = mix(colorSample.rgb, edgecolor, 1.0-zedge);\n";

  shader += "   colorSample.rgb *= dot(lightparm, grad);\n";

  shader += " if (any(greaterThan(colorSample.rgb,vec3(1.0,1.0,1.0)))) \n";
  shader += "   colorSample.rgb = vec3(1.0,1.0,1.0);\n";

  shader += "  gl_FragColor = colorSample;\n";

  shader += "}\n";

  return shader;
}
