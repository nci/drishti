#include "../volinterface.h"

#include <QCoreApplication>
#include <QPluginLoader>
#include <QPointer>

#include <iostream>

namespace
{
int fail(const QString &message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

QObject *loadPluginObject(const QString &fileName, QString *error)
{
  QPluginLoader loader(fileName);
  QObject *object = loader.instance();
  if (!object)
    *error = loader.errorString();
  return object;
}
}

int main(int argc, char **argv)
{
  QCoreApplication application(argc, argv);
  if (argc != 2)
    return fail("Usage: plugin_lifecycle_smoke <import-plugin.dll>");

  const QString pluginFile = QString::fromLocal8Bit(argv[1]);
  QString error;
  QObject *firstObject = loadPluginObject(pluginFile, &error);
  if (!firstObject)
    return fail(QString("Initial plugin load failed: %1").arg(error));

  VolInterface *first = qobject_cast<VolInterface *>(firstObject);
  if (!first || first->registerPlugin().isEmpty())
    return fail("Initial object does not implement a usable VolInterface");

  QPointer<QObject> firstGuard(firstObject);
  delete first;
  if (!firstGuard.isNull())
    return fail("Deleting the interface did not destroy the plugin object");

  error.clear();
  QObject *secondObject = loadPluginObject(pluginFile, &error);
  if (!secondObject)
    return fail(QString("Second plugin load failed: %1").arg(error));

  VolInterface *second = qobject_cast<VolInterface *>(secondObject);
  if (!second || second->registerPlugin().isEmpty())
    return fail("The plugin could not be used after deleting its first instance");

  delete second;
  std::cout << "Import plugin lifecycle smoke passed" << std::endl;
  return 0;
}
