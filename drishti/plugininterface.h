#ifndef RENDERPLUGININTERFACE_H
#define RENDERPLUGININTERFACE_H

#include <QtCore>

#include "volumefilemanager.h"
#include "cropobject.h"
#include "pathobject.h"

class RenderPluginInterface
{
 public :
  virtual ~RenderPluginInterface() {}

  // register plugin with the main program
  virtual QStringList registerPlugin() = 0;

  virtual void setPvlFileManager(VolumeFileManager*)=0;
  virtual void setLodFileManager(VolumeFileManager*)=0;
  virtual void setClipInfo(QList<Vec>, QList<Vec>)=0;
  virtual void setCropInfo(QList<CropObject>)=0;
  virtual void setPathInfo(QList<PathObject>)=0;
  virtual void setLookupTable(int, QImage)=0;
  virtual void setSamplingLevel(int)=0;
  virtual void setDataLimits(Vec, Vec)=0;
  virtual void setVoxelScaling(Vec)=0;
  virtual void setPreviousDirectory(QString)=0;
  virtual void setPruneData(int, int, int, int, QVector<uchar>)=0;
  virtual void setTagColors(QVector<uchar>)=0;


  virtual void init() = 0;
  virtual void start() = 0;
};

QT_BEGIN_NAMESPACE
Q_DECLARE_INTERFACE(RenderPluginInterface,
		    "drishti.render.Plugin.PluginInterface/1.0");
QT_END_NAMESPACE

#endif

