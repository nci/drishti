#ifndef GEOMETRYOBJECTS_H
#define GEOMETRYOBJECTS_H

#include <GL/glew.h>

#include "clipplane.h"
#include "trisets.h"
#include "captions.h"
#include "scalebar.h"
#include "hitpoints.h"
#include "paths.h"

class GeometryObjects
{
 public :
  enum GeometryObjectTypes
    {
      NOGEO = 0,
      HITPOINT,
      PATH,
      TRISET,
      CAPTION,
      SCALEBAR
    };

  static void init();
  static void clear();
  static bool grabsMouse();

  static Captions* captions();
  static ScaleBars* scalebars();
  static HitPoints* hitpoints();
  static Paths* paths();
  static Trisets* trisets();
  static ClipPlanes* clipplanes();

  static void removeFromMouseGrabberPool();
  static void addInMouseGrabberPool();

  static void show();
  static void hide();


  static bool keyPressEvent(QKeyEvent*);

  static bool inPool;
  static bool showGeometry;

  static MouseGrabber* checkIfGrabsMouse(int, int, Camera*);
  static int mouseGrabberType();

 private :
  static bool m_initialized;
  static int m_mgType;
  static Captions* m_captions;
  static ScaleBars* m_scalebars;
  static HitPoints* m_hitpoints;
  static Paths* m_paths;
  static Trisets* m_trisets;
  static ClipPlanes* m_clipplanes;
};

#endif
