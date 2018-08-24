#include "rcshaderfactory.h"
#include "cropshaderfactory.h"
#include "blendshaderfactory.h"
#include "global.h"

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
  shader += " grad = vec3(vx, vy, vz);\n";

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
  shader += "   colorSample.rgb *= dot(lightparm, vec3(1.0,diff,spec));\n";
  shader += "    if (any(greaterThan(colorSample.rgb,vec3(1.0,1.0,1.0)))) \n";
  shader += "      colorSample.rgb = vec3(1.0,1.0,1.0);\n";
  shader += "  }\n";

  return shader;
}

QString
RcShaderFactory::genEntryExitShader()
{
  QString shader;
  shader += "varying vec3 pointpos;\n";
  shader += "uniform vec3 texBoxMin;\n";
  shader += "uniform vec3 texBoxSize;\n";
  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  gl_FragData[0] = vec4((pointpos-texBoxMin)/texBoxSize,1.0);\n";
  shader += "}\n";
  return shader;
}


QString
RcShaderFactory::genIsoRaycastShader(bool nearest,
				     QList<CropObject> crops,
				     bool bit16)
{
  //------------------------------------
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
  //------------------------------------

  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler3D filledTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect exitTex;\n";
  shader += "uniform sampler2DRect entryTex;\n";
  shader += "uniform float stepSize;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform vec3 vcorner;\n";
  shader += "uniform vec3 vsize;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";  
  shader += "uniform bool saveCoord;\n";
  shader += "uniform int skipLayers;\n";
  shader += "uniform vec3 ftsize;\n";
  shader += "uniform float boxSize;\n";
  shader += "uniform sampler1D tagTex;\n";
  shader += "uniform bool mixTag;\n";
  shader += "uniform int skipVoxels;\n";
  shader += "uniform int lod;\n";

  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops);

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "vec4 exP = texture2DRect(exitTex, gl_FragCoord.st);\n";
  shader += "vec4 enP = texture2DRect(entryTex, gl_FragCoord.st);\n";

  shader += "gl_FragData[0] = vec4(0.0);\n";
  shader += "gl_FragData[1] = vec4(0.0);\n";

  shader += "if (any(lessThan(enP.xyz,vec3(0.0))) ||\n";
  shader += "    any(greaterThan(enP.xyz,vec3(1.0)))) return;\n";
  shader += "if (any(lessThan(exP.xyz,vec3(0.0))) ||\n";
  shader += "    any(greaterThan(exP.xyz,vec3(1.0)))) return;\n";

  shader += "if (exP.a < 0.001 || enP.a < 0.001) discard;\n";

  shader += "vec3 exitPoint = exP.rgb;\n";
  shader += "vec3 entryPoint = enP.rgb;\n";

  shader += "vec3 dir = (exitPoint-entryPoint);\n";
  shader += "float totlen = length(dir);\n";
  shader += "if (totlen < 0.001) discard;\n";

  shader += "vec3 dirvec = normalize(dir);\n";

  shader += "vec3 deltaDir = dirvec*stepSize;\n";
  shader += "float deltaDirLen = stepSize;\n";

  shader += "vec3 voxelCoord = entryPoint;\n";
  shader += "float lengthAcum = 0.0;\n";

  shader += "bool gotFirstHit = false;\n";
  shader += "int nskipped = 0;\n"; 
  shader += "bool solid = false;\n";
 
  shader += "vec3 skipVoxStart = vec3(0.0);\n";
  shader += "int iend = int(length(exitPoint-entryPoint)/stepSize);\n";
  shader += "for(int i=0; i<iend; i++)\n";

  shader += "{\n";

  shader += "  vec3 vC = voxelCoord*vsize;\n";
  shader += "  vec3 texCoord = vC*vec3(lod)+vcorner;\n";

  //---------------------------------
  // -- check filled boxes --
  shader += "  vec3 ftpos = texCoord/vec3(boxSize);\n";
  shader += "  bvec3 fclt = lessThan(floor(ftpos+0.5), ftpos);\n";
  shader += "  ftpos += vec3(fclt)*vec3(0.5);\n";
  shader += "  ftpos -= vec3(not(fclt))*vec3(0.5);\n";
  shader += "  if (texture3D(filledTex, ftpos/ftsize).x < 0.5)\n";
  shader += "   {\n";
  shader += "     vec3 r = texCoord-(ftpos+vec3(boxSize/2.0));\n";
  shader += "     float jump = dot(r, deltaDir);\n";
  shader += "     if (abs(jump) > 0.0)\n";
  shader += "       {\n";
  shader += "         vC += 2.0*jump*deltaDir + deltaDir;\n";
  shader += "         voxelCoord = vC/vsize;\n";
  shader += "         lengthAcum = length(voxelCoord-entryPoint);\n";
  shader += "       }\n";
  shader += "   }\n";
  //---------------------------------

  shader += "  vec4 colorSample = vec4(1.0);\n";

  shader += "  float val, gradlen;\n";
  shader += "  vec3 grad;\n";
  shader += "  float tfSetToUse = 0.0;\n";

  shader += "  float feather = 0.0;\n";
  if (cropPresent)
    shader += "  feather = crop(texCoord, false);\n";


  shader += "  if (feather < 0.5)\n";
  shader += "  {\n";
  // -- get exact texture coordinate so we don't get interpolation artefacts --
  shader += "    bvec3 vclt = lessThan(floor(vC+0.5), vC);\n";
  shader += "    vC += vec3(vclt)*vec3(0.5);\n";
  shader += "    vC -= vec3(not(vclt))*vec3(0.5);\n";
  shader += "    vC /= vsize;\n";

  if (nearest)
    shader += "    val = texture3D(dataTex, vC).x;\n";
  else
    shader += "    val = texture3D(dataTex, voxelCoord).x;\n";

  shader +=      getGrad();
  shader += "    gradlen = length(grad);\n";
  if (!bit16)
    shader += QString("    colorSample = texture2D(lutTex, vec2(val,gradlen*%1));\n").\
      arg(1.0/Global::lutSize());
  else
    {
      shader += "   int h0 = int(65535.0*val);\n";
      shader += "   int h1 = h0 / 256;\n";
      shader += "   h0 = int(mod(float(h0),256.0));\n";
      shader += "   float fh0 = float(h0)/256.0;\n";
      shader += "   float fh1 = float(h1)/256.0;\n";
      shader += QString("    colorSample = texture2D(lutTex, vec2(fh0,fh1*%1));\n").\
	arg(1.0/Global::lutSize());
    }
	         
  shader += "    if (mixTag)\n";
  shader += "    {\n";
  if (!bit16)
    shader += "      vec4 tagcolor = texture1D(tagTex, val);\n";
  else
    shader += "      vec4 tagcolor = texture1D(tagTex, fh0);\n";
  shader += "      if (tagcolor.a > 0.0)\n";
  shader += "        colorSample.rgb = mix(colorSample.rgb, tagcolor.rgb, tagcolor.a);\n";
  shader += "      else\n";
  shader += "        colorSample = vec4(0.0);\n";
  shader += "    }\n";
  if (viewPresent)
    {
      if (!bit16)
	shader += QString("    vec2 vg = vec2(val,gradlen*%1);\n").arg(1.0/Global::lutSize());  
      else
	shader += QString("    vec2 vg = vec2(fh0, fh1*%1);\n").arg(1.0/Global::lutSize());  
      shader += "    vec4 cs=vec4(0.0);\n";
      shader += "    blend(true, texCoord, vg, cs);\n";
      shader += QString("    if (cs.x > 0.0) tfSetToUse = cs.y*%1;\n").arg(1.0/Global::lutSize());
      if (!bit16)
	shader += QString("    colorSample = texture2D(lutTex, vec2(val,gradlen*%1 + tfSetToUse));\n").arg(1.0/Global::lutSize());
      else
	shader += QString("    colorSample = texture2D(lutTex, vec2(fh0,fh1*%1 + tfSetToUse));\n").arg(1.0/Global::lutSize());
    }
  shader += "  }\n";
  shader += "  else\n";  // cropped
  shader += "  {\n";
  shader += "    colorSample = vec4(0.0);\n";
  shader += "  }\n";  // feather


  shader += "    colorSample.rgb *= colorSample.a;\n";

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
  shader += "           vec3 reflecvec = reflect(lightVec, grad);\n";
  shader += "           float spec = pow(abs(dot(grad, reflecvec)), 512.0);\n";
  shader += "           gl_FragData[1] = vec4(1.0,tfSetToUse,spec,1.0);\n";
  //------
  // not passing through diffuse
  //shader += "           float diff = abs(dot(lightVec, grad));\n";
  //shader += "           gl_FragData[1] = vec4(1.0,diff,spec,1.0);\n";
  //------
  shader += "        }\n";
  shader += "      else\n";
  shader += "        gl_FragData[1] = vec4(1.0,tfSetToUse,0.0,1.0);\n";
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
RcShaderFactory::genEdgeEnhanceShader(bool bit16)
{
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "varying vec3 pointpos;\n";
  shader += "uniform sampler1D tagTex;\n";
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
  shader += "uniform vec2 shdoffset;\n";
  shader += "uniform float edgethickness;\n";
  shader += "uniform bool mixTag;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  gl_FragColor = vec4(1.0);\n";

  shader += "  vec2 spos0 = gl_FragCoord.xy;\n";
  shader += "  vec2 spos = spos0 + vec2(shdoffset.x,shdoffset.y);\n";

  shader += "  vec4 dvt = texture2DRect(pvtTex, spos0);\n";
  shader += "  vec3 grad = texture2DRect(normalTex, spos0).xyz;\n";
  shader += "  float tfSetToUse = grad.y;\n";

  //---------------------
  shader += "  float alpha = dvt.w;\n";
  shader += "  if (alpha < 0.01) discard;\n";
  //---------------------

  shader += "  float depth = dvt.x;\n";


  //---------------------
  shader += "  vec4 color = vec4(0.0);\n";

  float dx[9] = {-0.5,-0.5,-0.5, 0.0, 0.0, 0.0, 0.5, 0.5, 0.5};
  float dy[9] = {-0.5, 0.0, 0.5,-0.5, 0.0, 0.5,-0.5, 0.0, 0.5};
  shader += "  float cx[9];\n";
  shader += "  float cy[9];\n";
  for(int i=0; i<9; i++)
    shader += QString("  cx[%1] = float(%2);\n").arg(i).arg(dx[i]);
  for(int i=0; i<9; i++)
    shader += QString("  cy[%1] = float(%2);\n").arg(i).arg(dy[i]);

  shader += "  vec3 rgb = vec3(0.0);\n";
  shader += "  for(int i=0; i<9; i++)\n";
  shader += "  {\n";
  shader += "    vec2 vg = texture2DRect(pvtTex, spos0+vec2(cx[i],cy[i])).yz;\n";
  if (!bit16)
    shader += QString("    color = texture2D(lutTex,vec2(vg.x,vg.y*float(%1) + tfSetToUse));\n").arg(1.0/Global::lutSize());
  else
    {
      shader += "   int h0 = int(65535.0*vg.x);\n";
      shader += "   int h1 = h0 / 256;\n";
      shader += "   h0 = int(mod(float(h0),256.0));\n";
      shader += "   float fh0 = float(h0)/256.0;\n";
      shader += "   float fh1 = float(h1)/256.0;\n";
      shader += QString("    color = texture2D(lutTex, vec2(fh0,fh1*%1 + tfSetToUse));\n").\
	arg(1.0/Global::lutSize());
    }
  shader += "    rgb += color.rgb;\n";
  shader += "  }\n";
  shader += "  color.a = 1.0;\n";
  shader += "  color.rgb = rgb/9.0;\n";


  shader += "if (mixTag)\n";
  shader += "  {\n";  
  if (!bit16)
    shader += "    float tag = dvt.y;\n"; // use value as tag
  else
    {
      shader += "   int h0 = int(65535.0*dvt.y);\n";
      shader += "   h0 = int(mod(float(h0),256.0));\n";
      shader += "   float tag = float(h0)/256.0;\n";
    }
  shader += "    vec4 tcolor = texture1D(tagTex, tag);\n";
  shader += "    color = mix(color, tcolor, tcolor.a);\n";
  shader += "  }\n";  


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
  shader += "  float shadow = 1.0;\n";

  shader += "  if (length(shdoffset) < 0.1)\n";
  shader += "  {\n";
  shader += "  if (isoshadow > 0)\n";
  shader += "    {\n";
  shader += "      float sum = 0.0;\n";
  shader += "      float tele = 0.0;\n";
  shader += "      float r = 0.5;\n";
  shader += "      float theta = 0.0;\n";
  shader += "      float ege = 0.0;\n";
  shader += "      int j = 0;\n";
  shader += "      float rstep = 0.02+0.01*float(isoshadow);\n";
  shader += "      for(int i=0; i<128; i++)\n";
  shader += "      {\n";
  shader += "        int x = int(r*sin(theta));\n";
  shader += "        int y = int(r*cos(theta));\n";
  shader += "        vec2 pos = spos0 + vec2(float(i)*0.05,float(i)*0.05)*shdoffset + vec2(x,y);\n";
  shader += "        float od = depth - texture2DRect(pvtTex, pos).x;\n";
  shader += "        float wt = abs(spos0.x-pos.x)+abs(spos0.y-pos.y);\n";
  shader += "        float ege = wt*0.0005;\n";
  shader += "        sum += step(ege, od);\n";
  shader += "        tele ++;\n";
  shader += "        r += rstep;\n";
  shader += "        theta += 0.3;\n";  
  shader += "      }\n"; 
  shader += "      shadow = 0.1 + 0.9*(1.0-sum/tele);\n";
  shader += "    }\n";  
  shader += "  }\n";
  shader += "  else\n";
  shader += "  {\n";
  shader += "  if (isoshadow > 0)\n";
  shader += "    {\n";
  shader += "      float sum = 0.0;\n";
  shader += "      float tele = 0.0;\n";
  shader += "      float ege = 0.0;\n";
  shader += "      float r = -float(isoshadow);\n";
  shader += "      vec2 pso = normalize(vec2(-shdoffset.y, shdoffset.x));\n";
  shader += "      int j = 0;\n";
  shader += "      for(int i=0; i<128; i++)\n";
  shader += "      {\n";
  shader += "        vec2 pos = spos0 + vec2(float(i)*0.05,float(i)*0.05)*shdoffset + r*pso;\n";
  shader += "        float od = depth - texture2DRect(pvtTex, pos).x;\n";
  shader += "        float wt = abs(spos0.x-pos.x)+abs(spos0.y-pos.y);\n";
  shader += "        float ege = wt*0.0005;\n";
  shader += "        sum += step(ege, od);\n";
  shader += "        tele ++;\n";
  shader += "        r += 1.0;\n";
  shader += "        r = mix(r, -float(isoshadow), step(float(isoshadow),r));\n";
  shader += "      }\n"; 
  shader += "      shadow = 0.1 + 0.9*(1.0-sum/tele);\n";
  shader += "    }\n";
  shader += "  }\n";


  shader += "  vec4 colorSample = vec4(color.rgb, 1.0);\n";
  shader += "  colorSample.rgb = mix(colorSample.rgb, shadowcolor, 1.0-shadow);\n";
  shader += "  colorSample.rgb = mix(colorSample.rgb, edgecolor, 1.0-zedge);\n";

  //shader += "   colorSample.rgb *= dot(lightparm, grad);\n";
  shader += "   colorSample.rgb *= dot(lightparm, vec3(1.0,0.0,grad.z));\n";

  shader += " if (any(greaterThan(colorSample.rgb,vec3(1.0,1.0,1.0)))) \n";
  shader += "   colorSample.rgb = vec3(1.0,1.0,1.0);\n";

  shader += "  gl_FragColor = colorSample;\n";

  shader += "}\n";

  return shader;
}

QString
RcShaderFactory::genFirstHitShader(bool nearest,
				   QList<CropObject> crops,
				   bool bit16)
{
  //------------------------------------
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
  //------------------------------------

  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler3D filledTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect exitTex;\n";
  shader += "uniform sampler2DRect entryTex;\n";
  shader += "uniform float stepSize;\n";
  shader += "uniform vec3 vcorner;\n";
  shader += "uniform vec3 vsize;\n";
  shader += "uniform int skipLayers;\n";
  shader += "uniform vec3 ftsize;\n";
  shader += "uniform float boxSize;\n";  
  shader += "uniform int lod;\n";

  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops);

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "vec4 exP = texture2DRect(exitTex, gl_FragCoord.st);\n";
  shader += "vec4 enP = texture2DRect(entryTex, gl_FragCoord.st);\n";

  shader += "gl_FragColor = vec4(0.0);\n";

  shader += "if (any(lessThan(enP.xyz,vec3(0.0))) ||\n";
  shader += "    any(greaterThan(enP.xyz,vec3(1.0)))) discard;\n";
  shader += "if (any(lessThan(exP.xyz,vec3(0.0))) ||\n";
  shader += "    any(greaterThan(exP.xyz,vec3(1.0)))) discard;\n";

  shader += "if (exP.a < 0.001 || enP.a < 0.001) discard;\n";

  shader += "vec3 exitPoint = exP.rgb;\n";
  shader += "vec3 entryPoint = enP.rgb;\n";

  shader += "vec3 dir = (exitPoint-entryPoint);\n";
  shader += "float totlen = length(dir);\n";
  shader += "if (totlen < 0.001) discard;\n";

  shader += "vec3 deltaDir = normalize(dir)*stepSize;\n";
  shader += "float deltaDirLen = length(deltaDir);\n";

  shader += "vec3 voxelCoord = entryPoint;\n";
  shader += "vec4 colorAcum = vec4(0.0);\n"; // The dest color
  shader += "float lengthAcum = 0.0;\n";

  shader += "bool gotFirstHit = false;\n";
  shader += "int nskipped = 0;\n"; 
  shader += "bool solid = false;\n";
  shader += "for(int i=0; i<int(length(exitPoint-entryPoint)/stepSize); i++)\n";
  shader += "{\n";

  shader += "  vec3 vC = voxelCoord*vsize;\n";
  shader += "  vec3 texCoord = vC*vec3(lod)+vcorner;\n";

  //---------------------------------
  // -- check filled boxes --
  shader += "  vec3 ftpos = texCoord/vec3(boxSize);\n";
  shader += "  bvec3 fclt = lessThan(floor(ftpos+0.5), ftpos);\n";
  shader += "  ftpos += vec3(fclt)*vec3(0.5);\n";
  shader += "  ftpos -= vec3(not(fclt))*vec3(0.5);\n";
  shader += "  if (texture3D(filledTex, ftpos/ftsize).x < 0.5)\n";
  shader += "   {\n";
  shader += "     vec3 r = texCoord-(ftpos+vec3(boxSize/2.0));\n";
  shader += "     float jump = dot(r, deltaDir);\n";
  shader += "     if (abs(jump) > 0.0)\n";
  shader += "       {\n";
  shader += "         vC += 2.0*jump*deltaDir + deltaDir;\n";
  shader += "         voxelCoord = vC/vsize;\n";
  shader += "         lengthAcum = length(voxelCoord-entryPoint);\n";
  shader += "       }\n";
  shader += "   }\n";
  //---------------------------------

  shader += "  vec4 colorSample = vec4(1.0);\n";

  shader += "  float val, gradlen;\n";
  shader += "  vec3 grad;\n";


  shader += "  float feather = 0.0;\n";
  if (cropPresent)
    shader += "  feather = crop(texCoord, false);\n";


  shader += "  if (feather < 0.5)\n";
  shader += "  {\n";
  // -- get exact texture coordinate so we don't get interpolation artefacts --
  if (nearest)
    {
      shader += "    bvec3 vclt = lessThan(floor(vC+0.5), vC);\n";
      shader += "    vC += vec3(vclt)*vec3(0.5);\n";
      shader += "    vC -= vec3(not(vclt))*vec3(0.5);\n";
      shader += "    vC /= vsize;\n";
      shader += "    val = texture3D(dataTex, vC).x;\n";
    }
  else
    shader += "    val = texture3D(dataTex, voxelCoord).x;\n";

  shader +=      getGrad();
  shader += "    gradlen = length(grad);\n";
  if (!bit16)
    shader += QString("    colorSample = texture2D(lutTex, vec2(val,gradlen*%1));\n").arg(1.0/Global::lutSize());
  else
    {
      shader += "   int h0 = int(65535.0*val);\n";
      shader += "   int h1 = h0 / 256;\n";
      shader += "   h0 = int(mod(float(h0),256.0));\n";
      shader += "   float fh0 = float(h0)/256.0;\n";
      shader += "   float fh1 = float(h1)/256.0;\n";
      shader += QString("    colorSample = texture2D(lutTex, vec2(fh0,fh1*%1));\n").\
	arg(1.0/Global::lutSize());
    }
  shader += "  }\n";
  shader += "  else\n";  // feather
  shader += "  {\n";
  shader += "    colorSample = vec4(0.0);\n";
  shader += "  }\n";  // feather


  shader += "  if (!gotFirstHit && colorSample.a > 0.001) gotFirstHit = true;\n";  

  shader += "  if (gotFirstHit && nskipped > skipLayers)\n";
  shader += "  {\n";
  shader += "    if (colorSample.a > 0.001 )\n";
  shader += "      {\n";  
  shader += "        gl_FragColor = vec4(voxelCoord,1.0);\n";
  shader += "        return;\n";
  shader += "      }\n";
  shader += "  }\n"; // gotfirsthit && nskipped > skipLayers

  shader += "  if (lengthAcum >= totlen )\n";
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
RcShaderFactory::genRaycastShader(bool nearest,
				  QList<CropObject> crops,
				  bool bit16)
{
  //------------------------------------
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
  //------------------------------------

  float lastSet = (Global::lutSize()-1.0)/(float)Global::lutSize();
  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler3D filledTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect exitTex;\n";
  shader += "uniform float stepSize;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform vec3 vcorner;\n";
  shader += "uniform vec3 vsize;\n";
  shader += "uniform int skipLayers;\n";
  shader += "uniform vec3 lightparm;\n";
  shader += "uniform sampler2DRect entryTex;\n";
  shader += "uniform vec3 ftsize;\n";
  shader += "uniform float boxSize;\n";  
  shader += "uniform sampler1D tagTex;\n";
  shader += "uniform bool mixTag;\n";
  shader += "uniform int skipVoxels;\n";
  shader += "uniform int maxSteps;\n";
  shader += "uniform sampler2DRect colorAcumTex;\n";
  shader += "uniform int lod;\n";
  shader += "uniform vec3 viewUp;\n";
  shader += "uniform vec3 viewRight;\n";
  shader += "uniform int aoLevel;\n";

  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops);

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "vec4 exP = texture2DRect(exitTex, gl_FragCoord.st);\n";
  shader += "vec4 enP = texture2DRect(entryTex, gl_FragCoord.st);\n";

  shader += "gl_FragData[0] = texture2DRect(colorAcumTex, gl_FragCoord.st);\n";
  shader += "gl_FragData[1] = vec4(0.0);\n";

  shader += "if (exP.a < 0.001 || enP.a < 0.001) return;\n";

  shader += "vec3 exitPoint = exP.rgb;\n";
  shader += "vec3 entryPoint = enP.rgb;\n";

  shader += "vec3 dir = (exitPoint-entryPoint);\n";
  shader += "float totlen = length(dir);\n";

  shader += "if (totlen < 0.001) { gl_FragData[1] = vec4(0.0); return; }\n";

  shader += "vec3 dirvec = normalize(dir);\n";
  shader += "vec3 deltaDir = dirvec*stepSize;\n";
  shader += "float deltaDirLen = length(deltaDir);\n";

  //----------------------
  // for shadows
  shader += "vec3 aoVec[9];\n";
  shader += "aoVec[0] = viewRight;\n";
  shader += "aoVec[1] = viewUp;\n";
  shader += "aoVec[2] = -aoVec[1];\n";
  shader += "aoVec[3] = -aoVec[2];\n";
  shader += "aoVec[4] = normalize( aoVec[1]+aoVec[2]);\n";
  shader += "aoVec[5] = normalize(-aoVec[1]+aoVec[2]);\n";
  shader += "aoVec[6] = normalize(-aoVec[1]-aoVec[2]);\n";
  shader += "aoVec[7] = normalize( aoVec[1]-aoVec[2]);\n";
  shader += "aoVec[8] = vec3(0.0,0.0,0.0);\n";
  shader += "for(int iao=0; iao<9; iao++)\n";
  shader += "  aoVec[iao] *= stepSize;\n";
  //----------------------

  shader += "vec3 voxelCoord = entryPoint;\n";
  shader += "float lengthAcum = 0.0;\n";
  shader += "vec4 colorAcum = gl_FragData[0];\n";
  
  shader += "bool gotFirstHit = false;\n";
  shader += "int nskipped = 0;\n"; 
  shader += "bool solid = false;\n";
 
  shader += "float lightop = 0.0;\n";
  shader += "float stplod = stepSize*max(vsize.x, max(vsize.y, vsize.z));\n";

  shader += "vec3 skipVoxStart = voxelCoord*vsize;\n";
  shader += "int lastvoxel = int(length(exitPoint-entryPoint)/stepSize);\n";
  shader += "int iend = int(min(float(maxSteps), float(lastvoxel)));\n";
  shader += "for(int i=0; i<iend; i++)\n";
  shader += "{\n";

  shader += "  vec4 colorSample = vec4(1.0);\n";

  // -- get exact texture coordinate so we don't get interpolation artifacts--
  shader += "  vec3 vC = voxelCoord*vsize;\n";
  shader += "  vec3 texCoord = vC*vec3(lod)+vcorner;\n";

  //---------------------------------
  // -- check filled boxes --
  shader += "  vec3 ftpos = texCoord/vec3(boxSize);\n";
  shader += "  bvec3 fclt = lessThan(floor(ftpos+0.5), ftpos);\n";
  shader += "  ftpos += vec3(fclt)*vec3(0.5);\n";
  shader += "  ftpos -= vec3(not(fclt))*vec3(0.5);\n";
  shader += "  if (texture3D(filledTex, ftpos/ftsize).x < 1.0)\n";
  shader += "   {\n";
  shader += "     vec3 r = texCoord-(ftpos+vec3(boxSize/2.0));\n";
  shader += "     float jump = dot(r, deltaDir);\n";
  shader += "     if (abs(jump) > 0.0)\n";
  shader += "       {\n";
  shader += "         vC += 2.0*jump*deltaDir + deltaDir;\n";
  shader += "         voxelCoord = vC/vsize;\n";
  shader += "         lengthAcum = length(voxelCoord-entryPoint);\n";
//  shader += "if (jump > 0.0)\n";
//  shader += " colorSample = vec4(jump*0.1,0,0,jump*0.1);\n";
//  shader += "else\n";
//  shader += " colorSample = vec4(0,0,-jump*0.1,-jump*0.1);\n";
//  shader += "colorAcum += (1.0 - colorAcum.a) * colorSample;\n";
  shader += "       }\n";
  shader += "   }\n";
  //---------------------------------

  shader += "   float val, gradlen;\n";
  shader += "   vec3 grad;\n";
  if (bit16)
    {
      shader += "  float fh0, fh1;\n";
    }

  shader += "  float feather = 0.0;\n";
  if (cropPresent)
    shader += "  feather = crop(texCoord, false);\n";


  shader += "  if (feather < 0.5)\n";
  shader += "  {\n";
  shader += "    bvec3 vclt = lessThan(floor(vC+0.5), vC);\n";
  shader += "    vC += vec3(vclt)*vec3(0.5);\n";
  shader += "    vC -= vec3(not(vclt))*vec3(0.5);\n";
  shader += "    vC /= vsize;\n";

  if (nearest)
    shader += "    val = texture3D(dataTex, vC).x;\n";
  else
    shader += "    val = texture3D(dataTex, voxelCoord).x;\n";

  shader +=      getGrad();
  shader += "    gradlen = length(grad);\n";
  if (!bit16)
    shader += QString("    colorSample = texture2D(lutTex, vec2(val,gradlen*%1));\n").arg(1.0/Global::lutSize());
  else
    {
      shader += "   int h0 = int(65535.0*val);\n";
      shader += "   int h1 = h0 / 256;\n";
      shader += "   h0 = int(mod(float(h0),256.0));\n";
      shader += "   fh0 = float(h0)/256.0;\n";
      shader += "   fh1 = float(h1)/256.0;\n";
      shader += QString("    colorSample = texture2D(lutTex, vec2(fh0,fh1*%1));\n").\
	arg(1.0/Global::lutSize());
    }

  shader += "    if (mixTag)\n";
  shader += "    {\n";
  if (!bit16)
    shader += "      vec4 tagcolor = texture1D(tagTex, val);\n";
  else
    shader += "      vec4 tagcolor = texture1D(tagTex, fh0);\n";
  shader += "      if (tagcolor.a > 0.0)\n";
  shader += "        colorSample.rgb = mix(colorSample.rgb, tagcolor.rgb, tagcolor.a);\n";
  shader += "      else\n";
  shader += "        colorSample = vec4(0.0);\n";
  shader += "    }\n";
  if (viewPresent)
    {
      if (!bit16)
	shader += QString("  vec2 vg = vec2(val,gradlen*%1);\n").arg(1.0/Global::lutSize());  
      else
	shader += QString("    vec2 vg = vec2(fh0, fh1*%1);\n").arg(1.0/Global::lutSize());  
      shader += "  vec4 cs=vec4(0.0);\n";
      shader += "  blend(true, texCoord, vg, cs);\n";
      if (!bit16)
	shader += QString("  colorSample = mix(colorSample, texture2D(lutTex, vec2(val,(gradlen+cs.y)*%1)), cs.x);\n").arg(1.0/Global::lutSize());
      else
	shader += QString("  colorSample = mix(colorSample, texture2D(lutTex, vec2(fh0,(fh1+cs.y)*%1)), cs.x);\n").arg(1.0/Global::lutSize());
    }
  shader += "  }\n";
  shader += "  else\n";  // cropped
  shader += "  {\n";
  shader += "    colorSample = vec4(0.0);\n";
  shader += "  }\n";  // feather


  shader += "  colorSample.a = 1.0-pow((1.0-colorSample.a),stplod);\n";

  shader += "  colorSample.rgb *= colorSample.a;\n";

  shader += "  if (!gotFirstHit && colorSample.a > 0.001)\n";  
  shader += "  {\n";
  shader += "    gotFirstHit = true;\n";
  shader += "    skipVoxStart = voxelCoord*vsize;\n";
  shader += "  }\n";

  shader += "  colorSample *= step(float(skipVoxels), length(voxelCoord*vsize-skipVoxStart));\n";

  shader += "  bool validcol = colorSample.a > 0.0;\n";

  shader += "  if (validcol)\n";
  shader += "  {\n";
  shader += "    if (gotFirstHit && nskipped > skipLayers)\n";
  shader += "    {\n";
  shader +=        addLighting();

  //--------------------------
  // ambient occlusion
  shader += "      if (aoLevel > 0 && colorSample.a > 0.01)\n";
  shader += "      {\n";
  shader += "       vec3 shdir = stepSize*normalize(dirvec + 3.0*(dirvec-viewDir));\n";
  shader += "       int shend = aoLevel*9;\n";
  shader += "       float rd = 1.0/sqrt(float(aoLevel));\n";
  shader += "       float okval = 0.0;\n";
  shader += "       for(int sh=0; sh<shend; sh++)\n";
  shader += "        {\n";
  shader += "         int idx = int(mod(float(sh), 9.0));\n";
  shader += "         vec3 vxc = voxelCoord - rd*float(sh)*shdir + rd*float(sh)*aoVec[idx];\n";
  shader += "         float vv = texture3D(dataTex, vxc).x;\n";
  if (!bit16)
    shader += "         float valpha = texture2D(lutTex, vec2(vv, 0.0)).a;\n";
  else
    {
      shader += "         int v0 = int(65535.0*vv);\n";
      shader += "         int v1 = v0 / 256;\n";
      shader += "         v0 = int(mod(float(v0),256.0));\n";
      shader += "         float eh0 = float(v0)/256.0;\n";
      shader += "         float eh1 = float(v1)/256.0;\n";
      shader += "         float valpha = texture2D(lutTex, vec2(eh0, eh1)).a;\n";
    }
  shader += "         valpha *= step(float(skipVoxels+5), length(vxc*vsize-skipVoxStart));\n";
  shader += "         float okv = step(float(sh)*0.001, valpha);\n";
  shader += "         okval += okv;\n";
  shader += "        }\n";
  shader += "        okval /= float(shend);\n";
  shader += "        colorSample.rgb *= (1.0-clamp(okval, 0.0, 0.7));\n";
  shader += "      }\n";
  //--------------------------

  //--------------------------
  // add emissive color
  if (!bit16)
    shader += QString("      vec4 emisColor = texture2D(lutTex, vec2(val,gradlen*%1+%2));\n"). \
            arg(1.0/Global::lutSize()).arg(lastSet);
  else
    shader += QString("      vec4 emisColor = texture2D(lutTex, vec2(fh0,fh1*%1+%2));\n"). \
            arg(1.0/Global::lutSize()).arg(lastSet);
  shader += "      emisColor.a = 1.0-pow((1.0-emisColor.a),stplod);\n";
  shader += "      emisColor.rgb *= emisColor.a;\n";
  shader += "      colorSample.rgb += emisColor.rgb;\n";
  //--------------------------

  shader += "      colorAcum += (1.0 - colorAcum.a) * colorSample;\n";

  shader += "  }\n"; // validcol

  shader += "    }\n"; // gotfirsthit && nskipped > skipLayers


  shader += "  if (lengthAcum >= totlen )\n";
  shader += "    {\n";
  shader += "      gl_FragData[1] = vec4(0.0);\n";
  shader += "      break;\n";  // terminate if opacity > 1 or the ray is outside the volume	
  shader += "    }\n";
  shader += "  else if (colorAcum.a > 1.0)\n";
  shader += "    {\n";
  shader += "      gl_FragData[1] = vec4(0.0);\n";
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

  shader += "gl_FragData[0] = colorAcum;\n";
  shader += "if (iend < lastvoxel)\n";
  shader += "  gl_FragData[1] = vec4(voxelCoord-deltaDir,1.0);\n";

  shader += "}\n";

  return shader;
}

QString
RcShaderFactory::genXRayShader(bool nearest,
			       QList<CropObject> crops,
			       bool bit16)
{
  //------------------------------------
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
  //------------------------------------

  QString shader;

  shader =  "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler3D filledTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect exitTex;\n";
  shader += "uniform float stepSize;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 viewDir;\n";
  shader += "uniform vec3 vcorner;\n";
  shader += "uniform vec3 vsize;\n";
  shader += "uniform int skipLayers;\n";
  shader += "uniform vec3 lightparm;\n";
  shader += "uniform sampler2DRect entryTex;\n";
  shader += "uniform vec3 ftsize;\n";
  shader += "uniform float boxSize;\n";  
  shader += "uniform int skipVoxels;\n";
  shader += "uniform int maxSteps;\n";
  shader += "uniform sampler2DRect colorAcumTex;\n";
  shader += "uniform int lod;\n";
  shader += "uniform vec3 viewUp;\n";
  shader += "uniform vec3 viewRight;\n";
  shader += "uniform int aoLevel;\n";

  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops);

  shader += "void main(void)\n";
  shader += "{\n";


  shader += "vec4 exP = texture2DRect(exitTex, gl_FragCoord.st);\n";
  shader += "vec4 enP = texture2DRect(entryTex, gl_FragCoord.st);\n";

  shader += "gl_FragData[0] = texture2DRect(colorAcumTex, gl_FragCoord.st);\n";
  shader += "gl_FragData[1] = vec4(0.0);\n";

  shader += "if (exP.a < 0.001 || enP.a < 0.001) return;\n";

  shader += "vec3 exitPoint = exP.rgb;\n";
  shader += "vec3 entryPoint = enP.rgb;\n";

  shader += "vec3 dir = (exitPoint-entryPoint);\n";
  shader += "float totlen = length(dir);\n";

  shader += "if (totlen < 0.001) { gl_FragData[1] = vec4(0.0); return; }\n";

  shader += "vec3 normDir = normalize(dir);\n";
  shader += "vec3 deltaDir = normDir*stepSize;\n";
  shader += "float deltaDirLen = length(deltaDir);\n";

  shader += "vec3 voxelCoord = entryPoint;\n";
  shader += "float lengthAcum = 0.0;\n";

  shader += "vec4 colorAcum = gl_FragData[0];\n";

  shader += "bool gotFirstHit = false;\n";
  shader += "int nskipped = 0;\n"; 
  shader += "bool solid = false;\n";

  shader += "vec3 skipVoxStart = vec3(0.0);\n";
  shader += "int lastvoxel = int(length(exitPoint-entryPoint)/stepSize);\n";
  shader += "int iend = int(min(float(maxSteps), float(lastvoxel)));\n";
  shader += "for(int i=0; i<iend; i++)\n";

  shader += "{\n";

  // -- get exact texture coordinate so we don't get interpolation artefacts --
  shader += "  vec3 vC = voxelCoord*vsize;\n";
  shader += "  vec3 texCoord = vC*vec3(lod)+vcorner;\n";

  //---------------------------------
  // -- check filled boxes --
  shader += "  vec3 ftpos = texCoord/vec3(boxSize);\n";
  shader += "  bvec3 fclt = lessThan(floor(ftpos+0.5), ftpos);\n";
  shader += "  ftpos += vec3(fclt)*vec3(0.5);\n";
  shader += "  ftpos -= vec3(not(fclt))*vec3(0.5);\n";
  shader += "  if (texture3D(filledTex, ftpos/ftsize).x < 0.5)\n";
  shader += "   {\n";
  shader += "     vec3 r = texCoord-(ftpos+vec3(boxSize/2.0));\n";
  shader += "     float jump = dot(r, deltaDir);\n";
  shader += "     if (abs(jump) > 0.0)\n";
  shader += "       {\n";
  shader += "         vC += 2.0*jump*deltaDir + deltaDir;\n";
  shader += "         voxelCoord = vC/vsize;\n";
  shader += "         lengthAcum = length(voxelCoord-entryPoint);\n";
  shader += "       }\n";
  shader += "   }\n";
  //---------------------------------

  shader += "  vec4 colorSample = vec4(1.0);\n";
  shader += "   float val;\n";
  shader += "   vec3 grad;\n";

  shader += "  float feather = 0.0;\n";
  if (cropPresent)
    shader += "  feather = crop(texCoord, false);\n";

  shader += "  if (feather < 0.5)\n";
  shader += "  {\n";
  shader += "    bvec3 vclt = lessThan(floor(vC+0.5), vC);\n";
  shader += "    vC += vec3(vclt)*vec3(0.5);\n";
  shader += "    vC -= vec3(not(vclt))*vec3(0.5);\n";
  shader += "    vC /= vsize;\n";

  if (nearest)
    shader += "    val = texture3D(dataTex, vC).x;\n";
  else
    shader += "    val = texture3D(dataTex, voxelCoord).x;\n";

  shader +=      getGrad();
  shader += QString("    colorSample = texture2D(lutTex, vec2(val,length(grad)*%1));\n").arg(1.0/Global::lutSize());
  if (viewPresent)
    {
      shader += QString("  vec2 vg = vec2(val,length(grad)*%1);\n").arg(1.0/Global::lutSize());  
      shader += "  vec4 cs=vec4(0.0);\n";
      shader += "  blend(true, texCoord, vg, cs);\n";
      shader += QString("  colorSample = mix(colorSample, texture2D(lutTex, vec2(val,(length(grad)+cs.y)*%1)), cs.x);\n").arg(1.0/Global::lutSize());
    }
  shader += "  }\n";
  shader += "  else\n";  // feather
  shader += "  {\n";
  shader += "    colorSample = vec4(0.0);\n";
  shader += "  }\n";  // feather


  shader += "  if (!gotFirstHit && colorSample.a > 0.001)\n";  
  shader += "  {\n";
  shader += "    gotFirstHit = true;\n";
  shader += "    skipVoxStart = voxelCoord*vsize;\n";
  shader += "  }\n";

  shader += "  if (gotFirstHit)\n";
  shader += "    colorSample *= step(float(skipVoxels), length(voxelCoord*vsize-skipVoxStart));\n";

  shader += "  if (gotFirstHit && nskipped > skipLayers)\n";
  shader += "  {\n";

  // modulate opacity by angle wrt view direction
  shader += "    if (length(grad) > 0.1)\n";
  shader += "      {\n";
  shader += "        grad = normalize(grad);\n";
  shader += "        colorSample.a *= (0.3+0.9*(1.0-abs(dot(grad,normDir))));\n";
  shader += "      }\n";
  shader += "    else\n";
  shader += "      colorSample.a *= 0.2;\n";

  // reduce the opacity
  shader += "    colorSample.a *= 0.1;\n";
  shader += "    colorSample.rgb *= colorSample.a;\n";

  // just add up, don't composite
  shader += "    colorAcum += colorSample;\n";

  shader += "  }\n"; // gotfirsthit && nskipped > skipLayers

  shader += "  if (lengthAcum >= totlen )\n";
  shader += "    {\n";
  shader += "      gl_FragData[1] = vec4(0.0);\n";
  shader += "      break;\n";  // terminate if opacity > 1 or the ray is outside the volume	
  shader += "    }\n";
  shader += "  else if (colorAcum.a > 1.0)\n";
  shader += "    {\n";
  shader += "      gl_FragData[1] = vec4(0.0);\n";
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

  shader += "gl_FragData[0] = colorAcum;\n";
  shader += "if (iend < lastvoxel)\n";
  shader += "  gl_FragData[1] = vec4(voxelCoord-deltaDir,1.0);\n";

  shader += "}\n";

  return shader;
}


//------------------------------
// shaders from drishti paint
//------------------------------
QString
RcShaderFactory::clip()
{
  QString shader;

  shader += "vec3 clip(vec3 pt0, vec3 dir, vec3 vcorner, vec3 vsize, vec3 voxelScale)\n";
  shader += "{\n";
  shader += " vec3 pt = pt0;\n";
  shader += " vec3 nomod = vsize/max(vsize.x, max(vsize.y, vsize.z));\n";
  shader += " if (nclip > 0)\n";
  shader += "  {\n";
  shader += "    for(int c=0; c<nclip; c++)\n";
  shader += "      {\n";
  shader += "        vec3 cpos = clipPos[c];\n";
  shader += "        vec3 cnorm = clipNormal[c];\n";
  shader += "        cpos = (cpos-vcorner)/vsize;\n"; 
  shader += "        cpos /= voxelScale;\n"; 
  shader += "        cnorm *= nomod;\n";
  shader += "        cnorm *= voxelScale;\n"; 
  shader += "        float deno = dot(dir, cnorm);\n";
  shader += "        if (deno > 0.0)\n";
  shader += "          {\n";
  shader += "            float t = -dot((pt-cpos),cnorm)/deno;\n";
  shader += "            if (t >= 0)\n";
  shader += "             {\n";
  shader += "               pt = pt + t*dir;\n";
  shader += "             }\n";
  shader += "          }\n";
  shader += "      }\n";
  shader += "  }\n";
  shader += " return pt;\n";
  shader += "}\n";

  return shader;
}

QString
RcShaderFactory::getExactVoxelCoord()
{
  QString shader;
  shader += "vec3 exactVoxelCoordinate(vec3 voxelCoord, vec3 vsize)\n";
  shader += "{\n";
  shader += "  vec3 vC = voxelCoord*vsize;\n";
  shader += "  bvec3 vclt = lessThan(floor(vC+0.5), vC);\n";
  shader += "  vC += vec3(vclt)*vec3(0.5);\n";
  shader += "  vC -= vec3(not(vclt))*vec3(0.5);\n";
  shader += "  vC /= vsize;\n";
  shader += "  return vC;\n";
  shader += "}\n";  

  return shader;
}

QString
RcShaderFactory::genRaycastShader_1(bool nearest,
				    QList<CropObject> crops,
				    bool bit16)
{
  //------------------------------------
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
  //------------------------------------

  QString shader;
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler2DRect exitTex;\n";
  shader += "uniform sampler2DRect entryTex;\n";
  shader += "uniform sampler1D tagTex;\n";
  shader += "uniform float stepSize;\n";
  shader += "uniform vec3 eyepos;\n";
  shader += "uniform vec3 vcorner;\n";
  shader += "uniform vec3 vsize;\n";
  shader += "uniform vec3 voxelScale;\n";
  shader += "uniform bool saveCoord;\n";
  shader += "uniform int skipLayers;\n";
  shader += "uniform int skipVoxels;\n";
  shader += "uniform bool mixTag;\n";
  shader += "uniform int nclip;\n";
  shader += "uniform vec3 clipPos[5];\n";
  shader += "uniform vec3 clipNormal[5];\n";
  shader += "uniform float maxDeep;\n";
  shader += "uniform float minGrad;\n";
  shader += "uniform float maxGrad;\n";
  shader += "uniform float sslevel;\n";
  shader += "uniform mat4 MVP;\n";
  
  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);
  if (viewPresent) shader += BlendShaderFactory::generateBlend(crops);

  
  //---------------------
  // apply clip planes to modify entry and exit points
  shader += clip();
  //---------------------

  //---------------------
  // gradient and gradient magnitude evaluation
  //shader += getGrad();
  //---------------------

  //---------------------
  // voxel coordinate for saving coordinate or near neighbour interpolation
  shader += getExactVoxelCoord();
  //---------------------

  shader += "void main(void)\n";
  shader += "{\n";

  shader += "vec4 exP = texture2DRect(exitTex, gl_FragCoord.st);\n";
  shader += "vec4 enP = texture2DRect(entryTex, gl_FragCoord.st);\n";

  shader += "gl_FragData[0] = vec4(0.0);\n";
  shader += "gl_FragData[1] = vec4(0.0);\n";

  shader += "if (exP.a < 0.001 || enP.a < 0.001) return;\n";

  shader += "vec3 exitPoint = exP.rgb;\n";
  shader += "vec3 entryPoint = enP.rgb;\n";

  //==========
  shader += "entryPoint =  entryPoint/voxelScale;\n";
  shader += "entryPoint = (entryPoint - vcorner)/vsize;\n";
  shader += "exitPoint =  exitPoint/voxelScale;\n";
  shader += "exitPoint = (exitPoint - vcorner)/vsize;\n";
  //==========
  
  shader += "vec3 onev;\n";
  shader += "onev = vec3(sslevel)/vsize;\n";
  
  shader += "vec3 dir = (exitPoint-entryPoint);\n";

  shader += "entryPoint= clip(entryPoint, dir, vcorner, vsize, voxelScale);\n";
  shader += "exitPoint = clip(exitPoint, -dir, vcorner, vsize, voxelScale);\n";

  
  shader += "vec3 dirN = (exitPoint-entryPoint);\n";
  shader += "if (dot(dir, dirN) <= 0) return;\n";
  shader += "dir = dirN;\n";

  shader += "float totlen = length(dir);\n";
  shader += "if (totlen < 0.001) return;\n";

  shader += "float checkGrad = (minGrad > 0.0 || maxGrad < 1.0) ? 1.0 : 0.0;\n";

  shader += "vec3 deltaDir = normalize(dir)*stepSize;\n";
  shader += "float deltaDirLen = length(deltaDir);\n";

  shader += "vec3 voxelCoord = entryPoint;\n";
  shader += "vec4 colorAcum = vec4(0.0);\n"; // The dest color
  shader += "float lengthAcum = 0.0;\n";

  //----------------
  // modify coordinate so that it aligns well with respect to eye
  shader += "{\n";
  shader += " vec3 voxpos = vcorner + voxelCoord*vsize;\n";
  shader += " vec3 I = voxpos - eyepos;\n";
  shader += " float z = length(I)/stepSize;\n";
  shader += " I = normalize(I)*stepSize;\n";
  shader += " voxelCoord = (eyepos + floor(z)*I - vcorner)/vsize;\n";
  shader += "}\n";
  //----------------
  
  shader += "float gotFirstHit = 0;\n";
  shader += "vec3 skipVoxStart = vec3(0.0);\n";

  shader += "int iend = int(length(exitPoint-entryPoint)/stepSize);\n";
  
  //-------------------------
  // get the first hit
  //-------------------------
  shader += "int istart = 0;\n";
  shader += "for(int i=0; i<iend; i++)\n";
  shader += "{\n";
  shader += "  istart = i;\n";
  shader += "  float val = texture3D(dataTex, voxelCoord).x;\n";
  shader += "  vec4 colorSample = vec4(0.0);\n";

  if (!bit16)
    {
      shader += "  colorSample = texture2D(lutTex, vec2(val,0.0));\n";
    }
  else
    {
      shader += "  int h0 = int(65535.0*val);\n";
      shader += "  int h1 = h0 / 256;\n";
      shader += "  h0 = int(mod(float(h0),256.0));\n";
      shader += "  float fh0 = float(h0)/256.0;\n";
      shader += "  float fh1 = float(h1)/256.0;\n";
      shader += "  colorSample = texture2D(lutTex, vec2(fh0,fh1));\n";
    }

//  // find gradient magnitude
//  shader += "  if (checkGrad > 0 && colorSample.a > 0.001)\n";  
//  shader += "    {\n";
//  shader += "      float gradMag = gradMagnitude(voxelCoord, onev);\n";
//  shader += "      colorSample = mix(vec4(0.0), colorSample,\n";
//  shader += "                        step(minGrad, gradMag)*step(gradMag, maxGrad));\n";
//  shader += "    }\n";

  // reduce opacity
  shader += " colorSample.a *= colorSample.a;\n";


  if (cropPresent)
    {
      shader += "  float feather = 0.0;\n";
      shader += "  vec3 vC = voxelCoord*vsize;\n";
      shader += "  vec3 texCoord = vC*vec3(sslevel)+vcorner;\n";
      shader += "  feather = crop(texCoord, false);\n";
      shader += "  if (feather > 0.5)\n";
      shader += "    colorSample.a = 0.0;\n"; // cropped
    }
  
  // --- handle tags ---
  shader += "    if (mixTag)\n";
  shader += "    {\n";
  if (!bit16)
    shader += "      vec4 tagcolor = texture1D(tagTex, val);\n";
  else
    shader += "      vec4 tagcolor = texture1D(tagTex, fh0);\n";
  shader += "      if (tagcolor.a > 0.0)\n";
  shader += "        colorSample.rgb = mix(colorSample.rgb, tagcolor.rgb, tagcolor.a);\n";
  shader += "      else\n";
  shader += "        colorSample = vec4(0.0);\n";
  shader += "    }\n";
  // --------------------
  
  
  shader += "  if (gotFirstHit < 1.0 && colorSample.a > 0.001)\n";  
  shader += "  {\n";
  // use bisection method to get a more finer result
  shader += "    vec3 dD = -deltaDir*0.5;\n";
  shader += "    for (int b=0; b<5; b++)\n";
  shader += "    {\n";
  shader += "      voxelCoord += dD;\n";
  shader += "      val = texture3D(dataTex, voxelCoord).x;\n";
  shader += "      vec4 cS = texture2D(lutTex, vec2(val,0.0));\n";
  shader += "      dD = dD*0.5;\n";
  shader += "      dD *= mix(-1.0, 1.0, step(0.001, cS.a));\n";
  shader += "      colorSample = mix(colorSample, cS, step(0.001, cS.a));\n";
  shader += "    }\n";
  shader += "    gotFirstHit = 1.0;\n";
  shader += "    skipVoxStart = voxelCoord*vsize;\n";
  shader += "    i = iend;\n";
  shader += "    if (saveCoord)";
  shader += "      {\n";
  shader += "        vec3 vC = exactVoxelCoordinate(voxelCoord, vsize);\n";
  shader += "        gl_FragData[0] = vec4(vC,1.0);\n";
  shader += "        return;\n";
  shader += "      }\n";
  shader += "  }\n";
  shader += "  else\n";
  shader += "  {\n";
  shader += "    voxelCoord += deltaDir;\n";
  shader += "    lengthAcum += deltaDirLen;\n";
  shader += "  }\n";

  shader += "}\n";  // wait to get the first hit
  //-------------------------
  //-------------------------
  
  //-------------------------
  // we have now got the first hit
  //-------------------------
//  // change max ray depth based on screen position
//  shader += "float fcx = (gl_FragCoord.x-swidth*0.5);\n";
//  shader += "float fcy = (gl_FragCoord.y-sheight*0.5);\n";
//  shader += "float sigma = max(swidth*0.2, sheight*0.2);\n";
//  shader += "float fcd = exp(-(fcx*fcx + fcy*fcy)/(2.0*sigma*sigma));\n";
//  shader += "float maxDepth = maxDeep*fcd;\n";
  shader += "float maxDepth = maxDeep;\n";

  shader += "int nskipped = 0;\n"; 
  shader += "float solid = 0;\n";
  shader += "float deep = 0.0;\n";
  shader += "for(int i=istart; i<iend; i++)\n";
  shader += "{\n";
  shader += "  float val = texture3D(dataTex, voxelCoord).x;\n";
  shader += "  vec4 colorSample = vec4(0.0);\n";

  if (!bit16)
    {
      shader += "  colorSample = texture2D(lutTex, vec2(val,0.0));\n";
    }
  else
    {
      shader += "  int h0 = int(65535.0*val);\n";
      shader += "  int h1 = h0 / 256;\n";
      shader += "  h0 = int(mod(float(h0),256.0));\n";
      shader += "  float fh0 = float(h0)/256.0;\n";
      shader += "  float fh1 = float(h1)/256.0;\n";
      shader += "  colorSample = texture2D(lutTex, vec2(fh0,fh1));\n";
    }

//  // find gradient magnitude
//  shader += "  if (checkGrad > 0 && colorSample.a > 0.001)\n";  
//  shader += "    {\n";
//  shader += "      float gradMag = gradMagnitude(voxelCoord, onev);\n";
//  shader += "      colorSample = mix(vec4(0.0), colorSample,\n";
//  shader += "                        step(minGrad, gradMag)*step(gradMag, maxGrad));\n";
//  shader += "    }\n";
  
  // reduce opacity
  shader += "  colorSample.a *= colorSample.a;\n";

  // --- handle tags ---
  shader += "    if (mixTag)\n";
  shader += "    {\n";
  if (!bit16)
    shader += "      vec4 tagcolor = texture1D(tagTex, val);\n";
  else
    shader += "      vec4 tagcolor = texture1D(tagTex, fh0);\n";
  shader += "      if (tagcolor.a > 0.0)\n";
  shader += "        colorSample.rgb = mix(colorSample.rgb, tagcolor.rgb, tagcolor.a);\n";
  shader += "      else\n";
  shader += "        colorSample = vec4(0.0);\n";
  shader += "    }\n";
  // --------------------
  
  // ----------------------------
  shader += "  colorSample *= step(float(skipVoxels), length(voxelCoord*vsize-skipVoxStart));\n";
  // ----------------------------

  
  if (cropPresent)
    {
      shader += "  float feather = 0.0;\n";
      shader += "  vec3 vC = voxelCoord*vsize;\n";
      shader += "  vec3 texCoord = vC*vec3(sslevel)+vcorner;\n";
      shader += "  feather = crop(texCoord, false);\n";
      shader += "  if (feather > 0.5)\n";
      shader += "    colorSample.a = 0.0;\n"; // cropped
    }
  

  shader += "  if (nskipped > skipLayers)\n";
  shader += "  {\n";
  shader += "  if (colorSample.a > 0.001 || deep > 0.0)\n";
  shader += "    {\n";  
  shader += "      colorSample.rgb *= colorSample.a;\n";
  shader += "      colorSample.rgb *= (1.0-colorAcum.a);\n";
  shader += "      colorAcum += (1.0 - colorAcum.a) * colorSample;\n";
  shader += "      deep = deep + 1.0;\n";
  shader += "      if (deep < 1.1)\n"; // first hit
  shader += "       {\n";
  shader += "         vec3 voxpos = (vcorner + voxelCoord*vsize)*voxelScale;";
  shader += "         vec4 scrpos = MVP * vec4(voxpos, 1);\n";
  shader += "         float z = ((gl_DepthRange.diff * scrpos.z/scrpos.w) + \n";
  shader += "                     gl_DepthRange.near + gl_DepthRange.far) / 2.0;\n";
  shader += "         gl_FragDepth = z;\n";
  shader += "         float zLinear = length(vcorner+voxelCoord*vsize - eyepos);\n";
  shader += "         gl_FragData[1] = vec4(z,val,zLinear,1.0);\n";
  shader += "       }\n";
  shader += "      if (colorAcum.a > 0.95 || deep >= maxDepth)\n";
  shader += "       {\n";
  shader += "         gl_FragData[0] = colorAcum/colorAcum.a;\n";
  shader += "         return;\n";
  shader += "       }\n";
  shader += "    }\n"; // colorsample || deep
  shader += "  }\n"; // gotfirsthit && nskipped > skipLayers

  shader += "  if (lengthAcum >= totlen )\n";
  shader += "      break;\n";  // terminate if opacity > 1 or the ray is outside the volume	

  shader += "  voxelCoord += deltaDir;\n";
  shader += "  lengthAcum += deltaDirLen;\n";

  shader += "  if (colorSample.a > 0.001)\n";
  shader += "   {\n";  
  shader += "      if (solid < 1.0)\n";
  shader += "        {\n";    
  shader += "          solid = 1.0;\n";
  shader += "          nskipped++;\n";  
  shader += "        }\n";  
  shader += "   }\n";
  shader += "  else\n";
  shader += "   {\n";  
  shader += "      if (solid > 0.0)\n";
  shader += "        {\n";    
  shader += "          solid = 0.0;\n";
  shader += "        }\n";  
  shader += "   }\n";

  shader += "}\n";

  shader += "if (colorAcum.a > 0.001) gl_FragData[0] = colorAcum/colorAcum.a;\n";

  shader += "}\n";

  return shader;
}

QString
RcShaderFactory::genEdgeEnhanceShader_1()
{
  QString shader;

  shader += "#version 420 core\n";
  shader += "in vec3 pointpos;\n";
  shader += "out vec4 glFragColor;\n";
  
  shader += "uniform sampler2DRect normalTex;\n";
  shader += "uniform sampler2DRect pvtTex;\n";
  shader += "uniform float minZ;\n";
  shader += "uniform float maxZ;\n";
  shader += "uniform float dzScale;\n";
  shader += "uniform float isoshadow;\n";

  shader += "void main(void)\n";
  shader += "{\n";
  shader += "  glFragColor = vec4(0.0);\n";
  
  shader += "  vec2 spos0 = gl_FragCoord.xy;\n";

  shader += "  vec4 color = texture2DRect(pvtTex, spos0);\n";
  shader += "  vec3 dvt = texture2DRect(normalTex, spos0).xyz;\n";

  shader += "  gl_FragDepth = dvt.x;\n";
  
  //---------------------
  shader += "  if (color.a < 0.01)\n";
  shader += "    discard;\n";
  //---------------------

  shader += "    float cx[8] = float[](-1.0, 0.0, 1.0, 0.0, -1.0,-1.0, 1.0, 1.0);\n";
  shader += "    float cy[8] = float[]( 0.0,-1.0, 0.0, 1.0, -1.0, 1.0,-1.0, 1.0);\n";
  
  shader+= " if (isoshadow > 0.0)\n"; // soft shadows
  shader+= " {\n";
  shader += "  float depth = dvt.z;\n";
  shader+= "   float sum = 0.0;\n";
  shader+= "   float tele = 0.0;\n";
  shader+= "   int j = 0;\n";
  shader+= "   int nsteps = int(10.0*isoshadow);\n";
  shader+= "   for(int i=0; i<nsteps; i++)\n";
  shader+= "    {\n";
  shader+= "	  float r = 1.0 + float(i)/10.0;\n";
  shader+= "	  vec2 pos = spos0 + vec2(r*cx[int(mod(i,8))],r*cy[int(mod(i,8))]);\n";
  shader+= "	  float od = depth - texture2DRect(normalTex, pos).z;\n";
  shader+= "	  sum += step(3.0, od);\n";
  shader+= "	 tele ++;\n";
  shader+= "    } \n";
  shader+= "   sum /= tele;\n";
  shader+= "   sum = 1.0-sum;\n";
  shader+= "   color.rgb *= sum;\n";
  shader+= " }\n";

  
  shader += "  if (dzScale > 0.0)\n"; // edges
  shader += "  {\n";
  shader += "    vec2 sx1 = spos0+vec2(1.0,0.0);\n";
  shader += "    vec2 sy1 = spos0+vec2(0.0,1.0);\n";
  shader += "    vec2 sx0 = spos0-vec2(1.0,0.0);\n";
  shader += "    vec2 sy0 = spos0-vec2(0.0,1.0);\n";
  shader += "    float dx = texture2DRect(normalTex, sx1).z - texture2DRect(normalTex, sx0).z;\n";
  shader += "    float dy = texture2DRect(normalTex, sy1).z - texture2DRect(normalTex, sy0).z;\n";
  shader += "    float zedge = (maxZ-minZ)*0.5/dzScale;\n";
  shader += "    vec3 norm = normalize(vec3(dx, dy, (zedge*zedge)/(maxZ-minZ)));\n";  
  shader += "    color.rgb *= norm.z;\n";
  shader += "  }\n";
    
  shader += " if (any(greaterThan(color.rgb,vec3(1.0)))) \n";
  shader += "   color.rgb = vec3(1.0);\n";
  shader += "  glFragColor = color;\n";

  shader += "}\n";

  return shader;
}
//----------------------------
//----------------------------

GLint RcShaderFactory::m_boxShaderParm[20];
GLint* RcShaderFactory::boxShaderParm() { return &m_boxShaderParm[0]; }

GLuint RcShaderFactory::m_boxShader = 0;
GLuint RcShaderFactory::boxShader()
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
RcShaderFactory::boxShaderV()
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
RcShaderFactory::boxShaderF()
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
RcShaderFactory::loadShader(GLhandleARB &progObj,
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
//---------------
