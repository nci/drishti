#include "../common.h"
#include "../volinterface.h"

#include <QApplication>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPluginLoader>
#include <QTemporaryDir>
#include <QVector>

#include <cmath>
#include <iostream>
#include <limits>

namespace
{
int fail(const QString &message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

QString pluginError(QObject *pluginObject)
{
  QString error;
  QMetaObject::invokeMethod(pluginObject, "lastError", Qt::DirectConnection,
                            Q_RETURN_ARG(QString, error));
  return error;
}

bool pluginCanceled(QObject *pluginObject)
{
  bool canceled = false;
  QMetaObject::invokeMethod(pluginObject, "wasCanceled", Qt::DirectConnection,
                            Q_RETURN_ARG(bool, canceled));
  return canceled;
}

bool writeNrrd(const QString &fileName, const QVector<qint16> &values)
{
  if (values.size() != 8)
    return false;

  QFile output(fileName);
  if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate))
    return false;

  const QByteArray header(
    "NRRD0005\n"
    "type: short\n"
    "dimension: 3\n"
    "sizes: 2 2 2\n"
    "spacings: 0.5 0.6 0.7\n"
    "encoding: raw\n"
    "endian: little\n\n");
  if (output.write(header) != header.size())
    return false;

  QDataStream stream(&output);
  stream.setByteOrder(QDataStream::LittleEndian);
  for (qint16 value : values)
    stream << value;
  return stream.status() == QDataStream::Ok;
}

bool checkSlice(VolInterface *plugin, QObject *pluginObject,
                int sliceIndex, const QVector<qint16> &expected,
                QString &error)
{
  QVector<qint16> actual(4, 0);
  plugin->getDepthSlice(sliceIndex,
                        reinterpret_cast<uchar*>(actual.data()));
  error = pluginError(pluginObject);
  if (!error.isEmpty())
    return false;
  if (actual != expected)
    {
      error = QStringLiteral("decoded slice values differ");
      return false;
    }
  return true;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc != 2)
    return fail("Usage: nrrd_plugin_smoke <nrrdplugin.dll>");

  const QFileInfo pluginFile(
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath());
  QDir runtimeDirectory = pluginFile.absoluteDir();
  if (!runtimeDirectory.cdUp())
    return fail("Cannot locate the NRRD plugin runtime directory");
  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
          QFile::encodeName(runtimeDirectory.filePath("platforms")));

  QApplication application(argc, argv);
  QTemporaryDir fixtures;
  if (!fixtures.isValid())
    return fail("Cannot create the NRRD fixture directory");

  const QVector<qint16> first = {
    std::numeric_limits<qint16>::min(), -1, 0, 1,
    2, 3, 4, std::numeric_limits<qint16>::max()
  };
  const QVector<qint16> second = { 10, 11, 12, 13, 20, 21, 22, 23 };
  const QString firstFile = fixtures.filePath("first.nrrd");
  const QString secondFile = fixtures.filePath("second.nrrd");
  if (!writeNrrd(firstFile, first) || !writeNrrd(secondFile, second))
    return fail("Cannot write the NRRD fixtures");

  QPluginLoader loader(pluginFile.absoluteFilePath());
  QObject *pluginObject = loader.instance();
  if (!pluginObject)
    return fail(QString("Cannot load NRRD plugin: %1")
                  .arg(loader.errorString()));
  VolInterface *plugin = qobject_cast<VolInterface*>(pluginObject);
  if (!plugin)
    return fail("Loaded object does not implement VolInterface");

  plugin->init();
  if (plugin->setFile(QStringList()))
    return fail("NRRD empty input was accepted");
  if (pluginError(pluginObject).isEmpty())
    return fail("NRRD empty input did not expose lastError()");

  if (!plugin->setFile(QStringList() << firstFile))
    return fail(QString("Valid NRRD fixture was rejected: %1")
                  .arg(pluginError(pluginObject)));
  if (pluginCanceled(pluginObject))
    return fail("Successful NRRD import retained cancellation state");

  int depth = 0;
  int width = 0;
  int height = 0;
  plugin->gridSize(depth, width, height);
  float voxelX = 0;
  float voxelY = 0;
  float voxelZ = 0;
  plugin->voxelSize(voxelX, voxelY, voxelZ);
  if (depth != 2 || width != 2 || height != 2 ||
      plugin->voxelType() != _Short ||
      plugin->voxelUnit() != _Millimeter || plugin->headerBytes() != 0 ||
      plugin->rawMin() != -32768 || plugin->rawMax() != 32767 ||
      std::fabs(voxelX-0.5f) > 0.001f ||
      std::fabs(voxelY-0.6f) > 0.001f ||
      std::fabs(voxelZ-0.7f) > 0.001f)
    return fail("Unexpected NRRD volume contract");

  const QList<uint> histogram = plugin->histogram();
  quint64 histogramTotal = 0;
  for (uint count : histogram)
    histogramTotal += count;
  if (histogram.size() != 65536 || histogramTotal != 8 ||
      histogram.first() != 1 || histogram.last() != 1)
    return fail("Signed-short NRRD histogram is incorrect at its extrema");

  QString sliceError;
  if (!checkSlice(plugin, pluginObject, 0,
                  QVector<qint16>() << -32768 << -1 << 0 << 1,
                  sliceError) ||
      !checkSlice(plugin, pluginObject, 1,
                  QVector<qint16>() << 2 << 3 << 4 << 32767,
                  sliceError))
    return fail(QString("NRRD slice decoding failed: %1").arg(sliceError));
  if (plugin->rawValue(0, 0, 0).toInt() != -32768 ||
      plugin->rawValue(1, 1, 1).toInt() != 32767)
    return fail("NRRD rawValue coordinate mapping is incorrect");

  const float previousRawMinimum = plugin->rawMin();
  const float previousRawMaximum = plugin->rawMax();
  const QList<uint> previousHistogram = plugin->histogram();
  plugin->setMinMax(std::numeric_limits<float>::quiet_NaN(), 1.0f);
  if (pluginError(pluginObject).isEmpty() ||
      plugin->rawMin() != previousRawMinimum ||
      plugin->rawMax() != previousRawMaximum ||
      plugin->histogram() != previousHistogram)
    return fail("Invalid NRRD histogram range damaged active statistics");

  plugin->replaceFile(fixtures.filePath("missing.nrrd"));
  if (pluginError(pluginObject).isEmpty())
    return fail("Failed NRRD replacement did not expose lastError()");
  if (!checkSlice(plugin, pluginObject, 1,
                  QVector<qint16>() << 2 << 3 << 4 << 32767,
                  sliceError))
    return fail("Failed NRRD replacement corrupted the active volume");

  plugin->replaceFile(secondFile);
  if (!pluginError(pluginObject).isEmpty() ||
      !checkSlice(plugin, pluginObject, 1,
                  QVector<qint16>() << 20 << 21 << 22 << 23,
                  sliceError))
    return fail(QString("Valid NRRD replacement failed: %1")
                  .arg(pluginError(pluginObject)));

  plugin->clear();
  plugin->gridSize(depth, width, height);
  if (depth != 0 || width != 0 || height != 0 ||
      !plugin->histogram().isEmpty())
    return fail("NRRD clear() did not release its active volume state");

  std::cout << "NRRD plugin integration smoke passed" << std::endl;
  return 0;
}
