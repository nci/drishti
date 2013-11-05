#ifndef CONNECTVOLINFOWIDGET_H
#define CONNECTVOLINFOWIDGET_H

  connect(m_volInfoWidget, SIGNAL(volumeNumber(int)),
	  this, SLOT(setVolumeNumber(int)));

  connect(m_volInfoWidget, SIGNAL(volumeNumber(int, int)),
	  this, SLOT(setVolumeNumber(int, int)));

  connect(m_volInfoWidget, SIGNAL(repeatType(int, bool)),
	  this, SLOT(setRepeatType(int, bool)));

  connect(m_volInfoWidget, SIGNAL(updateScaling()),
	  this, SLOT(updateScaling()));

  connect(m_volInfoWidget, SIGNAL(updateScaling()),
	  m_Viewer, SLOT(showFullScene()));

  connect(m_volInfoWidget, SIGNAL(updateGL()),
	  m_Viewer, SLOT(update()));

  connect(this, SIGNAL(refreshVolInfo(int, VolumeInformation)),
	  m_volInfoWidget, SLOT(refreshVolInfo(int, VolumeInformation)));

  connect(this, SIGNAL(refreshVolInfo(int, int, VolumeInformation)),
	  m_volInfoWidget, SLOT(refreshVolInfo(int, int, VolumeInformation)));

  connect(this, SIGNAL(setVolumes(QList<int>)),
	  m_volInfoWidget, SLOT(setVolumes(QList<int>)));

#endif
