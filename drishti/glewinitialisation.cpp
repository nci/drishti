#include <stdio.h>
#include "glewinitialisation.h"
#include <QMessageBox>

bool GlewInit::glew_initialised = false;
bool GlewInit::fbo_ok = false;

static int glew_argc = 1;
static char *glew_argv[] = { "Drishti", 0 };

bool GlewInit::initialised() { return glew_initialised; }

bool
GlewInit::initialise()
{
  if (!glew_initialised)
    {
      if (glewInit() != GLEW_OK)
	{
	  QMessageBox::information(0, "Glew", \
				   "Failed to initialise glew");
	  return false;
	}
    
      if (glewGetExtension("GL_ARB_fragment_shader")      != GL_TRUE ||
	  glewGetExtension("GL_ARB_vertex_shader")        != GL_TRUE ||
	  glewGetExtension("GL_ARB_shader_objects")       != GL_TRUE ||
	  glewGetExtension("GL_ARB_shading_language_100") != GL_TRUE)    {
	QMessageBox::information(0, "Glew", \
				 "Driver does not support OpenGL Shading Language.");
	return false;
      }

      glew_initialised = true;

      if (glewGetExtension("GL_EXT_framebuffer_object") != GL_TRUE)
	{	  
	  fbo_ok = false;
	  QMessageBox::information(0, "Glew", \
				   "Driver does not support Framebuffer Objects (GL_EXT_framebuffer_object)");
	}
      else
	fbo_ok = true;
    }

  return true;
}


