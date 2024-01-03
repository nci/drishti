#include "vrmenu.h"
#include <QMessageBox>

VrMenu::VrMenu() : QObject()
{
  m_menus["01"] = new Menu01();

  m_currMenu = "none";

  connect(m_menus["01"], SIGNAL(resetModel()),
	  this, SIGNAL(resetModel()));

  connect(m_menus["01"], SIGNAL(updateScale(int)),
	  this, SIGNAL(updateScale(int)));

  connect(m_menus["01"], SIGNAL(updateSoftShadows(bool)),
	  this, SIGNAL(updateSoftShadows(bool)));

  connect(m_menus["01"], SIGNAL(updateEdges(bool)),
	  this, SIGNAL(updateEdges(bool)));

  connect(m_menus["01"], SIGNAL(toggle(QString, float)),
	  this, SIGNAL(toggle(QString, float)));
}

VrMenu::~VrMenu()
{
}

void
VrMenu::draw(QMatrix4x4 mvp, QMatrix4x4 matL, bool triggerPressed)
{
  if (m_currMenu == "01")
    ((Menu01*)(m_menus[m_currMenu]))->draw(mvp, matL, triggerPressed);
}

int
VrMenu::checkOptions(QMatrix4x4 matL, QMatrix4x4 matR, int triggered)
{
  if (m_currMenu == "01")
    return ((Menu01*)(m_menus[m_currMenu]))->checkOptions(matL, matR, triggered);
  
  return -1;
}

void
VrMenu::generateHUD(QMatrix4x4 mat)
{
  ((Menu01*)(m_menus["01"]))->generateHUD(mat);
}
void
VrMenu::reGenerateHUD()
{
  ((Menu01*)(m_menus["01"]))->reGenerateHUD();
}

void
VrMenu::setCurrentMenu(QString m)
{
  m_currMenu = m;

  ((Menu01*)(m_menus["01"]))->setVisible(false);

  if (m_currMenu == "01")
    ((Menu01*)(m_menus["01"]))->setVisible(true);
}

bool
VrMenu::pointingToMenu()
{
  bool pm = false;
  
  pm = ((Menu01*)(m_menus["01"]))->pointingToMenu();

  return pm;
}

QVector3D
VrMenu::pinPoint()
{
  if (((Menu01*)(m_menus["01"]))->pointingToMenu())
    return ((Menu01*)(m_menus["01"]))->pinPoint();

  return QVector3D(-1000,-1000,-1000);
}

void
VrMenu::setValue(QString varItem, float value)
{
  ((Menu01*)(m_menus["01"]))->setValue(varItem, value);
}

float
VrMenu::value(QString name)
{
  return ((Menu01*)(m_menus["01"]))->value(name);
}
