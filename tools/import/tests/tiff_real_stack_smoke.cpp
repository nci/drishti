#include "../common.h"
#include "../volinterface.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QPluginLoader>
#include <QProgressDialog>
#include <QTextStream>
#include <QTimer>
#include <QVector>

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
QString pluginLastError(QObject *object)
{
  QString error;
  QMetaObject::invokeMethod(object, "lastError", Qt::DirectConnection,
                            Q_RETURN_ARG(QString, error));
  return error;
}

int fail(const QString& message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc != 4 && argc != 5)
    return fail("Usage: tiff_real_stack_smoke <volume-plugin.dll> "
                "<image-directory> <expected-slices> [files]");

  bool validCount = false;
  const int expectedSlices = QString::fromLocal8Bit(argv[3]).toInt(&validCount);
  if (!validCount || expectedSlices <= 0)
    return fail("The expected slice count is invalid");

  const QString pluginPath =
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
  QDir runtimeDirectory = QFileInfo(pluginPath).absoluteDir();
  if (!runtimeDirectory.cdUp())
    return fail("Cannot locate the TIFF plugin runtime directory");
  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
          QFile::encodeName(runtimeDirectory.filePath("platforms")));

  QApplication application(argc, argv);
  std::cerr << "checkpoint: application" << std::endl;
  QTextStream output(stdout);

  const QString imageDirectory =
    QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath();
  const bool passIndividualFiles = argc == 5;
  if (passIndividualFiles &&
      QString::fromLocal8Bit(argv[4]).compare("files", Qt::CaseInsensitive) != 0)
    return fail("The optional input mode must be 'files'");

  QStringList pluginInput;
  if (passIndividualFiles)
    {
      QDir directory(imageDirectory);
      const QFileInfoList images = directory.entryInfoList(
        QStringList() << "*.tif" << "*.tiff",
        QDir::Files | QDir::Readable,
        QDir::Name);
      for (const QFileInfo& image : images)
        pluginInput << image.absoluteFilePath();
      if (pluginInput.size() != expectedSlices)
        return fail(QString("Found %1 readable TIFF files, expected %2")
                      .arg(pluginInput.size()).arg(expectedSlices));
    }
  else
    pluginInput << imageDirectory;

  QPluginLoader loader(pluginPath);
  QObject *pluginObject = loader.instance();
  if (!pluginObject)
    return fail(QString("Cannot load volume plugin: %1").arg(loader.errorString()));
  std::cerr << "checkpoint: plugin loaded" << std::endl;

  VolInterface *plugin = qobject_cast<VolInterface *>(pluginObject);
  if (!plugin)
    return fail("The loaded object does not implement VolInterface");
  plugin->init();
  std::cerr << "checkpoint: plugin initialized" << std::endl;

  QStringList dialogErrors;
  QTimer dialogTimer;
  QObject::connect(&dialogTimer, &QTimer::timeout, [&]()
  {
    for (QWidget *widget : QApplication::topLevelWidgets())
      if (QMessageBox *message = qobject_cast<QMessageBox *>(widget))
        {
          dialogErrors << message->text();
          message->accept();
        }
      else if (QInputDialog *input = qobject_cast<QInputDialog *>(widget))
        input->accept();
  });
  dialogTimer.start(25);

  QElapsedTimer timer;
  timer.start();
  std::cerr << "checkpoint: setFile" << std::endl;
  if (!plugin->setFile(pluginInput))
    {
      dialogTimer.stop();
      return fail(QString("Volume plugin setFile failed: %1 %2")
                    .arg(pluginLastError(pluginObject), dialogErrors.join(" | ")));
    }
  std::cerr << "checkpoint: setFile complete" << std::endl;
  dialogTimer.stop();

  int depth = 0;
  int width = 0;
  int height = 0;
  plugin->gridSize(depth, width, height);
  if (depth != expectedSlices || width != 1024 || height != 1024 ||
      plugin->voxelType() != _UShort)
    return fail(QString("Unexpected volume contract: %1 x %2 x %3, type %4")
                  .arg(depth).arg(width).arg(height).arg(plugin->voxelType()));

  const QList<uint> histogram = plugin->histogram();
  quint64 histogramTotal = 0;
  for (uint count : histogram)
    histogramTotal += count;
  const quint64 expectedVoxels =
    static_cast<quint64>(depth)*width*height;
  if (histogram.size() != 65536 || histogramTotal != expectedVoxels ||
      !std::isfinite(plugin->rawMin()) || !std::isfinite(plugin->rawMax()) ||
      plugin->rawMin() > plugin->rawMax())
    return fail(QString("Invalid volume statistics: bins=%1 total=%2 range=%3..%4")
                  .arg(histogram.size()).arg(histogramTotal)
                  .arg(plugin->rawMin()).arg(plugin->rawMax()));

  const quint64 sliceBytes64 =
    static_cast<quint64>(width)*height*sizeof(quint16);
  if (sliceBytes64 > static_cast<quint64>(std::numeric_limits<int>::max()))
    return fail("The test slice exceeds QVector's supported size");

  QVector<quint16> slice(width*height, 0);
  QCryptographicHash hash(QCryptographicHash::Sha256);
  for (int index=0; index<depth; ++index)
    {
      plugin->getDepthSlice(index,
        reinterpret_cast<uchar *>(slice.data()));
      const QString error = pluginLastError(pluginObject);
      if (!error.isEmpty())
        return fail(QString("Volume slice %1 failed: %2").arg(index).arg(error));
      hash.addData(reinterpret_cast<const char *>(slice.constData()),
                   static_cast<int>(sliceBytes64));

      if (index == 0 || index == depth/2 || index == depth-1)
        {
          const int w = width/3;
          const int h = height/3;
          const quint16 expected = slice[w*height+h];
          if (plugin->rawValue(index, w, h).toUInt() != expected)
            return fail(QString("Volume rawValue mismatch at slice %1").arg(index));
        }
    }

  output << (passIndividualFiles ?
             "Living Ant image-stack smoke passed" :
             "Living Ant TIFF stack smoke passed")
         << " slices=" << depth
         << " dimensions=" << width << "x" << height
         << " histogram_total=" << histogramTotal
         << " raw_range=" << plugin->rawMin() << ".." << plugin->rawMax()
         << " elapsed_ms=" << timer.elapsed()
         << " sha256=" << hash.result().toHex()
         << Qt::endl;
  plugin->clear();
  return 0;
}
