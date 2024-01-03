#ifndef RENDERPLUGININTERFACE_H
#define RENDERPLUGININTERFACE_H

#include <QtCore>
#include <QProgressDialog>
#include <QMessageBox>
#include <QInputDialog>

class RenderPluginInterface
{
 public :
  virtual ~RenderPluginInterface() {}

  // register plugin with the main program
  virtual QStringList registerPlugin() = 0;

  virtual void setPreviousDirectory(QString)=0;
  
  virtual void init() = 0;
  virtual void start() = 0;
};

Q_DECLARE_INTERFACE(RenderPluginInterface,
		    "drishti.render.Plugin.PluginInterface/1.0");

#endif

