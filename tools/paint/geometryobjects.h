#ifndef GEOMETRYOBJECTS_H
#define GEOMETRYOBJECTS_H

#include <GL/glew.h>

#include "clipplane.h"
#include "crops.h"
//#include "paths.h"

class GeometryObjects
{
 public :
  enum GeometryObjectTypes
    {
      NOGEO = 0,
      HITPOINT,
      PATH,
      PATHGROUP,
      TRISET,
      NETWORK,
      GRID,
      CAPTION,
      COLORBAR,
      SCALEBAR,
      CROP,
      LANDMARKS
    };

  static void init();
  static void clear();
  static bool grabsMouse();
  
  static ClipPlanes* clipplanes();
  static Crops* crops();
  //static Paths* paths();

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
  
  static ClipPlanes* m_clipplanes;
  static Crops* m_crops;
  //static Paths* m_paths;
};

#endif
