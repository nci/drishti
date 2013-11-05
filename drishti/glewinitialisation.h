
#ifndef GLEWINITIALISATION_H
#define GLEWINITIALISATION_H

#include <GL/glew.h>
#include <QtGui>

class GlewInit
{
public:
  static bool initialised();
  static bool initialise();
  static bool fbo_ok;
private:
  static bool glew_initialised;
};

#endif // GLEW_INITIALISATION_H
