#include "shaderfactory.h"

#include <QTextEdit>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDebug>
#include <QDialog>
#include <QRegularExpression>
#include <QSet>

#include "cropshaderfactory.h"

namespace
{
QString sourceWithoutComments(const QString &source)
{
  QString code;
  code.reserve(source.size());
  bool lineComment = false;
  bool blockComment = false;

  for (int i=0; i<source.size(); ++i)
    {
      const QChar c = source.at(i);
      const QChar next = (i+1 < source.size() ? source.at(i+1) : QChar());
      if (lineComment)
        {
          if (c == '\n')
            {
              lineComment = false;
              code += c;
            }
          else
            code += ' ';
          continue;
        }
      if (blockComment)
        {
          if (c == '*' && next == '/')
            {
              blockComment = false;
              code += "  ";
              ++i;
            }
          else
            code += (c == '\n' ? '\n' : ' ');
          continue;
        }
      if (c == '/' && next == '/')
        {
          lineComment = true;
          code += "  ";
          ++i;
        }
      else if (c == '/' && next == '*')
        {
          blockComment = true;
          code += "  ";
          ++i;
        }
      else
        code += c;
    }

  return code;
}

bool usesLegacyBuiltins(const QString &source)
{
  QSet<QString> legacy;
  legacy << "attribute" << "varying" << "ftransform"
         << "gl_Vertex" << "gl_Normal" << "gl_Color"
         << "gl_SecondaryColor" << "gl_FogCoord"
         << "gl_MultiTexCoord0" << "gl_MultiTexCoord1"
         << "gl_MultiTexCoord2" << "gl_MultiTexCoord3"
         << "gl_MultiTexCoord4" << "gl_MultiTexCoord5"
         << "gl_MultiTexCoord6" << "gl_MultiTexCoord7"
         << "gl_ModelViewMatrix" << "gl_ModelViewMatrixInverse"
         << "gl_ModelViewMatrixTranspose" << "gl_ModelViewMatrixInverseTranspose"
         << "gl_ProjectionMatrix" << "gl_ProjectionMatrixInverse"
         << "gl_ProjectionMatrixTranspose" << "gl_ProjectionMatrixInverseTranspose"
         << "gl_ModelViewProjectionMatrix" << "gl_ModelViewProjectionMatrixInverse"
         << "gl_ModelViewProjectionMatrixTranspose"
         << "gl_ModelViewProjectionMatrixInverseTranspose"
         << "gl_TextureMatrix" << "gl_TextureMatrixInverse"
         << "gl_TextureMatrixTranspose" << "gl_TextureMatrixInverseTranspose"
         << "gl_NormalMatrix" << "gl_NormalScale" << "gl_ClipPlane"
         << "gl_Point" << "gl_FrontMaterial" << "gl_BackMaterial"
         << "gl_LightSource" << "gl_LightModel"
         << "gl_FrontLightModelProduct" << "gl_BackLightModelProduct"
         << "gl_FrontLightProduct" << "gl_BackLightProduct"
         << "gl_TextureEnvColor" << "gl_EyePlaneS" << "gl_EyePlaneT"
         << "gl_EyePlaneR" << "gl_EyePlaneQ" << "gl_ObjectPlaneS"
         << "gl_ObjectPlaneT" << "gl_ObjectPlaneR" << "gl_ObjectPlaneQ"
         << "gl_Fog" << "gl_FrontColor" << "gl_BackColor"
         << "gl_FrontSecondaryColor" << "gl_BackSecondaryColor"
         << "gl_TexCoord" << "gl_FogFragCoord" << "gl_ClipVertex"
         << "gl_FragColor" << "gl_FragData"
         << "texture1D" << "texture1DProj" << "texture1DLod"
         << "texture1DProjLod" << "texture2D" << "texture2DProj"
         << "texture2DLod" << "texture2DProjLod" << "texture3D"
         << "texture3DProj" << "texture3DLod" << "texture3DProjLod"
         << "textureCube" << "textureCubeLod" << "texture2DRect"
         << "texture2DRectProj" << "shadow1D" << "shadow1DProj"
         << "shadow1DLod" << "shadow1DProjLod" << "shadow2D"
         << "shadow2DProj" << "shadow2DLod" << "shadow2DProjLod"
         << "shadow2DRect" << "shadow2DRectProj";

  const QString code = sourceWithoutComments(source);
  for (int i=0; i<code.size(); )
    {
      if (!(code.at(i).isLetter() || code.at(i) == '_'))
        {
          ++i;
          continue;
        }
      const int start = i++;
      while (i<code.size() && (code.at(i).isLetterOrNumber() || code.at(i) == '_'))
        ++i;
      if (legacy.contains(code.mid(start, i-start)))
        return true;
    }
  return false;
}

QString normalizedShaderSource(QString source)
{
  if (!usesLegacyBuiltins(source))
    return source;

  const QRegularExpression versionCore(
    "(^|\\n)([ \\t\\x{FEFF}]*#[ \\t]*version[ \\t]+[0-9]+)[ \\t]+core\\b");
  const QRegularExpressionMatch match = versionCore.match(sourceWithoutComments(source));
  if (match.hasMatch())
    source.replace(match.capturedStart(), match.capturedLength(),
                   match.captured(1) + match.captured(2) + " compatibility");
  return source;
}

QString shaderLabel(const QString &stage, const QString &source)
{
  QString canonicalSource = source;
  canonicalSource.replace("\r\n", "\n");
  canonicalSource.replace('\r', '\n');
  const QByteArray bytes = canonicalSource.toUtf8();
  quint32 hash = 2166136261u;
  for (int i=0; i<bytes.size(); ++i)
    {
      hash ^= static_cast<unsigned char>(bytes.at(i));
      hash *= 16777619u;
    }
  return QString("paint/%1-%2").arg(stage).arg(hash, 8, 16, QLatin1Char('0'));
}

QString numberedSource(const QString &source)
{
  QString numbered;
  const QStringList lines = source.split('\n');
  for (int i=0; i<lines.size(); ++i)
    numbered += QString("%1 : %2\n").arg(i+1).arg(lines.at(i));
  return numbered;
}

QString objectInfoLog(GLhandleARB object)
{
  GLint size = 0;
  glGetObjectParameterivARB(object, GL_OBJECT_INFO_LOG_LENGTH_ARB, &size);
  if (size <= 1)
    return "(driver returned no shader info log)";

  QByteArray buffer(size, '\0');
  GLsizei written = 0;
  glGetInfoLogARB(object, size, &written, buffer.data());
  buffer.resize(qMax(0, static_cast<int>(written)));
  return QString::fromLatin1(buffer.constData(), buffer.size());
}

void showShaderDiagnostic(const QString &title,
                          const QString &label,
                          const QString &log,
                          const QString &source = QString())
{
  QString diagnostic = QString("Shader: %1\n\nDriver log:\n%2").arg(label, log);
  if (!source.isEmpty())
    diagnostic += "\n\nSource:\n" + numberedSource(source);
  qCritical().noquote() << diagnostic;

  QDialog dialog;
  dialog.setWindowTitle(title);
  dialog.resize(900, 600);
  QVBoxLayout *layout = new QVBoxLayout(&dialog);
  QTextEdit *editor = new QTextEdit(&dialog);
  editor->setReadOnly(true);
  editor->setPlainText(diagnostic);
  layout->addWidget(editor);
  dialog.exec();
}

void discardProgram(GLhandleARB &program,
                    GLhandleARB vertexShader,
                    GLhandleARB fragmentShader,
                    bool attached)
{
  if (attached && program)
    {
      if (vertexShader) glDetachObjectARB(program, vertexShader);
      if (fragmentShader) glDetachObjectARB(program, fragmentShader);
    }
  if (vertexShader) glDeleteObjectARB(vertexShader);
  if (fragmentShader) glDeleteObjectARB(fragmentShader);
  if (program) glDeleteObjectARB(program);
  program = 0;
}

bool compileShader(GLhandleARB shader,
                   const QString &label,
                   const QString &source)
{
  const QByteArray sourceBytes = source.toLatin1();
  const char *sourceData = sourceBytes.constData();
  const GLint sourceLength = sourceBytes.size();
  glShaderSourceARB(shader, 1, &sourceData, &sourceLength);
  glCompileShaderARB(shader);

  GLint compiled = GL_FALSE;
  glGetObjectParameterivARB(shader, GL_OBJECT_COMPILE_STATUS_ARB, &compiled);
  if (compiled == GL_TRUE)
    return true;

  showShaderDiagnostic("Shader compilation failed", label,
                       objectInfoLog(shader), source);
  return false;
}

bool loadProgram(GLhandleARB &program,
                 QString vertexSource,
                 QString fragmentSource)
{
  vertexSource = normalizedShaderSource(vertexSource);
  fragmentSource = normalizedShaderSource(fragmentSource);
  const QString vertexLabel = shaderLabel("vertex", vertexSource);
  const QString fragmentLabel = shaderLabel("fragment", fragmentSource);
  const QString programLabel = shaderLabel("program",
                                            vertexSource + "\n--fragment--\n" +
                                            fragmentSource);

  if (!program)
    {
      showShaderDiagnostic("Shader creation failed", programLabel,
                           "Cannot attach shaders to program 0.");
      return false;
    }

  const GLhandleARB vertexShader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
  const GLhandleARB fragmentShader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
  if (!vertexShader || !fragmentShader)
    {
      showShaderDiagnostic("Shader creation failed", programLabel,
                           "glCreateShaderObjectARB returned 0.");
      discardProgram(program, vertexShader, fragmentShader, false);
      return false;
    }

  if (!compileShader(vertexShader, vertexLabel, vertexSource) ||
      !compileShader(fragmentShader, fragmentLabel, fragmentSource))
    {
      discardProgram(program, vertexShader, fragmentShader, false);
      return false;
    }

  glAttachObjectARB(program, vertexShader);
  glAttachObjectARB(program, fragmentShader);
  glLinkProgramARB(program);

  GLint linked = GL_FALSE;
  glGetObjectParameterivARB(program, GL_OBJECT_LINK_STATUS_ARB, &linked);
  if (linked != GL_TRUE)
    {
      const QString sources = "// " + vertexLabel + "\n" + vertexSource +
                              "\n// " + fragmentLabel + "\n" + fragmentSource;
      showShaderDiagnostic("Shader link failed", programLabel,
                           objectInfoLog(program), sources);
      discardProgram(program, vertexShader, fragmentShader, true);
      return false;
    }

  glDetachObjectARB(program, vertexShader);
  glDetachObjectARB(program, fragmentShader);
  glDeleteObjectARB(vertexShader);
  glDeleteObjectARB(fragmentShader);
  return true;
}
}

bool
ShaderFactory::loadShader(GLhandleARB &progObj,
			  QString shaderString)
{
  QString vertexSource;
  vertexSource = "varying vec3 pointpos;\n";
  vertexSource += "void main(void)\n";
  vertexSource += "{\n";
  vertexSource += "  gl_FrontColor = gl_Color;\n";
  vertexSource += "  gl_BackColor = gl_Color;\n";
  vertexSource += "  gl_Position = ftransform();\n";
  vertexSource += "  gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;\n";
  vertexSource += "  gl_TexCoord[1] = gl_TextureMatrix[1] * gl_MultiTexCoord1;\n";
  vertexSource += "  gl_TexCoord[2] = gl_TextureMatrix[2] * gl_MultiTexCoord2;\n";
  vertexSource += "  gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n";
  vertexSource += "  pointpos = gl_Vertex.xyz;\n";
  vertexSource += "}\n";

  return loadProgram(progObj, vertexSource, shaderString);
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
  shader += "uniform sampler2D tagTex;\n";
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
      shader += "  int h0 = int(65536.0*val);\n";
      shader += "  int h1 = h0 / 256;\n";
      shader += "  h0 = int(mod(float(h0),256.0));\n";
      shader += "  float fh0 = float(h0)/256.0;\n";
      shader += "  float fh1 = float(h1)/256.0;\n";
      shader += "  color = texture(lutTex, vec2(fh0,fh1));\n";
    }

  shader += "  if (color.a < 0.001) discard;\n";

  shader += "  float tag = texture(maskTex, gl_TexCoord[0].xyz).x;\n";
  shader += "  vec4 tagcolor = texture(tagTex, vec2(tag, 0));\n";
  shader += "  if (tag < 0.0000001) tagcolor.rgb = color.rgb;\n";
  
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
ShaderFactory::getGrad2(bool nearest) // Sobel operator
{
  QString shader;

  shader += " vec3 gx, gy, gz;\n";
  shader += " float h[9] = float[](1, 2, 1, 2, 4, 2, 1, 2, 1);\n";

  shader += " gx = vec3(1.0/vsize.x,0,0);\n";
  shader += " gy = vec3(0,1.0/vsize.y,0);\n";
  shader += " gz = vec3(0,0,1.0/vsize.z);\n";

  shader += " float a[3] = float[](-1.0/vsize.x,0.0,1.0/vsize.x);\n";
  shader += " float b[3] = float[](-1.0/vsize.y,0.0,1.0/vsize.y);\n";
  shader += " float c[3] = float[](-1.0/vsize.z,0.0,1.0/vsize.z);\n";
  shader += " float sum = 0.0;\n";

  shader += " float vx = 0.0;\n";
  shader += " for(int j=0; j<=2; j++)\n";
  shader += " for(int k=0; k<=2; k++)\n";
  shader += " {\n";
  shader += "   vec3 g0 = vec3(a[0],b[j],c[k]);\n";
  shader += "   vec3 g1 = vec3(a[2],b[j],c[k]);\n";
  shader += "   int i = 3*j+k;\n";
  if (nearest)
    {
      shader += "   vx -= h[i]*texture(dataTex, vC+g0).x;\n";
      shader += "   vx += h[i]*texture(dataTex, vC+g1).x;\n";
    }
  else
    {
      shader += "   vx -= h[i]*texture(dataTex, voxelCoord+g0).x;\n";
      shader += "   vx += h[i]*texture(dataTex, voxelCoord+g1).x;\n";
    }
  shader += " }\n";

  shader += " float vy = 0.0;\n";
  shader += " for(int j=0; j<=2; j++)\n";
  shader += " for(int k=0; k<=2; k++)\n";
  shader += " {\n";
  shader += "   vec3 g0 = vec3(a[j],b[0],c[k]);\n";
  shader += "   vec3 g1 = vec3(a[j],b[2],c[k]);\n";
  shader += "   int i = 3*j+k;\n";
  if (nearest)
    {
      shader += "   vy -= h[i]*texture(dataTex, vC+g0).x;\n";
      shader += "   vy += h[i]*texture(dataTex, vC+g1).x;\n";
    }
  else
    {
      shader += "   vy -= h[i]*texture(dataTex, voxelCoord+g0).x;\n";
      shader += "   vy += h[i]*texture(dataTex, voxelCoord+g1).x;\n";
    }
  shader += " }\n";

  shader += " float vz = 0.0;\n";
  shader += " for(int j=0; j<=2; j++)\n";
  shader += " for(int k=0; k<=2; k++)\n";
  shader += " {\n";
  shader += "   vec3 g0 = vec3(a[j],b[k],c[0]);\n";
  shader += "   vec3 g1 = vec3(a[j],b[k],c[2]);\n";
  shader += "   int i = 3*j+k;\n";
  if (nearest)
    {
      shader += "   vz -= h[i]*texture(dataTex, vC+g0).x;\n";
      shader += "   vz += h[i]*texture(dataTex, vC+g1).x;\n";
    }
  else
    {
      shader += "   vz -= h[i]*texture(dataTex, voxelCoord+g0).x;\n";
      shader += "   vz += h[i]*texture(dataTex, voxelCoord+g1).x;\n";
    }
  shader += " }\n";

  shader += " vec3 grad = vec3(vx, vy, vz);\n";
  shader += " float gradMag = length(grad);\n";
  shader += " gradMag = clamp(gradMag, 0.0, 1.0);\n";
  
  return shader;
}

QString
ShaderFactory::getGrad3(bool nearest)  // Laplacian
{
  QString shader;
  
  shader += " float h[27] = float[](2,3,2, 3,6,3, 2,3,2,   3,6,3, 6,-88,6, 3,6,3,   2,3,2, 3,6,3, 2,3,2);\n";
  shader += " float a[3] = float[](-1.0/vsize.x,0.0,1.0/vsize.x);\n";
  shader += " float b[3] = float[](-1.0/vsize.y,0.0,1.0/vsize.y);\n";
  shader += " float c[3] = float[](-1.0/vsize.z,0.0,1.0/vsize.z);\n";
  shader += " float sum = 0.0;\n";
  shader += " int m = -1;\n";
  shader += " for(int i=0; i<=2; i++)\n";
  shader += " for(int j=0; j<=2; j++)\n";
  shader += " for(int k=0; k<=2; k++)\n";
  shader += " {\n";
  shader += "   vec3 g = vec3(a[i],b[j],c[k]);\n";
  shader += "   m++;\n";
  if (nearest)
    shader += "   sum += h[m]*texture(dataTex, vC+g).x;\n";
  else
    shader += "   sum += h[m]*texture(dataTex, voxelCoord+g).x;\n";
  shader += " }\n";
  shader += " float gradMag = sum/26.0;\n";
  shader += " gradMag += 0.5;\n";
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
				   bool val16,
				   bool mask16,
				   int gradType,
				   QList<CropObject> crops)
{
  bool cropPresent = false;
  for(int i=0; i<crops.count(); i++)
    if (crops[i].cropType() < CropObject::Tear_Tear)
      cropPresent = true;

  QString shader;
  shader  = "#version 420 core\n";
  shader += "#extension GL_ARB_texture_rectangle : enable\n";
  shader += "uniform sampler2D tagTex;\n";
  shader += "uniform sampler2D lutTex;\n";
  shader += "uniform sampler3D dataTex;\n";
  shader += "uniform sampler3D maskTex;\n";
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
  shader += "uniform vec3 voxelSize;\n";


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


  if (cropPresent) shader += CropShaderFactory::generateCropping(crops);

  
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

  
  //------------
  //------------
  if (cropPresent) shader += "float feather = 1.0 - crop(vcorner + vC/voxelSize, false);\n";
  //------------
  //------------
  
  
  shader += "  bvec3 vclt = lessThan(floor(vC+0.5), vC);\n";
  shader += "  vC += vec3(vclt)*vec3(0.5);\n";
  shader += "  vC -= vec3(not(vclt))*vec3(0.5);\n";
  shader += "  vC /= vsize;\n";

  if (nearest)
    shader += "  float val = texture(dataTex, vC).x;\n";
  else
    shader += "  float val = texture(dataTex, voxelCoord).x;\n";

  shader += "  vec4 colorSample = vec4(0.0);\n";
  
  
  if (!val16)
    shader += "  colorSample = texture(lutTex, vec2(val,0.0));\n";
  else
    {
      shader += "{\n";
      shader += "  int h0 = int(65536.0*val);\n";
      shader += "  int h1 = h0 / 256;\n";
      shader += "  h0 = int(mod(float(h0),256.0));\n";
      shader += "  float fh0 = float(h0)/256.0;\n";
      shader += "  float fh1 = float(h1)/256.0;\n";
      shader += "  colorSample = texture(lutTex, vec2(fh0,fh1));\n";
      shader += "}\n";
    }

  //crop
  if (cropPresent)
    shader += "  colorSample.a *= feather;\n";

  
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

  
  shader += "  float tag = texture(maskTex, vC).x;\n";
  //shader += "  vec4 tagcolor = texture(tagTex, vec2(tag, 0));\n";
  shader += "  vec4 tagcolor;\n";
  if (!mask16)
    shader += "  tagcolor = texture(tagTex, vec2(tag, 0));\n";
  else
    {
      shader += "{\n";
      shader += "  int h0 = int(65536.0*tag);\n";
      shader += "  int h1 = h0 / 256;\n";
      shader += "  h0 = int(mod(float(h0),256.0));\n";
      shader += "  float fh0 = float(h0)/256.0;\n";
      shader += "  float fh1 = float(h1)/256.0;\n";
      shader += "  tagcolor = texture(tagTex, vec2(fh0, fh1));\n";
      shader += "}\n";
    }

  shader += "  if (tag < 0.0000001) tagcolor.rgb = colorSample.rgb;\n";
  
  shader += "  colorSample.rgb = mix(colorSample.rgb, tagcolor.rgb, 0.5);\n";
  
  // so that we can use tag opacity to hide certain tagged regions
  // tagcolor.a should either 0 or 1
  shader += "  colorSample *= tagcolor.a;\n";
  

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
ShaderFactory::genEdgeEnhanceShader(bool val16, bool mask16)
{
  QString shader;

  shader = "#version 420 core\n";
  shader += "out vec4 glFragColor;\n";
  shader += "uniform sampler2D tagTex;\n";
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
  if (!mask16)
    shader += "  color = texture(tagTex, vec2(tag, 0));\n";
  else
    {
      shader += "{\n";
      shader += "  int h0 = int(65536.0*tag);\n";
      shader += "  int h1 = h0 / 256;\n";
      shader += "  h0 = int(mod(float(h0),256.0));\n";
      shader += "  float fh0 = float(h0)/256.0;\n";
      shader += "  float fh1 = float(h1)/256.0;\n";
      shader += "  color = texture(tagTex, vec2(fh0, fh1));\n";
      shader += "}\n";
    }
  // so that we can use tag opacity to hide certain tagged regions
  // tagcolor.a should either 0 or 1
  shader += "  if (color.a < 0.1) discard;\n";
  //---------------------

  shader += "  if (tag < 0.0000001)\n";
  shader += "   {\n";
  shader += "    val = texture2DRect(pvtTex, spos0).y;\n";
  if (!val16)
    shader += "   color = texture(lutTex, vec2(val,0.0));\n";
  else
    {
      shader += "    int h0 = int(65536.0*val);\n";
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
  return loadProgram(progObj, vertShaderString, fragShaderString);
}
