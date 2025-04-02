#include "geometryobjects.h"

int GeometryObjects::m_mgType = NOGEO;
bool GeometryObjects::m_initialized = false;

void GeometryObjects::init()
{
  if(GeometryObjects::m_initialized)
    return;
  
  m_clipplanes = new ClipPlanes();
  m_crops = new Crops();
//  m_paths = new Paths();

  m_initialized = true;
  m_mgType = NOGEO;
}

void GeometryObjects::clear()
{
  m_mgType = NOGEO;

  m_clipplanes->clear();
  m_crops->clear();
//  m_paths->clear();
}

bool GeometryObjects::grabsMouse()
{
  return (GeometryObjects::clipplanes()->grabsMouse() ||
	  GeometryObjects::crops()->grabsMouse());
//	  GeometryObjects::paths()->grabsMouse());
}

bool
GeometryObjects::keyPressEvent(QKeyEvent *event)
{
  if (GeometryObjects::clipplanes()->grabsMouse())
    return clipplanes()->keyPressEvent(event);

  if (GeometryObjects::crops()->grabsMouse())
    return crops()->keyPressEvent(event);

//  if (GeometryObjects::paths()->grabsMouse())
//    return paths()->keyPressEvent(event);

  return false;
}

Crops* GeometryObjects::m_crops = NULL;
Crops* GeometryObjects::crops() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_crops; 
}

ClipPlanes* GeometryObjects::m_clipplanes = NULL;
ClipPlanes* GeometryObjects::clipplanes() 
{ 
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  return m_clipplanes; 
}

//Paths* GeometryObjects::m_paths = NULL;
//Paths* GeometryObjects::paths() 
//{ 
//  if(!GeometryObjects::m_initialized) GeometryObjects::init();
//  return m_paths; 
//}

bool GeometryObjects::inPool = false;
bool GeometryObjects::showGeometry = true;

void
GeometryObjects::removeFromMouseGrabberPool()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();

  m_clipplanes->removeFromMouseGrabberPool();
  m_crops->removeFromMouseGrabberPool();
//  m_paths->removeFromMouseGrabberPool();
}

void
GeometryObjects::addInMouseGrabberPool()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();

  m_clipplanes->addInMouseGrabberPool();
  m_crops->addInMouseGrabberPool();
//  m_paths->addInMouseGrabberPool();
}

void
GeometryObjects::show()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  m_clipplanes->show();
  m_crops->show();
}

void
GeometryObjects::hide()
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  m_clipplanes->hide();
  m_crops->hide();
}

MouseGrabber*
GeometryObjects::checkIfGrabsMouse(int x, int y, Camera *cam)
{
  if(!GeometryObjects::m_initialized) GeometryObjects::init();
  m_mgType = NOGEO;

  MouseGrabber *mg = NULL;

//  if (mg == NULL) mg = m_paths->checkIfGrabsMouse(x,y,cam);
//  if (mg != NULL) m_mgType = PATH;
  
  return mg;
}

int GeometryObjects::mouseGrabberType() { return m_mgType; }
