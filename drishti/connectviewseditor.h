#ifndef CONNECTVIEWSEDITOR_H
#define CONNECTVIEWSEDITOR_H

  connect(this, SIGNAL(addView(float,
			       float,
			       bool, bool,
			       Vec,
			       Vec, Quaternion,
			       float,
			       QImage,
			       int,
			       LightingInformation,
			       QList<BrickInformation>,
			       Vec, Vec,
			       QList<SplineInformation>,
			       int, int, QString, QString, QString)),
	  m_gallery, SLOT(addView(float,
				  float,
				  bool, bool,
				  Vec,
				  Vec, Quaternion,
				  float,
				  QImage,
				  int,
				  LightingInformation,
				  QList<BrickInformation>,
				  Vec, Vec,
				  QList<SplineInformation>,
				  int, int, QString, QString, QString)));

  connect(m_Viewer, SIGNAL(setHiresMode(bool)),
	  m_gallery, SLOT(setHiresMode(bool)));

  connect(m_gallery, SIGNAL(updateVolInfo(int)),
	  this, SLOT(updateVolInfo(int)));

connect(m_gallery, SIGNAL(updateVolInfo(int, int)),
	this, SLOT(updateVolInfo(int, int)));

  connect(m_gallery, SIGNAL(updateParameters(float, float,
					     int, bool, bool, Vec,
					     QString,
					     int, int, 
					     QString, QString, QString)),
	  this, SLOT(updateParameters(float, float,
				      int, bool, bool, Vec,
				      QString,
				      int, int,
				      QString, QString, QString)));

  connect(m_gallery, SIGNAL(updateTransferFunctionManager(QList<SplineInformation>)),
	  this, SLOT(updateTransferFunctionManager(QList<SplineInformation>)));

  connect(m_gallery, SIGNAL(currentView()),
	  m_Viewer, SLOT(currentView()));

  connect(m_gallery, SIGNAL(updateGL()),
	  m_Viewer, SLOT(updateGL()));

  connect(m_gallery, SIGNAL(updateLookFrom(Vec, Quaternion, float)),
	  m_Viewer, SLOT(updateLookFrom(Vec, Quaternion, float)));

  connect(m_gallery, SIGNAL(updateBrickInfo(QList<BrickInformation>)),
	  m_bricks, SLOT(setBricks(QList<BrickInformation>)));

  connect(m_gallery, SIGNAL(updateLightInfo(LightingInformation)),
	  m_lightingWidget,
	  SLOT(setLightInfo(LightingInformation)));

  connect(m_gallery, SIGNAL(updateTagColors()),
	  m_preferencesWidget, SLOT(updateTagColors()));


#endif
