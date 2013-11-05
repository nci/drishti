#ifndef PLUGINTHREAD_H
#define PLUGINTHREAD_H

#include <QtCore>
#include "plugininterface.h"

//---------------------------------------
// run plugin code in a thread
//---------------------------------------
class PluginThread : public QThread
{
  Q_OBJECT

 public :
  PluginThread(QObject *parent=0);

  void setPlugin(RenderPluginInterface *);
 protected :
  void run();
  
 private :
  RenderPluginInterface *m_pluginInterface;
};
//---------------------------------------

#endif
