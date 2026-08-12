#include "../common.h"
#include "../volinterface.h"

#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPluginLoader>
#include <QTemporaryDir>
#include <QTimer>

#include <cmath>
#include <cstring>
#include <iostream>

namespace
{
int fail(const QString& message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

bool writeFixture(const QString& fileName, uchar code,
                  qint32 depth, qint32 width, qint32 height,
                  const QByteArray& payload)
{
  QFile file(fileName);
  if (!file.open(QFile::WriteOnly | QFile::Truncate))
    return false;
  return file.write(reinterpret_cast<const char*>(&code), sizeof(code)) ==
           sizeof(code) &&
         file.write(reinterpret_cast<const char*>(&depth), sizeof(depth)) ==
           sizeof(depth) &&
         file.write(reinterpret_cast<const char*>(&width), sizeof(width)) ==
           sizeof(width) &&
         file.write(reinterpret_cast<const char*>(&height), sizeof(height)) ==
           sizeof(height) &&
         file.write(payload) == payload.size();
}

QString pluginError(QObject *pluginObject)
{
  QString error;
  if (!QMetaObject::invokeMethod(pluginObject, "lastError",
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(QString, error)))
    return "lastError() could not be invoked";
  return error;
}

bool pluginCanceled(QObject *pluginObject)
{
  bool canceled = false;
  return QMetaObject::invokeMethod(pluginObject, "wasCanceled",
                                   Qt::DirectConnection,
                                   Q_RETURN_ARG(bool, canceled)) && canceled;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc != 2)
    return fail("usage: native_raw_plugin_smoke <rawplugin.dll>");

  const QFileInfo pluginFile(
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath());
  QDir runtimeDirectory = pluginFile.absoluteDir();
  if (!runtimeDirectory.cdUp())
    return fail("cannot locate the RAW plugin runtime directory");
  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
          QFile::encodeName(runtimeDirectory.filePath("platforms")));

  QApplication application(argc, argv);
  QPluginLoader loader(pluginFile.absoluteFilePath());
  QObject *pluginObject = loader.instance();
  if (!pluginObject)
    return fail(QString("cannot load raw plugin: %1").arg(loader.errorString()));
  VolInterface *plugin = qobject_cast<VolInterface*>(pluginObject);
  if (!plugin)
    return fail("loaded object does not implement VolInterface");

  QTemporaryDir fixtures;
  if (!fixtures.isValid())
    return fail("cannot create temporary fixture directory");

  const QString signedFile = fixtures.filePath("signed-char.raw");
  QByteArray signedPayload;
  signedPayload.append(static_cast<char>(0x80));
  signedPayload.append(static_cast<char>(0x7f));
  if (!writeFixture(signedFile, 1, 1, 1, 2, signedPayload))
    return fail("cannot write signed-char fixture");

  plugin->init();
  QTimer cancelTimer;
  QObject::connect(&cancelTimer, &QTimer::timeout, []()
  {
    for (QWidget *widget : QApplication::topLevelWidgets())
      if (QDialog *dialog = qobject_cast<QDialog*>(widget))
        if (dialog->isVisible())
          dialog->reject();
  });
  cancelTimer.start(5);
  const bool canceledLoad = plugin->setFile(QStringList() << signedFile);
  cancelTimer.stop();
  if (canceledLoad || !pluginCanceled(pluginObject))
    return fail("canceling the RAW parameter dialog was not reported as cancellation");

  plugin->init();
  plugin->set4DVolume(true);
  plugin->setValue("skiprawdialog", 1);
  if (!plugin->setFile(QStringList() << signedFile))
    return fail("signed-char fixture was rejected");
  int depth = 0;
  int width = 0;
  int height = 0;
  plugin->gridSize(depth, width, height);
  if (plugin->voxelType() != _Char || depth != 1 || width != 1 || height != 2)
    return fail("signed-char metadata is incorrect");
  uchar slice[2] = { 0, 0 };
  plugin->getDepthSlice(0, slice);
  if (slice[0] != 0x80 || slice[1] != 0x7f || !pluginError(pluginObject).isEmpty())
    return fail("signed-char depth slice is incorrect");
  if (plugin->rawValue(0, 0, 0).toInt() != -128 ||
      plugin->rawValue(0, 0, 1).toInt() != 127)
    return fail("signed-char extreme voxel values are incorrect");

  QFile truncated(signedFile);
  if (!truncated.open(QFile::WriteOnly | QFile::Truncate) ||
      truncated.write("x", 1) != 1)
    return fail("cannot truncate signed-char fixture");
  truncated.close();
  slice[0] = slice[1] = 0xff;
  plugin->getDepthSlice(0, slice);
  if (slice[0] != 0 || slice[1] != 0 || pluginError(pluginObject).isEmpty())
    return fail("a post-load short read was not cleared and reported");

  const QString floatFile = fixtures.filePath("float.raw");
  const float expected = 3.5f;
  QByteArray floatPayload(sizeof(expected), 0);
  memcpy(floatPayload.data(), &expected, sizeof(expected));
  if (!writeFixture(floatFile, 8, 1, 1, 1, floatPayload))
    return fail("cannot write float fixture");

  plugin->init();
  plugin->set4DVolume(false);
  plugin->setValue("skiprawdialog", 1);
  if (!plugin->setFile(QStringList() << floatFile) ||
      plugin->voxelType() != _Float ||
      std::fabs(plugin->rawValue(0, 0, 0).toDouble()-expected) > 0.0001 ||
      plugin->rawMin() != expected || plugin->rawMax() != expected ||
      plugin->histogram().size() != 65536 ||
      plugin->histogram()[0] != 1)
    return fail("embedded float voxel type code 8 was not decoded correctly");
  if (pluginCanceled(pluginObject))
    return fail("a successful RAW load retained a stale cancellation state");

  std::cout << "Native RAW plugin smoke passed" << std::endl;
  return 0;
}
