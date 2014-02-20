#include "itksegmentation.h"
#include "mopplugininterface.h"
#include <QMessageBox>

bool
ITKSegmentation::applyITKFilter(int nx, int ny, int nz,
				uchar* inVol,
				QList<Vec> seeds)
{
  //...................................
  // look in @executable_path/mopplugins
  QString sep = QDir::separator();
  QString pluginName;

#if defined(Q_OS_WIN32)
  // look in mopplugins
  QString plugindir = qApp->applicationDirPath() + QDir::separator() + "mopplugins";
  pluginName = plugindir + sep + "itkplugin.dll";
#endif
#ifdef Q_OS_MACX
  // look in drishti.app/mopplugins
  QString plugindir = qApp->applicationDirPath()+sep+".."+sep+".."+sep+"mopplugins";
  pluginName = plugindir + sep + "libitkplugin.dylib";
#endif
#if defined(Q_OS_LINUX)
  // look in mopplugins
  QString plugindir = qApp->applicationDirPath() + QDir::separator() + "mopplugins";
  pluginName = plugindir + sep + "libitkplugin.so";
#endif

  QPluginLoader pluginLoader(pluginName);
  QObject *plugin = pluginLoader.instance();

  if (!plugin)
    {
      QMessageBox::information(0, "Error", QString("Cannot load plugin %1").\
			       arg(pluginName));
      return false;
    }  

  MopPluginInterface *pluginInterface = qobject_cast<MopPluginInterface *>(plugin);
  if (!pluginInterface)
    {
      QMessageBox::information(0, "Error", QString("Cannot load plugin interface %1").\
			       arg(pluginName));
      return false;
    }

  pluginInterface->init();
  pluginInterface->setData(nx, ny, nz, inVol, seeds);
  return pluginInterface->start();
}
