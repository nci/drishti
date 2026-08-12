#include "../common.h"
#include "../volinterface.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPluginLoader>
#include <QProcess>
#include <QProgressDialog>
#include <QTemporaryDir>
#include <QTimer>
#include <QVector>

#include <iostream>

namespace
{
int fail(const QString &message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

bool writePgm(const QString &fileName, int width, int height, int offset=0)
{
  QByteArray contents = QByteArray("P5\n") + QByteArray::number(width) + " " +
                        QByteArray::number(height) + "\n255\n";
  for (int y=0; y<height; ++y)
    for (int x=0; x<width; ++x)
      contents.append(static_cast<char>((offset+y*width+x) & 0xff));

  QFile file(fileName);
  return file.open(QIODevice::WriteOnly) &&
         file.write(contents) == contents.size();
}

bool compress(const QString &compressor, const QString &input,
              const QString &output, QString *error)
{
  QProcess process;
  process.start(compressor, QStringList() << "-i" << input << "-o" << output
                                         << "-n" << "3");
  if (!process.waitForStarted(10000) || !process.waitForFinished(60000))
    {
      *error = process.errorString();
      return false;
    }
  if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
      *error = QString::fromLocal8Bit(process.readAllStandardError());
      return false;
    }
  return QFileInfo(output).size() > 0;
}

bool checkImage(VolInterface *plugin, const QString &fileName, QString *error)
{
  if (!plugin->setFile(QStringList() << fileName))
    {
      *error = QString("plugin rejected %1").arg(fileName);
      return false;
    }

  int depth = 0;
  int width = 0;
  int height = 0;
  plugin->gridSize(depth, width, height);
  if (depth != 1 || width != 32 || height != 24 ||
      plugin->voxelType() != _UChar)
    {
      *error = QString("unexpected layout %1 x %2 x %3").arg(depth)
                 .arg(width).arg(height);
      return false;
    }

  quint64 histogramTotal = 0;
  for (uint count : plugin->histogram())
    histogramTotal += count;
  if (histogramTotal != 32*24 || plugin->rawMin() != 0 ||
      plugin->rawMax() != 255 || plugin->rawValue(0, 1, 2).toUInt() != 65)
    {
      *error = "histogram, range, or point lookup is incorrect";
      return false;
    }

  QVector<uchar> slice(width*height, 0);
  plugin->getDepthSlice(0, slice.data());
  if (slice[1*height+2] != 65)
    {
      *error = "depth-slice transpose is incorrect";
      return false;
    }

  for (int iteration=0; iteration<100; ++iteration)
    if (plugin->rawValue(0, iteration%width, iteration%height).toUInt() > 255)
      {
        *error = "repeated point lookup returned an invalid value";
        return false;
      }
  return true;
}

QString lastError(QObject *object)
{
  QString result;
  QMetaObject::invokeMethod(object, "lastError", Qt::DirectConnection,
                            Q_RETURN_ARG(QString, result));
  return result;
}

bool wasCanceled(QObject *object)
{
  bool result = false;
  QMetaObject::invokeMethod(object, "wasCanceled", Qt::DirectConnection,
                            Q_RETURN_ARG(bool, result));
  return result;
}

bool checkLayout(VolInterface *plugin, int depth, int width, int height)
{
  int actualDepth = 0;
  int actualWidth = 0;
  int actualHeight = 0;
  plugin->gridSize(actualDepth, actualWidth, actualHeight);
  return actualDepth == depth && actualWidth == width &&
         actualHeight == height && plugin->voxelType() == _UChar;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc != 3)
    return fail("Usage: jp2_plugin_smoke <jp2plugin.dll> <opj_compress.exe>");

  const QFileInfo pluginFile(
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath());
  QDir runtimeDirectory = pluginFile.absoluteDir();
  if (!runtimeDirectory.cdUp())
    return fail("Cannot locate the JPEG2000 plugin runtime directory");
  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
          QFile::encodeName(runtimeDirectory.filePath("platforms")));

  QApplication application(argc, argv);
  QTemporaryDir directory;
  if (!directory.isValid())
    return fail("Cannot create the JPEG2000 fixture directory");

  const QString pgm = directory.filePath("input.pgm");
  const QString smallPgm = directory.filePath("small.pgm");
  const QString jp2 = directory.filePath("input.jp2");
  const QString j2k = directory.filePath("input.j2k");
  const QString smallJp2 = directory.filePath("small.jp2");
  const QString truncatedJp2 = directory.filePath("truncated.jp2");
  const QString replacementStack = directory.filePath("replacement-stack");
  const QString orderingStack = directory.filePath("ordering-stack");
  const QString orderingPgm1 = directory.filePath("ordering-1.pgm");
  const QString orderingPgm2 = directory.filePath("ordering-2.pgm");
  const QString orderingPgm10 = directory.filePath("ordering-10.pgm");
  const QString orderingJp1 = directory.filePath("ordering-1.jp2");
  const QString orderingJp2 = directory.filePath("ordering-2.jp2");
  const QString orderingJp10 = directory.filePath("ordering-10.jp2");
  const QString unicodeDirectory = directory.filePath(
    QString(QChar(0x6d4b))+QChar(0x8bd5));
  const QString unicodeJp2 = QDir(unicodeDirectory).filePath("slice.jp2");
  if (!writePgm(pgm, 32, 24) || !writePgm(smallPgm, 16, 24) ||
      !writePgm(orderingPgm1, 32, 24, 10) ||
      !writePgm(orderingPgm2, 32, 24, 20) ||
      !writePgm(orderingPgm10, 32, 24, 100))
    return fail("Cannot write the PGM fixtures");
  QString error;
  const QString compressor = QString::fromLocal8Bit(argv[2]);
  if (!compress(compressor, pgm, jp2, &error) ||
      !compress(compressor, pgm, j2k, &error) ||
      !compress(compressor, smallPgm, smallJp2, &error) ||
      !compress(compressor, orderingPgm1, orderingJp1, &error) ||
      !compress(compressor, orderingPgm2, orderingJp2, &error) ||
      !compress(compressor, orderingPgm10, orderingJp10, &error))
    return fail(QString("Cannot encode JPEG2000 fixtures: %1").arg(error));
  if (!QDir().mkpath(replacementStack) ||
      !QFile::copy(jp2, QDir(replacementStack).filePath("slice-1.jp2")) ||
      !QFile::copy(jp2, QDir(replacementStack).filePath("slice-2.jp2")))
    return fail("Cannot create the JPEG2000 replacement stack");
  if (!QDir().mkpath(orderingStack) ||
      !QFile::copy(orderingJp1, QDir(orderingStack).filePath("slice-1.jp2")) ||
      !QFile::copy(orderingJp2, QDir(orderingStack).filePath("slice-2.jp2")) ||
      !QFile::copy(orderingJp10,
                   QDir(orderingStack).filePath("slice-10.Jp2")))
    return fail("Cannot create the JPEG2000 ordering stack");
  if (!QDir().mkpath(unicodeDirectory) || !QFile::copy(jp2, unicodeJp2))
    return fail("Cannot create the Unicode-path JPEG2000 fixture");
  if (!QFile::copy(jp2, truncatedJp2))
    return fail("Cannot create the truncated JPEG2000 fixture");
  QFile truncated(truncatedJp2);
  if (!truncated.open(QIODevice::ReadWrite) ||
      !truncated.resize(qMax<qint64>(1, truncated.size()/2)))
    return fail("Cannot truncate the JPEG2000 fixture");
  truncated.close();

  QPluginLoader loader(pluginFile.absoluteFilePath());
  QObject *pluginObject = loader.instance();
  if (!pluginObject)
    return fail(QString("Cannot load JPEG2000 plugin: %1")
                  .arg(loader.errorString()));
  VolInterface *plugin = qobject_cast<VolInterface *>(pluginObject);
  if (!plugin)
    return fail("Loaded object does not implement VolInterface");
  plugin->init();

  QDir().mkdir(directory.filePath("empty"));
  if (plugin->setFile(QStringList() << directory.filePath("empty")) ||
      lastError(pluginObject).isEmpty())
    return fail("An empty JPEG2000 directory was not rejected with an error");
  if (!plugin->setFile(QStringList() << orderingStack) ||
      !checkLayout(plugin, 3, 32, 24) ||
      plugin->rawValue(0, 0, 0).toUInt() != 10 ||
      plugin->rawValue(1, 0, 0).toUInt() != 20 ||
      plugin->rawValue(2, 0, 0).toUInt() != 100)
    return fail(QString("JPEG2000 directory slices are not naturally ordered: %1")
                  .arg(lastError(pluginObject)));
  if (!checkImage(plugin, jp2, &error))
    return fail(QString("JP2 regression failed: %1").arg(error));
  if (!checkImage(plugin, unicodeJp2, &error))
    return fail(QString("Unicode-path JP2 regression failed: %1").arg(error));

  if (plugin->setFile(QStringList() << truncatedJp2) ||
      lastError(pluginObject).isEmpty() ||
      !checkLayout(plugin, 1, 32, 24))
    return fail("A truncated JPEG2000 input did not preserve the active volume");
  if (plugin->rawValue(0, 1, 2).toUInt() != 65)
    return fail("A truncated JPEG2000 input damaged the active pixels");

  if (plugin->setFile(QStringList() << jp2 << smallJp2) ||
      lastError(pluginObject).isEmpty() ||
      !checkLayout(plugin, 1, 32, 24))
    return fail("A mixed-layout JPEG2000 stack did not preserve the active volume");
  if (plugin->rawValue(0, 1, 2).toUInt() != 65)
    return fail("A mixed-layout JPEG2000 stack damaged the active pixels");

  QTimer::singleShot(0, []()
  {
    for (QWidget *widget : QApplication::topLevelWidgets())
      if (QProgressDialog *progress = qobject_cast<QProgressDialog *>(widget))
        progress->cancel();
  });
  if (plugin->setFile(QStringList() << jp2 << jp2) ||
      !wasCanceled(pluginObject) || !checkLayout(plugin, 1, 32, 24))
    return fail("A canceled JPEG2000 scan did not preserve the active volume");
  if (plugin->rawValue(0, 1, 2).toUInt() != 65)
    return fail("A canceled JPEG2000 scan damaged the active pixels");

  QTimer statisticsCancelTimer;
  QObject::connect(&statisticsCancelTimer, &QTimer::timeout, []()
  {
    for (QWidget *widget : QApplication::topLevelWidgets())
      if (QProgressDialog *progress = qobject_cast<QProgressDialog *>(widget))
        if (progress->maximum() == 100)
          progress->cancel();
  });
  statisticsCancelTimer.start(0);
  const bool acceptedCanceledStatistics =
    plugin->setFile(QStringList() << jp2);
  statisticsCancelTimer.stop();
  if (acceptedCanceledStatistics || !wasCanceled(pluginObject) ||
      !checkLayout(plugin, 1, 32, 24))
    return fail("A canceled JPEG2000 statistics scan did not preserve the volume");
  if (plugin->rawValue(0, 1, 2).toUInt() != 65)
    return fail("A canceled JPEG2000 statistics scan damaged the active pixels");

  QTimer replacementCancelTimer;
  QObject::connect(&replacementCancelTimer, &QTimer::timeout, []()
  {
    for (QWidget *widget : QApplication::topLevelWidgets())
      if (QProgressDialog *progress = qobject_cast<QProgressDialog *>(widget))
        if (progress->labelText().contains("pixels", Qt::CaseInsensitive))
          progress->cancel();
  });
  replacementCancelTimer.start(0);
  plugin->replaceFile(replacementStack);
  replacementCancelTimer.stop();
  if (!wasCanceled(pluginObject) || !checkLayout(plugin, 1, 32, 24))
    return fail("A canceled JPEG2000 replacement did not preserve the volume");
  if (plugin->rawValue(0, 1, 2).toUInt() != 65)
    return fail("A canceled JPEG2000 replacement damaged the active pixels");

  plugin->replaceFile(truncatedJp2);
  if (lastError(pluginObject).isEmpty() ||
      !checkLayout(plugin, 1, 32, 24))
    return fail("A truncated JPEG2000 replacement was accepted");
  if (plugin->rawValue(0, 1, 2).toUInt() != 65)
    return fail("A truncated JPEG2000 replacement damaged the active pixels");

  plugin->replaceFile(smallJp2);
  if (lastError(pluginObject).isEmpty() ||
      !checkLayout(plugin, 1, 32, 24))
    return fail("An incompatible JPEG2000 replacement was accepted");
  if (plugin->rawValue(0, 1, 2).toUInt() != 65)
    return fail("An incompatible JPEG2000 replacement damaged the active pixels");

  plugin->replaceFile(j2k);
  if (!lastError(pluginObject).isEmpty() ||
      !checkLayout(plugin, 1, 32, 24) ||
      plugin->rawValue(0, 1, 2).toUInt() != 65)
    return fail("A compatible JPEG2000 replacement failed");

  if (!checkImage(plugin, j2k, &error))
    return fail(QString("J2K regression failed: %1").arg(error));

  plugin->clear();
  std::cout << "JPEG2000 plugin smoke passed" << std::endl;
  return 0;
}
