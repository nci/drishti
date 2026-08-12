#include <stdio.h>
#include "glewinitialisation.h"
#include <QDebug>
#include <QGLFramebufferObject>
#include <QMessageBox>
#include <QString>

namespace
{
QString glString(GLenum name)
{
  const GLubyte *value = glGetString(name);
  if (!value)
    return QStringLiteral("unknown");

  return QString::fromLatin1(reinterpret_cast<const char*>(value));
}

QString contextDescription()
{
  return QStringLiteral("Vendor: %1\nRenderer: %2\nOpenGL: %3\nGLSL: %4")
    .arg(glString(GL_VENDOR),
         glString(GL_RENDERER),
         glString(GL_VERSION),
         glString(GL_SHADING_LANGUAGE_VERSION));
}

bool contextVersionAtLeast(GLint requiredMajor, GLint requiredMinor)
{
  GLint major = 0;
  GLint minor = 0;
  glGetIntegerv(GL_MAJOR_VERSION, &major);
  glGetIntegerv(GL_MINOR_VERSION, &minor);
  return major > requiredMajor ||
         (major == requiredMajor && minor >= requiredMinor);
}
}

bool GlewInit::glew_initialised = false;
bool GlewInit::fbo_ok = false;

bool GlewInit::initialised() { return glew_initialised; }

bool
GlewInit::initialise()
{
  if (!glew_initialised)
    {

      glewExperimental = GL_TRUE;
      GLenum glewStatus = glewInit();
      QString contextInfo = contextDescription();
      qInfo().noquote() << QStringLiteral("OpenGL context\n%1").arg(contextInfo);

      if (glewStatus != GLEW_OK)
	{
	  QString glewError = QString::fromLatin1(
	    reinterpret_cast<const char*>(glewGetErrorString(glewStatus)));
	  QMessageBox::critical(0, "OpenGL",
				QStringLiteral("Failed to initialise GLEW: %1\n\n%2")
				.arg(glewError, contextInfo));
	  return false;
	}

      // GLEW may leave GL_INVALID_ENUM behind while probing a modern context.
      glGetError();

#if defined(Q_OS_WIN32) || defined(Q_OS_LINUX)
      // Intel compatibility-profile drivers can expose a valid 4.2+ context
      // even when GLEW's aggregate flag is false because an unused entry
      // point is unavailable.
      if (!GLEW_VERSION_4_2 && !contextVersionAtLeast(4, 2))
	{
	  QMessageBox::critical(0, "OpenGL",
				QStringLiteral("Drishti Mesh requires OpenGL 4.2 or newer.\n\n%1")
				.arg(contextInfo));
	  return false;
	}

      GLint profileMask = 0;
      glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMask);
      if ((profileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) == 0)
	{
	  QMessageBox::critical(0, "OpenGL",
				QStringLiteral("Drishti Mesh requires an OpenGL compatibility profile.\n\n%1")
				.arg(contextInfo));
	  return false;
	}
#else
      if (glewGetExtension("GL_ARB_fragment_shader")      != GL_TRUE ||
	  glewGetExtension("GL_ARB_vertex_shader")        != GL_TRUE ||
	  glewGetExtension("GL_ARB_shader_objects")       != GL_TRUE ||
	  glewGetExtension("GL_ARB_shading_language_100") != GL_TRUE)
	{
	  QMessageBox::critical(0, "OpenGL",
				QStringLiteral("Driver does not support OpenGL Shading Language.\n\n%1")
				.arg(contextInfo));
	  return false;
	}
#endif

      fbo_ok = GLEW_VERSION_3_0 ||
	       GLEW_ARB_framebuffer_object ||
	       GLEW_EXT_framebuffer_object;
      fbo_ok = fbo_ok && QGLFramebufferObject::hasOpenGLFramebufferObjects();
      if (!fbo_ok)
	{
	  QMessageBox::critical(0, "OpenGL",
				QStringLiteral("Drishti Mesh requires framebuffer objects.\n\n%1")
				.arg(contextInfo));
	  return false;
	}

      glew_initialised = true;
    }

  return true;
}


