#include "pluginthread.h"

//---------------------------------------
// run plugin code in a thread
//---------------------------------------
PluginThread::PluginThread(QObject *parent) : QThread(parent)
{
  m_pluginInterface = 0;
}
void PluginThread::setPlugin(RenderPluginInterface *rpi) { m_pluginInterface = rpi; }
void PluginThread::run() { m_pluginInterface->start(); }

