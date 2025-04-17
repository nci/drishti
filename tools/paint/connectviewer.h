#ifndef CONNECTVIEWER_H
#define CONNECTVIEWER_H

  
  connect(m_viewer, SIGNAL(checkFileSave()),
	  this, SLOT(checkFileSave()));
	  
  connect(m_viewer, SIGNAL(undoPaint3D()),
	  this, SLOT(undoPaint3D()));
  connect(m_viewer, SIGNAL(paint3DStart()),
	  this, SLOT(paint3DStart()));
  connect(m_viewer, SIGNAL(paint3D(Vec,Vec,int,int,int,int,int, bool)),
	  this, SLOT(paint3D(Vec,Vec,int,int,int,int,int, bool)));
  connect(m_viewer, SIGNAL(paint3DEnd()),
	  this, SLOT(paint3DEnd()));

  connect(m_viewer, SIGNAL(changeImageSlice(int, int, int)),
	  this, SLOT(changeImageSlice(int, int, int)));

  connect(m_viewer, SIGNAL(hatchConnectedRegion(int,int,int,Vec,Vec,int,int,int,int)),
	  this, SLOT(hatchConnectedRegion(int,int,int,Vec,Vec,int,int,int,int)));

  connect(m_viewer, SIGNAL(connectedRegion(int,int,int,Vec,Vec,int,int)),
	  this, SLOT(connectedRegion(int,int,int,Vec,Vec,int,int)));

connect(m_viewer, SIGNAL(smoothConnectedRegion(int,int,int,Vec,Vec,int,int)),
	  this, SLOT(smoothConnectedRegion(int,int,int,Vec,Vec,int,int)));

connect(m_viewer, SIGNAL(smoothAllRegion(Vec,Vec,int,int)),
	this, SLOT(smoothAllRegion(Vec,Vec,int,int)));

  connect(m_viewer, SIGNAL(mergeTags(Vec, Vec, int, int, bool)),
	  this, SLOT(mergeTags(Vec, Vec, int, int, bool)));

  connect(m_viewer, SIGNAL(stepTag(Vec, Vec, int, int)),
	  this, SLOT(stepTags(Vec, Vec, int, int)));

  connect(m_viewer, SIGNAL(dilateConnected(int,int,int,Vec,Vec,int,bool)),
	  this, SLOT(dilateConnected(int,int,int,Vec,Vec,int,bool)));

  connect(m_viewer, SIGNAL(erodeConnected(int,int,int,Vec,Vec,int)),
	  this, SLOT(erodeConnected(int,int,int,Vec,Vec,int)));

  connect(m_viewer, SIGNAL(dilateAll(Vec,Vec,int,int)),
	  this, SLOT(dilateAll(Vec,Vec,int,int)));

  connect(m_viewer, SIGNAL(erodeAll(Vec,Vec,int,int)),
	  this, SLOT(erodeAll(Vec,Vec,int,int)));

  connect(m_viewer, SIGNAL(tagUsingSketchPad(Vec,Vec)),
	  this, SLOT(tagUsingSketchPad(Vec,Vec)));

  connect(m_viewer, SIGNAL(setVisible(Vec, Vec, int, bool)),
	  this, SLOT(setVisible(Vec, Vec, int, bool)));

  connect(m_viewer, SIGNAL(resetTag(Vec, Vec, int)),
	  this, SLOT(resetTag(Vec, Vec, int)));

  connect(m_viewer, SIGNAL(reloadMask()),
	  this, SLOT(reloadMask()));

  connect(m_viewer, SIGNAL(loadRawMask(QString)),
	  this, SLOT(loadRawMask(QString)));

//  connect(m_viewer, SIGNAL(updateSliceBounds(Vec, Vec)),
//	  this, SLOT(updateSliceBounds(Vec, Vec)));

  connect(m_viewer, SIGNAL(shrinkwrap(Vec, Vec, int, bool, int)),
	  this, SLOT(shrinkwrap(Vec, Vec, int, bool, int)));

  connect(m_viewer, SIGNAL(shrinkwrap(Vec, Vec, int, bool, int,
				      bool, int, int, int, int)),
	  this, SLOT(shrinkwrap(Vec, Vec, int, bool, int,
				bool, int, int, int, int)));

  connect(m_viewer, SIGNAL(tagTubes(Vec, Vec, int)),
	  this, SLOT(tagTubes(Vec, Vec, int)));

  connect(m_viewer, SIGNAL(tagTubes(Vec, Vec, int,
				      bool, int, int, int, int)),
	  this, SLOT(tagTubes(Vec, Vec, int,
				bool, int, int, int, int)));

  connect(m_viewer, SIGNAL(modifyOriginalVolume(Vec, Vec, int)),
	  this, SLOT(modifyOriginalVolume(Vec, Vec, int)));

#endif
