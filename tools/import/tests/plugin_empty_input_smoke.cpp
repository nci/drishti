#include "../volinterface.h"

#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QPluginLoader>
#include <QPointer>
#include <QTimer>

#include <iostream>

namespace
{
int fail(const QString& message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

void closeModalDialogs()
{
  for (QWidget *widget : QApplication::topLevelWidgets())
    {
      if (!widget->isVisible())
        continue;
      if (QMessageBox *message = qobject_cast<QMessageBox*>(widget))
        message->accept();
      else if (QDialog *dialog = qobject_cast<QDialog*>(widget))
        dialog->reject();
    }
}

bool loadAndDestroy(const QString& pluginFile, bool testEmptyInput,
                    QString& error)
{
  QPluginLoader loader(pluginFile);
  QObject *object = loader.instance();
  if (!object)
    {
      error = loader.errorString();
      return false;
    }

  VolInterface *plugin = qobject_cast<VolInterface*>(object);
  if (!plugin || plugin->registerPlugin().isEmpty())
    {
      error = "The loaded object does not implement a usable VolInterface.";
      return false;
    }

  plugin->init();
  if (testEmptyInput)
    {
      QTimer dialogCloser;
      QObject::connect(&dialogCloser, &QTimer::timeout, closeModalDialogs);
      dialogCloser.start(5);
      const bool accepted = plugin->setFile(QStringList());
      dialogCloser.stop();
      closeModalDialogs();
      if (accepted)
        {
          error = "The plugin accepted an empty file selection.";
          return false;
        }
    }

  plugin->clear();
  QPointer<QObject> guard(object);
  delete plugin;
  if (!guard.isNull())
    {
      error = "Deleting VolInterface did not destroy the plugin object.";
      return false;
    }
  return true;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc == 2)
    {
      const QFileInfo pluginInfo(QString::fromLocal8Bit(argv[1]));
      const QDir runtimeDirectory(pluginInfo.absoluteDir().absolutePath() +
                                  "/..");
      qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
              runtimeDirectory.filePath("platforms").toLocal8Bit());
    }
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QApplication application(argc, argv);
  if (argc != 2)
    return fail("Usage: plugin_empty_input_smoke <import-plugin.dll>");

  const QString pluginFile =
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
  QString error;
  if (!loadAndDestroy(pluginFile, true, error))
    return fail(QString("Empty-input pass failed for %1: %2")
                  .arg(pluginFile, error));
  error.clear();
  if (!loadAndDestroy(pluginFile, false, error))
    return fail(QString("Reload pass failed for %1: %2")
                  .arg(pluginFile, error));

  std::cout << "Plugin empty-input and reload smoke passed" << std::endl;
  return 0;
}
