#ifndef GEOMETRYOBJECTS_H
#define GEOMETRYOBJECTS_H

#include "clipplane.h"
#include "networks.h"
#include "trisets.h"
#include "captions.h"
#include "colorbar.h"
#include "scalebar.h"
#include "hitpoints.h"
#include "paths.h"
#include "crops.h"
#include "pathgroups.h"
#include "grids.h"

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
      CROP
    };

  static void init();
  static void clear();
  static bool grabsMouse();
  static Captions* captions();
  static ColorBars* colorbars();
  static ScaleBars* scalebars();
  static HitPoints* hitpoints();
  static Paths* paths();
  static Crops* crops();
  static PathGroups* pathgroups();
  static Trisets* trisets();
  static Networks* networks();
  static ClipPlanes* clipplanes();
  static Grids* grids();

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
  static ColorBars* m_colorbars;
  static ScaleBars* m_scalebars;
  static HitPoints* m_hitpoints;
  static Paths* m_paths;
  static Crops* m_crops;
  static PathGroups* m_pathgroups;
  static Trisets* m_trisets;
  static Networks* m_networks;
  static ClipPlanes* m_clipplanes;
  static Grids* m_grids;
};

#endif
