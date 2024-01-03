#include "geometryobjects.h"

int GeometryObjects::m_mgType = NOGEO;
bool GeometryObjects::m_initialized = false;

void GeometryObjects::init()
{
  if(GeometryObjects::m_initialized)
    return;
  
  m_captions = new Captions();
  m_scalebars = new ScaleBars();
  m_hitpoints = new HitPoints();
  m_paths = new Paths();
  m_trisets = new Trisets();
  m_clipplanes = new ClipPlanes();
  m_initialized = true;
  m_mgType = NOGEO;
}

void GeometryObjects::clear()
{
  m_mgType = NOGEO;
  m_captions->clear();
  m_scalebars->clear();
  m_hitpoints->clear();
  m_paths->clear();
  m_trisets->clear();
  m_clipplanes->clear();
}

bool GeometryObjects::grabsMouse()
{
  return (GeometryObjects::captions()->grabsMouse() ||
	  GeometryObjects::scalebars()->grabsMouse() ||
	  GeometryObjects::hitpoints()->grabsMouse() ||
	  GeometryObjects::clipplanes()->grabsMouse() ||
	  GeometryObjects::paths()->grabsMouse() ||
	  GeometryObjects::trisets()->grabsMouse());
}

bool
GeometryObjects::keyPressEvent(QKeyEvent *event)
{
  if (captions()->grabsMouse())
    return captions()->keyPressEvent(event);

  if (GeometryObjects::scalebars()->grabsMouse())
    return scalebars()->keyPressEvent(event);

  if (GeometryObjects::hitpoints()->grabsMouse())
    return hitpoints()->keyPressEvent(event);

  if (GeometryObjects::clipplanes()->grabsMouse())
    return clipplanes()->keyPressEvent(event);

  if (GeometryObjects::paths()->grabsMouse())
    return paths()->keyPressEvent(event);

  if (GeometryObjects::trisets()->grabsMouse())
    return trisets()->keyPressEvent(event);

  return false;
}

Captions* GeometryObjects::m_captions = NULL;
Captions* GeometryObjects::captions() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_captions; 
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


Trisets* GeometryObjects::m_trisets = NULL;
Trisets* GeometryObjects::trisets() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_trisets; 
}

ClipPlanes* GeometryObjects::m_clipplanes = NULL;
ClipPlanes* GeometryObjects::clipplanes() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_clipplanes; 
}

bool GeometryObjects::inPool = false;
bool GeometryObjects::showGeometry = true;

void
GeometryObjects::removeFromMouseGrabberPool()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();

  m_trisets->setGrab(false);
  
  //m_trisets->removeFromMouseGrabberPool();
  //m_paths->removeFromMouseGrabberPool();
  m_clipplanes->removeFromMouseGrabberPool();
}

void
GeometryObjects::addInMouseGrabberPool()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();

  m_trisets->setGrab(true);
  
  //m_trisets->addInMouseGrabberPool();
  //m_paths->addInMouseGrabberPool();
  m_clipplanes->addInMouseGrabberPool();
}

void
GeometryObjects::show()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  m_clipplanes->show();
}

void
GeometryObjects::hide()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  m_clipplanes->hide();
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
