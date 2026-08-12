#include <QApplication>
#include <QGLFormat>
#include <QGLWidget>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFramebufferObject>
#include <QOpenGLShaderProgram>

#include <iostream>

namespace
{
int fail(const QString& message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

QString glString(QOpenGLExtraFunctions *functions, GLenum name)
{
  const GLubyte *value = functions->glGetString(name);
  return value ?
    QString::fromLatin1(reinterpret_cast<const char*>(value)) :
    QStringLiteral("<unavailable>");
}

bool versionAtLeast(const QGLFormat& format, int major, int minor)
{
  return format.majorVersion() > major ||
         (format.majorVersion() == major && format.minorVersion() >= minor);
}
}

int main(int argc, char **argv)
{
  QApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
  QApplication application(argc, argv);

  QGLFormat requested;
  requested.setVersion(4, 5);
  requested.setProfile(QGLFormat::CompatibilityProfile);
  requested.setDepthBufferSize(24);
  requested.setStencilBufferSize(8);
  QGLFormat::setDefaultFormat(requested);

  QGLWidget widget(requested);
  widget.makeCurrent();
  QOpenGLContext *context = QOpenGLContext::currentContext();
  if (!widget.isValid() || !context || !context->isValid())
    return fail("Could not create the QGLWidget Desktop OpenGL context used by Drishti.");

  QOpenGLExtraFunctions *functions = context->extraFunctions();
  functions->initializeOpenGLFunctions();

  const QString vendor = glString(functions, GL_VENDOR);
  const QString renderer = glString(functions, GL_RENDERER);
  const QString version = glString(functions, GL_VERSION);
  const QString shadingLanguage =
    glString(functions, GL_SHADING_LANGUAGE_VERSION);
  const QGLFormat actual = widget.format();

  std::cout << "vendor=" << vendor.toStdString() << '\n'
            << "renderer=" << renderer.toStdString() << '\n'
            << "version=" << version.toStdString() << '\n'
            << "glsl=" << shadingLanguage.toStdString() << '\n'
            << "context=" << actual.majorVersion() << '.'
            << actual.minorVersion() << " profile="
            << static_cast<int>(actual.profile()) << std::endl;

  GLint contextFlags = 0;
  GLint profileMask = 0;
  functions->glGetIntegerv(GL_CONTEXT_FLAGS, &contextFlags);
  functions->glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMask);
  std::cout << "context_flags=0x" << std::hex << contextFlags
            << " profile_mask=0x" << profileMask << std::dec << std::endl;

  if (!versionAtLeast(actual, 4, 5) ||
      actual.profile() != QGLFormat::CompatibilityProfile ||
      (profileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) == 0 ||
      (contextFlags & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT) != 0)
    return fail("The created context does not satisfy OpenGL 4.5 Compatibility.");

  if (argc == 2)
    {
      const QString expected = QString::fromLocal8Bit(argv[1]);
      if (!vendor.contains(expected, Qt::CaseInsensitive) &&
          !renderer.contains(expected, Qt::CaseInsensitive))
        return fail(QString("Expected GPU '%1', but got %2 / %3.")
                      .arg(expected, vendor, renderer));
    }
  else if (argc > 2)
    return fail("Usage: desktop_opengl_context_smoke [expected-vendor]");

  GLint maximumTextureSize = 0;
  GLint maximum3DTextureSize = 0;
  GLint maximumArrayLayers = 0;
  functions->glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maximumTextureSize);
  functions->glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maximum3DTextureSize);
  functions->glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maximumArrayLayers);
  std::cout << "max_texture=" << maximumTextureSize
            << " max_3d_texture=" << maximum3DTextureSize
            << " max_array_layers=" << maximumArrayLayers << std::endl;
  if (maximumTextureSize < 1024 || maximum3DTextureSize < 64 ||
      maximumArrayLayers < 64)
    return fail("The OpenGL texture limits are below the portable renderer minimum.");

  QOpenGLShaderProgram shader;
  if (!shader.addShaderFromSourceCode(
        QOpenGLShader::Vertex,
        "#version 450 compatibility\n"
        "void main() { gl_Position = ftransform(); }\n") ||
      !shader.addShaderFromSourceCode(
        QOpenGLShader::Fragment,
        "#version 450 compatibility\n"
        "void main() { gl_FragColor = vec4(0.25, 0.5, 0.75, 1.0); }\n") ||
      !shader.link())
    return fail(QString("Compatibility shader compilation failed: %1")
                  .arg(shader.log()));

  QOpenGLFramebufferObjectFormat framebufferFormat;
  framebufferFormat.setAttachment(
    QOpenGLFramebufferObject::CombinedDepthStencil);
  framebufferFormat.setInternalTextureFormat(GL_RGBA16F);
  QOpenGLFramebufferObject framebuffer(QSize(1024, 1024), framebufferFormat);
  if (!framebuffer.isValid() || !framebuffer.bind())
    return fail("Could not create and bind an RGBA16F framebuffer.");
  framebuffer.release();

  GLuint textures[2] = { 0, 0 };
  functions->glGenTextures(2, textures);
  functions->glBindTexture(GL_TEXTURE_3D, textures[0]);
  functions->glTexImage3D(GL_TEXTURE_3D, 0, GL_R16,
                          64, 64, 64, 0,
                          GL_RED, GL_UNSIGNED_SHORT, nullptr);
  GLenum error = functions->glGetError();
  if (error == GL_NO_ERROR)
    {
      functions->glBindTexture(GL_TEXTURE_2D_ARRAY, textures[1]);
      functions->glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_R8,
                              256, 256, 64, 0,
                              GL_RED, GL_UNSIGNED_BYTE, nullptr);
      error = functions->glGetError();
    }
  functions->glBindTexture(GL_TEXTURE_3D, 0);
  functions->glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
  functions->glDeleteTextures(2, textures);
  if (error != GL_NO_ERROR)
    return fail(QString("Representative texture allocation failed with OpenGL error 0x%1.")
                  .arg(QString::number(static_cast<qulonglong>(error), 16)));

  widget.doneCurrent();
  std::cout << "Desktop OpenGL context smoke passed" << std::endl;
  return 0;
}
