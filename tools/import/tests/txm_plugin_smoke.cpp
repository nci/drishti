#include "../common.h"
#include "../volinterface.h"
#include "../plugins/txm/pole.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPluginLoader>
#include <QProgressDialog>
#include <QTemporaryDir>
#include <QTimer>
#include <QtEndian>
#include <QVector>

#include <cmath>
#include <cstring>
#include <iostream>
#include <limits>

namespace
{
int fail(const QString &message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

bool writeStream(POLE::Storage *storage, const char *name,
                 const void *data, int byteCount)
{
  POLE::Stream stream(storage, name, true, byteCount);
  if (stream.fail() ||
      stream.write(reinterpret_cast<uchar*>(const_cast<void*>(data)),
                   byteCount) != static_cast<POLE::uint64>(byteCount))
    return false;
  stream.flush();
  return !stream.fail();
}

bool writeTxm(const QString &fileName, int width, int height,
              const QVector<QByteArray> &images, bool truncateLast=false,
              int dataType=3, bool includeAuxiliaryImage=false)
{
  POLE::Storage storage(fileName.toUtf8().constData());
  if (!storage.open(true, true))
    return false;

  const int depth = images.size();
  bool ok = writeStream(&storage, "/ImageInfo/DataType",
                        &dataType, sizeof(dataType)) &&
            writeStream(&storage, "/ImageInfo/NoOfImages",
                        &depth, sizeof(depth)) &&
            writeStream(&storage, "/ImageInfo/ImageWidth",
                        &width, sizeof(width)) &&
            writeStream(&storage, "/ImageInfo/ImageHeight",
                        &height, sizeof(height));
  for (int index=0; ok && index<images.size(); ++index)
    {
      QByteArray pixels = images[index];
      if (truncateLast && index == images.size()-1 && !pixels.isEmpty())
        pixels.chop(1);
      const QByteArray name = QByteArray("/ImageData1/Image")+
                              QByteArray::number(index);
      ok = writeStream(&storage, name.constData(),
                       pixels.constData(), pixels.size());
    }
  if (ok && includeAuxiliaryImage)
    {
      const QByteArray preview(width*height, 0);
      ok = writeStream(&storage, "/ImageData1/ImagePreview",
                       preview.constData(), preview.size()) &&
           writeStream(&storage, "/ImageData1/Image+1",
                       preview.constData(), preview.size()) &&
           writeStream(&storage, "/ImageData1/Image 2",
                       preview.constData(), preview.size());
    }
  storage.close();
  return ok && QFileInfo(fileName).size() > 0;
}

bool patchStreamSize(const QString &fileName, const QByteArray &streamName,
                     quint64 streamSize)
{
  QFile file(fileName);
  if (!file.open(QIODevice::ReadWrite))
    return false;
  QByteArray contents = file.readAll();
  QByteArray encodedName;
  for (char character : streamName)
    {
      encodedName.append(character);
      encodedName.append('\0');
    }

  int entryOffset = -1;
  for (int offset=contents.indexOf(encodedName); offset >= 0;
       offset=contents.indexOf(encodedName, offset+1))
    if (offset%128 == 0 && offset+128 <= contents.size())
      {
        entryOffset = offset;
        break;
      }
  if (entryOffset < 0)
    return false;

  const quint64 encodedSize = qToLittleEndian(streamSize);
  memcpy(contents.data()+entryOffset+120, &encodedSize, sizeof(encodedSize));
  if (!file.seek(0) || file.write(contents) != contents.size())
    return false;
  return file.resize(contents.size());
}

QVector<QByteArray> makeImages(int width, int height, int base, int depth=2)
{
  QVector<QByteArray> images;
  for (int slice=0; slice<depth; ++slice)
    {
      QByteArray pixels(width*height, 0);
      for (int index=0; index<pixels.size(); ++index)
        pixels[index] = static_cast<char>(base+slice*width*height+index);
      images.append(pixels);
    }
  return images;
}

QVector<QByteArray> makeFloatImages()
{
  const float values[] = {
    -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 4.0f,
    std::numeric_limits<float>::quiet_NaN(),
    std::numeric_limits<float>::infinity(),
    -std::numeric_limits<float>::infinity(), 1.5f, 2.5f, 3.5f
  };
  QVector<QByteArray> images;
  for (int slice=0; slice<2; ++slice)
    {
      QByteArray pixels(6*static_cast<int>(sizeof(float)), 0);
      memcpy(pixels.data(), values+slice*6, pixels.size());
      images.append(pixels);
    }
  return images;
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

bool checkLayout(VolInterface *plugin, int depth, int width, int height,
                 int voxelType=_UChar)
{
  int actualDepth = 0;
  int actualWidth = 0;
  int actualHeight = 0;
  plugin->gridSize(actualDepth, actualWidth, actualHeight);
  return actualDepth == depth && actualWidth == width &&
         actualHeight == height && plugin->voxelType() == voxelType;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc != 2)
    return fail("Usage: txm_plugin_smoke <txmplugin.dll>");

  const QFileInfo pluginFile(
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath());
  QDir runtimeDirectory = pluginFile.absoluteDir();
  if (!runtimeDirectory.cdUp())
    return fail("Cannot locate the TXM plugin runtime directory");
  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
          QFile::encodeName(runtimeDirectory.filePath("platforms")));

  QApplication application(argc, argv);
  QTemporaryDir directory;
  if (!directory.isValid())
    return fail("Cannot create the TXM fixture directory");

  const QString firstFile = directory.filePath("first.txm");
  const QString secondFile = directory.filePath("second.txm");
  const QString truncatedFile = directory.filePath("truncated.txm");
  const QString incompatibleFile = directory.filePath("incompatible.txm");
  const QString orderingFile = directory.filePath("ordering.txm");
  const QString auxiliaryFile = directory.filePath("auxiliary.txm");
  const QString floatFile = directory.filePath("nonfinite-float.txm");
  const QString corruptContainer = directory.filePath("corrupt-container.txm");
  const QString unreadableReplacement =
    directory.filePath("unreadable-replacement.txm");
  QVector<QByteArray> unreadableImages = makeImages(3, 2, 70);
  unreadableImages[1].clear();
  if (!writeTxm(firstFile, 3, 2, makeImages(3, 2, 10)) ||
      !writeTxm(secondFile, 3, 2, makeImages(3, 2, 100)) ||
      !writeTxm(truncatedFile, 3, 2, makeImages(3, 2, 50), true) ||
      !writeTxm(incompatibleFile, 4, 2, makeImages(4, 2, 20)) ||
      !writeTxm(orderingFile, 3, 2, makeImages(3, 2, 5, 12)) ||
      !writeTxm(auxiliaryFile, 3, 2, makeImages(3, 2, 30), false, 3, true) ||
      !writeTxm(floatFile, 3, 2, makeFloatImages(), false, 10) ||
      !writeTxm(unreadableReplacement, 3, 2, unreadableImages) ||
      !patchStreamSize(unreadableReplacement, "Image1", 6))
    return fail("Cannot write TXM fixtures");
  if (!QFile::copy(firstFile, corruptContainer))
    return fail("Cannot create the corrupt TXM fixture");
  QFile corrupt(corruptContainer);
  if (!corrupt.open(QIODevice::ReadWrite) ||
      !corrupt.resize(qMax<qint64>(1, corrupt.size()/2)))
    return fail("Cannot truncate the TXM container fixture");
  corrupt.close();

  QPluginLoader loader(pluginFile.absoluteFilePath());
  QObject *pluginObject = loader.instance();
  if (!pluginObject)
    return fail(QString("Cannot load TXM plugin: %1").arg(loader.errorString()));
  VolInterface *plugin = qobject_cast<VolInterface *>(pluginObject);
  if (!plugin)
    return fail("Loaded object does not implement VolInterface");
  plugin->init();

  if (plugin->setFile(QStringList()) || lastError(pluginObject).isEmpty())
    return fail("An empty TXM selection was not rejected with an error");
  if (!plugin->setFile(QStringList() << firstFile) ||
      !checkLayout(plugin, 2, 3, 2) ||
      plugin->rawValue(0, 1, 1).toUInt() != 14)
    return fail(QString("Initial TXM load failed: %1").arg(lastError(pluginObject)));

  quint64 histogramTotal = 0;
  for (uint count : plugin->histogram())
    histogramTotal += count;
  if (histogramTotal != 12 || plugin->rawMin() != 10 || plugin->rawMax() != 21)
    return fail("Initial TXM histogram or range is incorrect");

  QVector<uchar> slice(6, 0);
  plugin->getDepthSlice(0, slice.data());
  const QVector<uchar> expected = QVector<uchar>() << 10 << 13 << 11
                                                   << 14 << 12 << 15;
  if (slice != expected)
    return fail("TXM depth-slice transpose is incorrect");

  if (plugin->setFile(QStringList() << corruptContainer) ||
      lastError(pluginObject).isEmpty() || !checkLayout(plugin, 2, 3, 2) ||
      plugin->rawValue(0, 1, 1).toUInt() != 14)
    return fail("A corrupt TXM container did not preserve the active volume");

  if (plugin->setFile(QStringList() << truncatedFile) ||
      lastError(pluginObject).isEmpty() || !checkLayout(plugin, 2, 3, 2) ||
      plugin->rawValue(0, 1, 1).toUInt() != 14)
    return fail("A truncated TXM input did not preserve the active volume");

  plugin->replaceFile(unreadableReplacement);
  if (lastError(pluginObject).isEmpty() ||
      !checkLayout(plugin, 2, 3, 2) ||
      plugin->rawValue(0, 1, 1).toUInt() != 14)
    return fail("An unreadable TXM replacement damaged the active volume");

  QTimer::singleShot(0, []()
  {
    for (QWidget *widget : QApplication::topLevelWidgets())
      if (QProgressDialog *progress = qobject_cast<QProgressDialog *>(widget))
        progress->cancel();
  });
  if (plugin->setFile(QStringList() << orderingFile) ||
      !wasCanceled(pluginObject) || !checkLayout(plugin, 2, 3, 2) ||
      plugin->rawValue(0, 1, 1).toUInt() != 14)
    return fail("A canceled TXM scan did not preserve the active volume");

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
    plugin->setFile(QStringList() << orderingFile);
  statisticsCancelTimer.stop();
  if (acceptedCanceledStatistics || !wasCanceled(pluginObject) ||
      !checkLayout(plugin, 2, 3, 2) ||
      plugin->rawValue(0, 1, 1).toUInt() != 14)
    return fail("A canceled TXM statistics scan did not preserve the active volume");

  if (!plugin->setFile(QStringList() << orderingFile) ||
      !checkLayout(plugin, 12, 3, 2) ||
      plugin->rawValue(10, 1, 1).toUInt() != 69)
    return fail(QString("TXM image streams are not naturally ordered: %1")
                  .arg(lastError(pluginObject)));

  if (!plugin->setFile(QStringList() << auxiliaryFile) ||
      !checkLayout(plugin, 2, 3, 2) ||
      plugin->rawValue(0, 1, 1).toUInt() != 34)
    return fail(QString("A TXM auxiliary image stream broke import: %1")
                  .arg(lastError(pluginObject)));

  if (!plugin->setFile(QStringList() << floatFile) ||
      !checkLayout(plugin, 2, 3, 2, _Float) ||
      plugin->rawMin() != -2.0f || plugin->rawMax() != 4.0f ||
      !std::isnan(plugin->rawValue(1, 0, 0).toDouble()))
    return fail(QString("Non-finite TXM float data was not handled safely: %1")
                  .arg(lastError(pluginObject)));
  quint64 floatHistogramTotal = 0;
  for (uint count : plugin->histogram())
    floatHistogramTotal += count;
  if (plugin->histogram().size() != 65536 || floatHistogramTotal != 12)
    return fail("Non-finite TXM histogram accounting is incorrect");

  plugin->setMinMax(-1.0f, 3.0f);
  quint64 remappedHistogramTotal = 0;
  for (uint count : plugin->histogram())
    remappedHistogramTotal += count;
  if (!lastError(pluginObject).isEmpty() || plugin->rawMin() != -1.0f ||
      plugin->rawMax() != 3.0f || remappedHistogramTotal != 12)
    return fail("A valid TXM float histogram range was not applied");
  const QList<uint> remappedHistogram = plugin->histogram();
  plugin->setMinMax(std::numeric_limits<float>::quiet_NaN(), 1.0f);
  if (lastError(pluginObject).isEmpty() || plugin->rawMin() != -1.0f ||
      plugin->rawMax() != 3.0f || plugin->histogram() != remappedHistogram)
    return fail("An invalid TXM histogram range damaged the active statistics");

  if (!plugin->setFile(QStringList() << firstFile))
    return fail("Cannot restore the initial TXM fixture");

  plugin->replaceFile(secondFile);
  if (!lastError(pluginObject).isEmpty() ||
      plugin->rawValue(0, 1, 1).toUInt() != 104)
    return fail(QString("Compatible TXM replacement failed: %1")
                  .arg(lastError(pluginObject)));

  plugin->replaceFile(incompatibleFile);
  if (lastError(pluginObject).isEmpty() ||
      plugin->rawValue(0, 1, 1).toUInt() != 104)
    return fail("Incompatible TXM replacement damaged the active volume");

  for (int iteration=0; iteration<1000; ++iteration)
    if (plugin->rawValue(iteration%2, iteration%3, iteration%2).toUInt() < 100)
      return fail("Repeated TXM point lookup returned an invalid value");

  plugin->clear();
  std::cout << "TXM plugin smoke passed" << std::endl;
  return 0;
}
