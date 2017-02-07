#include "viewer3d.h"

Viewer3D::Viewer3D(QWidget *parent) :
  QFrame(parent)
{
  setFrameShape(QFrame::Box);

  m_viewer = new Viewer(this);

  m_raycast = new QCheckBox("Raycast");
  m_raycast->setChecked(true);

  m_changeLayout = new QPushButton(QIcon(":/images/enlarge.png"),"");

  QHBoxLayout *hl = new QHBoxLayout();
  hl->addWidget(m_raycast);
  hl->addStretch();
  hl->addWidget(m_changeLayout);

  QVBoxLayout *vl = new QVBoxLayout();
  vl->addLayout(hl);
  vl->addWidget(m_viewer);

  setLayout(vl);

  connect(m_raycast, SIGNAL(clicked(bool)),
	  m_viewer, SLOT(setRenderMode(bool)));

  connect(m_changeLayout, SIGNAL(clicked()),
	  this, SIGNAL(changeLayout()));

  m_maximized = false;
}

void
Viewer3D::setLarge(bool ms)
{
  m_maximized = ms;

  if (m_maximized)
    m_changeLayout->setIcon(QIcon(":/images/shrink.png"));
  else
    m_changeLayout->setIcon(QIcon(":/images/enlarge.png"));
}
