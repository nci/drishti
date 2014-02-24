#include "geometryobjects.h"

int GeometryObjects::m_mgType = NOGEO;
bool GeometryObjects::m_initialized = false;

void GeometryObjects::init()
{
  if(GeometryObjects::m_initialized)
    return;
  
  m_captions = new Captions();
  m_imageCaptions = new ImageCaptions();
  m_colorbars = new ColorBars();
  m_scalebars = new ScaleBars();
  m_hitpoints = new HitPoints();
  m_paths = new Paths();
  m_crops = new Crops();
  m_pathgroups = new PathGroups();
  m_trisets = new Trisets();
  m_networks = new Networks();
  m_clipplanes = new ClipPlanes();
  m_grids = new Grids();
  m_initialized = true;
  m_mgType = NOGEO;
}

void GeometryObjects::clear()
{
  m_mgType = NOGEO;
  m_captions->clear();
  m_imageCaptions->clear();
  m_colorbars->clear();
  m_scalebars->clear();
  m_hitpoints->clear();
  m_paths->clear();
  m_crops->clear();
  m_pathgroups->clear();
  m_trisets->clear();
  m_networks->clear();
  m_clipplanes->clear();
  m_grids->clear();
}

bool GeometryObjects::grabsMouse()
{
  return (GeometryObjects::captions()->grabsMouse() ||
	  GeometryObjects::imageCaptions()->grabsMouse() ||
	  GeometryObjects::colorbars()->grabsMouse() ||
	  GeometryObjects::scalebars()->grabsMouse() ||
	  GeometryObjects::hitpoints()->grabsMouse() ||
	  GeometryObjects::clipplanes()->grabsMouse() ||
	  GeometryObjects::crops()->grabsMouse() ||
	  GeometryObjects::paths()->grabsMouse() ||
	  GeometryObjects::grids()->grabsMouse() ||
	  GeometryObjects::pathgroups()->grabsMouse() ||
	  GeometryObjects::trisets()->grabsMouse() ||
	  GeometryObjects::networks()->grabsMouse());
}

bool
GeometryObjects::keyPressEvent(QKeyEvent *event)
{
  if (captions()->grabsMouse())
    return captions()->keyPressEvent(event);

  if (imageCaptions()->grabsMouse())
    return imageCaptions()->keyPressEvent(event);

  if (colorbars()->grabsMouse())
    return colorbars()->keyPressEvent(event);

  if (GeometryObjects::scalebars()->grabsMouse())
    return scalebars()->keyPressEvent(event);

  if (GeometryObjects::hitpoints()->grabsMouse())
    return hitpoints()->keyPressEvent(event);

  if (GeometryObjects::clipplanes()->grabsMouse())
    return clipplanes()->keyPressEvent(event);

  if (GeometryObjects::crops()->grabsMouse())
    return crops()->keyPressEvent(event);

  if (GeometryObjects::paths()->grabsMouse())
    return paths()->keyPressEvent(event);

  if (GeometryObjects::grids()->grabsMouse())
    return grids()->keyPressEvent(event);

  if (GeometryObjects::pathgroups()->grabsMouse())
    return pathgroups()->keyPressEvent(event);

  if (GeometryObjects::trisets()->grabsMouse())
    return trisets()->keyPressEvent(event);

  if (GeometryObjects::networks()->grabsMouse())
    return networks()->keyPressEvent(event);

  return false;
}

Captions* GeometryObjects::m_captions = NULL;
Captions* GeometryObjects::captions() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_captions; 
}

ImageCaptions* GeometryObjects::m_imageCaptions = NULL;
ImageCaptions* GeometryObjects::imageCaptions() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_imageCaptions; 
}

ColorBars* GeometryObjects::m_colorbars = NULL;
ColorBars* GeometryObjects::colorbars() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_colorbars; 
}

ScaleBars* GeometryObjects::m_scalebars = NULL;
ScaleBars* GeometryObjects::scalebars() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_scalebars; 
}

HitPoints* GeometryObjects::m_hitpoints = NULL;
HitPoints* GeometryObjects::hitpoints() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_hitpoints; 
}

Paths* GeometryObjects::m_paths = NULL;
Paths* GeometryObjects::paths() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_paths; 
}

Crops* GeometryObjects::m_crops = NULL;
Crops* GeometryObjects::crops() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_crops; 
}

PathGroups* GeometryObjects::m_pathgroups = NULL;
PathGroups* GeometryObjects::pathgroups() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_pathgroups; 
}

Trisets* GeometryObjects::m_trisets = NULL;
Trisets* GeometryObjects::trisets() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_trisets; 
}

Networks* GeometryObjects::m_networks = NULL;
Networks* GeometryObjects::networks() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_networks; 
}

ClipPlanes* GeometryObjects::m_clipplanes = NULL;
ClipPlanes* GeometryObjects::clipplanes() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_clipplanes; 
}

Grids* GeometryObjects::m_grids = NULL;
Grids* GeometryObjects::grids() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_grids; 
}

bool GeometryObjects::inPool = false;
bool GeometryObjects::showGeometry = true;

void
GeometryObjects::removeFromMouseGrabberPool()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  m_trisets->removeFromMouseGrabberPool();
  m_networks->removeFromMouseGrabberPool();
  m_paths->removeFromMouseGrabberPool();
  m_pathgroups->removeFromMouseGrabberPool();
  m_crops->removeFromMouseGrabberPool();
  m_clipplanes->removeFromMouseGrabberPool();
  m_grids->removeFromMouseGrabberPool();
}

void
GeometryObjects::addInMouseGrabberPool()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  m_trisets->addInMouseGrabberPool();
  m_networks->addInMouseGrabberPool();
  m_paths->addInMouseGrabberPool();
  m_pathgroups->addInMouseGrabberPool();
  m_crops->addInMouseGrabberPool();
  m_clipplanes->addInMouseGrabberPool();
  m_grids->addInMouseGrabberPool();
}

void
GeometryObjects::show()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  m_networks->show();
  m_clipplanes->show();
  m_crops->show();
}

void
GeometryObjects::hide()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  m_networks->hide();
  m_clipplanes->hide();
  m_crops->hide();
}

MouseGrabber*
GeometryObjects::checkIfGrabsMouse(int x, int y, Camera *cam)
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  m_mgType = NOGEO;

  MouseGrabber *mg = NULL;

  mg = m_hitpoints->checkIfGrabsMouse(x,y,cam);
  if (mg != NULL) m_mgType = HITPOINT;

  if (mg == NULL) mg = m_paths->checkIfGrabsMouse(x,y,cam);
  if (mg != NULL) m_mgType = PATH;
  
  return mg;
}

int GeometryObjects::mouseGrabberType() { return m_mgType; }
