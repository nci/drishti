#include <QtGui>
#include "meshplugin.h"
#include "meshsimplify.h"

#include <fstream>
using namespace std;

QStringList
MeshSimplifyPlugin::registerPlugin()
{
  QStringList regString;

  regString << "Mesh Simplify";

  return regString;
}

void
MeshSimplifyPlugin::init()
{
  m_pvlFileManager = 0;
  m_lodFileManager = 0;
  m_clipPos.clear();
  m_clipNormal.clear();
  m_crops.clear();
  m_paths.clear();
  m_lutSize = 0;
  m_dataMin = m_dataMax = Vec(0,0,0);
  m_voxelScaling = Vec(1,1,1);
  m_previousDirectory = qApp->applicationDirPath();
  m_pruneLod = m_pruneX = m_pruneY = m_pruneZ = 0;
  m_pruneData.clear();
  m_tagColors.clear();

  m_batchMode = false;
}

void MeshSimplifyPlugin::setPvlFileManager(VolumeFileManager *p) { m_pvlFileManager = p; }
void MeshSimplifyPlugin::setLodFileManager(VolumeFileManager *l) { m_lodFileManager = l; }
void MeshSimplifyPlugin::setClipInfo(QList<Vec> cp, QList<Vec> cn) { m_clipPos = cp; m_clipNormal = cn; }
void MeshSimplifyPlugin::setCropInfo(QList<CropObject> c) { m_crops = c; }
void MeshSimplifyPlugin::setSamplingLevel(int s) { m_samplingLevel = s; }
void MeshSimplifyPlugin::setDataLimits(Vec dmin, Vec dmax) { m_dataMin = dmin; m_dataMax = dmax; }
void MeshSimplifyPlugin::setVoxelScaling(Vec v) { m_voxelScaling = v; }
void MeshSimplifyPlugin::setPreviousDirectory(QString d) { m_previousDirectory = d; }
void MeshSimplifyPlugin::setLookupTable(int ls, QImage li) { m_lutSize = ls; m_lutImage = li; }

void MeshSimplifyPlugin::setPathInfo(QList<PathObject> c)
{
  m_paths.clear();
  for(int i=0; i<c.count(); i++)
    {
      if (c[i].useType() > 0) // add only if used for crop/blend
	m_paths << c[i];
    }
}

void
MeshSimplifyPlugin::setPruneData(int lod,
			 int px, int py, int pz,
			 QVector<uchar> p)
{
  m_pruneLod = lod;
  m_pruneX = px;
  m_pruneY = py;
  m_pruneZ = pz;
  m_pruneData = p;
}

void MeshSimplifyPlugin::setTagColors(QVector<uchar> t) { m_tagColors = t; }

void MeshSimplifyPlugin::setBatchMode(bool b) { m_batchMode = b; }

void
MeshSimplifyPlugin::start()
{
  MeshSimplify meshSimplify;
  meshSimplify.start(m_previousDirectory);
}
