#include "../common.h"
#include "../volinterface.h"

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

template <class T>
void writeLittleEndian(QByteArray *buffer, int offset, T value)
{
  const T encoded = qToLittleEndian(value);
  memcpy(buffer->data()+offset, &encoded, sizeof(T));
}

void writeFloat(QByteArray *buffer, int offset, float value)
{
  quint32 bits = 0;
  memcpy(&bits, &value, sizeof(bits));
  writeLittleEndian(buffer, offset, bits);
}

bool writeNifti(const QString &fileName, qint16 dataType, qint16 bitCount,
                const QByteArray &pixels)
{
  QByteArray contents(352, 0);
  writeLittleEndian<qint32>(&contents, 0, 348);
  writeLittleEndian<qint16>(&contents, 40, 3);
  writeLittleEndian<qint16>(&contents, 42, 3);
  writeLittleEndian<qint16>(&contents, 44, 2);
  writeLittleEndian<qint16>(&contents, 46, 2);
  writeLittleEndian<qint16>(&contents, 70, dataType);
  writeLittleEndian<qint16>(&contents, 72, bitCount);
  writeFloat(&contents, 80, 0.5f);
  writeFloat(&contents, 84, 0.75f);
  writeFloat(&contents, 88, 1.25f);
  writeFloat(&contents, 108, 352.0f);
  contents[123] = 2; // millimeters
  memcpy(contents.data()+344, "n+1\0", 4);
  contents.append(pixels);

  QFile file(fileName);
  return file.open(QIODevice::WriteOnly) &&
         file.write(contents) == contents.size();
}

template <class T>
QByteArray pixelBytes(const QVector<T> &values)
{
  QByteArray bytes(values.size()*static_cast<int>(sizeof(T)), 0);
  for (int index=0; index<values.size(); ++index)
    {
      T encoded = qToLittleEndian(values[index]);
      memcpy(bytes.data()+index*sizeof(T), &encoded, sizeof(T));
    }
  return bytes;
}

QByteArray floatPixelBytes(const QVector<float> &values)
{
  QByteArray bytes(values.size()*static_cast<int>(sizeof(float)), 0);
  for (int index=0; index<values.size(); ++index)
    writeFloat(&bytes, index*sizeof(float), values[index]);
  return bytes;
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

bool checkLayout(VolInterface *plugin, int voxelType)
{
  int depth = 0;
  int width = 0;
  int height = 0;
  plugin->gridSize(depth, width, height);
  return depth == 2 && width == 2 && height == 3 &&
         plugin->voxelType() == voxelType;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc != 2)
    return fail("Usage: nifti_plugin_smoke <niftiplugin.dll>");

  const QFileInfo pluginFile(
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath());
  QDir runtimeDirectory = pluginFile.absoluteDir();
  if (!runtimeDirectory.cdUp())
    return fail("Cannot locate the NIfTI plugin runtime directory");
  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
          QFile::encodeName(runtimeDirectory.filePath("platforms")));

  QApplication application(argc, argv);
  QTemporaryDir directory;
  if (!directory.isValid())
    return fail("Cannot create the NIfTI fixture directory");

  const QVector<qint8> charValues =
    QVector<qint8>() << -128 << -127 << -1 << 0 << 1 << 2
                     << 3 << 4 << 5 << 6 << 7 << 127;
  const QVector<qint16> shortValues =
    QVector<qint16>() << -32768 << -1 << 0 << 32767 << 1 << 2
                      << 3 << 4 << 5 << 6 << 7 << 8;
  const QVector<qint32> intValues =
    QVector<qint32>() << -2000000000 << -40000 << 0 << 123456789
                      << 4 << 5 << 6 << 7 << 8 << 9 << 10 << 11;
  const QVector<float> floatValues =
    QVector<float>() << -3.5f << -0.25f << 0.0f << 1.25f
                     << 2.5f << 3.75f << 4.0f << 5.0f
                     << 6.0f << 7.0f << 8.0f << 9.0f;
  const QVector<float> nonfiniteFloatValues =
    QVector<float>() << -2.0f << -1.0f << 0.0f << 1.0f
                     << 2.0f << 4.0f
                     << std::numeric_limits<float>::quiet_NaN()
                     << std::numeric_limits<float>::infinity()
                     << -std::numeric_limits<float>::infinity()
                     << 1.5f << 2.5f << 3.5f;

  const QVector<qint16> secondShortValues =
    QVector<qint16>() << 1000 << 1001 << 1002 << 1003 << 1004 << 1005
                      << 1006 << 1007 << 1008 << 1009 << 1010 << 1011;

  const QString charFile = directory.filePath("signed-char.nii");
  const QString shortFile = directory.filePath("signed-short.nii");
  const QString secondShortFile = directory.filePath("second-short.nii");
  const QString intFile = directory.filePath("signed-int.nii");
  const QString floatFile = directory.filePath("float.nii");
  const QString nonfiniteFloatFile = directory.filePath("nonfinite-float.nii");
  const QString corruptFile = directory.filePath("corrupt.nii");
  const QString truncatedShortFile = directory.filePath("truncated-short.nii");
  if (!writeNifti(charFile, 256, 8, pixelBytes(charValues)) ||
      !writeNifti(shortFile, 4, 16, pixelBytes(shortValues)) ||
      !writeNifti(secondShortFile, 4, 16, pixelBytes(secondShortValues)) ||
      !writeNifti(intFile, 8, 32, pixelBytes(intValues)) ||
      !writeNifti(floatFile, 16, 32, floatPixelBytes(floatValues)) ||
      !writeNifti(nonfiniteFloatFile, 16, 32,
                  floatPixelBytes(nonfiniteFloatValues)) ||
      !writeNifti(truncatedShortFile, 4, 16,
                  pixelBytes(QVector<qint16>() << 1)))
    return fail("Cannot write NIfTI fixtures");
  QFile corrupt(corruptFile);
  if (!corrupt.open(QIODevice::WriteOnly) || corrupt.write("bad", 3) != 3)
    return fail("Cannot write the corrupt NIfTI fixture");
  corrupt.close();

  QPluginLoader loader(pluginFile.absoluteFilePath());
  QObject *pluginObject = loader.instance();
  if (!pluginObject)
    return fail(QString("Cannot load NIfTI plugin: %1").arg(loader.errorString()));
  VolInterface *plugin = qobject_cast<VolInterface *>(pluginObject);
  if (!plugin)
    return fail("Loaded object does not implement VolInterface");
  plugin->init();

  if (plugin->setFile(QStringList()) || lastError(pluginObject).isEmpty())
    return fail("An empty NIfTI selection was not rejected with an error");

  if (!plugin->setFile(QStringList() << charFile) ||
      !lastError(pluginObject).isEmpty() || !checkLayout(plugin, _Char))
    return fail(QString("Signed-char NIfTI load failed: %1")
                  .arg(lastError(pluginObject)));
  const QList<uint> charHistogram = plugin->histogram();
  quint64 charHistogramTotal = 0;
  for (uint count : charHistogram)
    charHistogramTotal += count;
  if (charHistogram.size() != 256 || charHistogramTotal != 12 ||
      charHistogram[0] != 1 || charHistogram[255] != 1 ||
      plugin->rawMin() != -128 || plugin->rawMax() != 127 ||
      plugin->rawValue(0, 0, 0).toInt() != -128)
    return fail("Signed-char NIfTI histogram, range, or lookup is incorrect");

  if (!plugin->setFile(QStringList() << shortFile) ||
      !lastError(pluginObject).isEmpty() || !checkLayout(plugin, _Short))
    return fail(QString("Signed-short NIfTI load failed: %1")
                  .arg(lastError(pluginObject)));
  const QList<uint> histogram = plugin->histogram();
  quint64 histogramTotal = 0;
  for (uint count : histogram)
    histogramTotal += count;
  if (histogram.size() != 65536 || histogramTotal != 12 ||
      histogram[0] != 1 || histogram[65535] != 1 ||
      plugin->rawMin() != -32768 || plugin->rawMax() != 32767)
    return fail("Signed-short NIfTI histogram or range is incorrect");
  if (plugin->rawValue(0, 0, 0).toInt() != -32768 ||
      plugin->rawValue(0, 1, 0).toInt() != 32767)
    return fail("Signed-short NIfTI point lookup is incorrect");

  QVector<qint16> shortSlice(6, 0);
  plugin->getDepthSlice(0, reinterpret_cast<uchar*>(shortSlice.data()));
  for (int index=0; index<shortSlice.size(); ++index)
    if (shortSlice[index] != shortValues[index])
      return fail("Signed-short NIfTI slice pixels are incorrect");

  QTimer::singleShot(0, []()
  {
    for (QWidget *widget : QApplication::topLevelWidgets())
      if (QProgressDialog *progress = qobject_cast<QProgressDialog *>(widget))
        progress->cancel();
  });
  if (plugin->setFile(QStringList() << shortFile) ||
      !wasCanceled(pluginObject) || !checkLayout(plugin, _Short) ||
      plugin->rawValue(0, 0, 0).toInt() != -32768)
    return fail("A canceled NIfTI scan did not preserve the active volume");

  if (plugin->setFile(QStringList() << corruptFile) ||
      lastError(pluginObject).isEmpty() || !checkLayout(plugin, _Short) ||
      plugin->rawValue(0, 0, 0).toInt() != -32768)
    return fail("A corrupt NIfTI input did not preserve the active volume");

  plugin->replaceFile(secondShortFile);
  if (!lastError(pluginObject).isEmpty() ||
      plugin->rawValue(0, 0, 0).toInt() != 1000 ||
      plugin->histogram() != histogram)
    return fail(QString("Compatible NIfTI replacement failed: %1")
                  .arg(lastError(pluginObject)));

  plugin->replaceFile(truncatedShortFile);
  if (lastError(pluginObject).isEmpty() ||
      plugin->rawValue(0, 0, 0).toInt() != 1000)
    return fail("A truncated NIfTI replacement damaged the active volume");

  plugin->replaceFile(intFile);
  if (lastError(pluginObject).isEmpty() ||
      plugin->rawValue(0, 0, 0).toInt() != 1000)
    return fail("Incompatible NIfTI replacement damaged the active volume");

  if (!plugin->setFile(QStringList() << intFile) ||
      !checkLayout(plugin, _Int) ||
      plugin->rawValue(0, 1, 0).toInt() != 123456789)
    return fail(QString("Signed-int NIfTI lookup failed: %1")
                  .arg(lastError(pluginObject)));

  QFile originalIntFile(intFile);
  QFile replacementFloatFile(floatFile);
  if (!originalIntFile.open(QIODevice::ReadOnly) ||
      !replacementFloatFile.open(QIODevice::ReadOnly))
    return fail("Cannot open NIfTI mutation fixtures");
  const QByteArray originalIntContents = originalIntFile.readAll();
  const QByteArray replacementFloatContents = replacementFloatFile.readAll();
  originalIntFile.close();
  replacementFloatFile.close();
  if (!originalIntFile.open(QIODevice::WriteOnly|QIODevice::Truncate) ||
      originalIntFile.write(replacementFloatContents) !=
        replacementFloatContents.size())
    return fail("Cannot mutate the active NIfTI fixture");
  originalIntFile.close();
  if (plugin->rawValue(0, 1, 0).isValid() || lastError(pluginObject).isEmpty())
    return fail("A changed NIfTI component type was read using stale metadata");
  if (!originalIntFile.open(QIODevice::WriteOnly|QIODevice::Truncate) ||
      originalIntFile.write(originalIntContents) != originalIntContents.size())
    return fail("Cannot restore the active NIfTI fixture");
  originalIntFile.close();
  if (plugin->rawValue(0, 1, 0).toInt() != 123456789 ||
      !lastError(pluginObject).isEmpty())
    return fail("The restored NIfTI fixture is no longer readable");

  if (!plugin->setFile(QStringList() << floatFile) ||
      !checkLayout(plugin, _Float) ||
      std::fabs(plugin->rawValue(0, 1, 0).toDouble()-1.25) > 0.0001)
    return fail(QString("Float NIfTI lookup failed: %1")
                  .arg(lastError(pluginObject)));

  const bool loadedNonfinite =
    plugin->setFile(QStringList() << nonfiniteFloatFile);
  const double decodedNonfinite = plugin->rawValue(1, 0, 0).toDouble();
  if (!loadedNonfinite ||
      !checkLayout(plugin, _Float) || plugin->rawMin() != -2.0f ||
      plugin->rawMax() != 4.0f || std::isinf(decodedNonfinite))
    return fail(QString("Non-finite NIfTI float data was not handled safely: "
                        "min=%1 max=%2 value=%3 error=%4")
                  .arg(plugin->rawMin()).arg(plugin->rawMax())
                  .arg(decodedNonfinite)
                  .arg(lastError(pluginObject)));
  quint64 nonfiniteHistogramTotal = 0;
  for (uint count : plugin->histogram())
    nonfiniteHistogramTotal += count;
  if (plugin->histogram().size() != 65536 ||
      nonfiniteHistogramTotal != 12)
    return fail("Non-finite NIfTI histogram accounting is incorrect");

  const float previousMin = plugin->rawMin();
  const float previousMax = plugin->rawMax();
  const QList<uint> previousHistogram = plugin->histogram();
  plugin->setMinMax(std::numeric_limits<float>::quiet_NaN(), 1.0f);
  if (lastError(pluginObject).isEmpty() || plugin->rawMin() != previousMin ||
      plugin->rawMax() != previousMax || plugin->histogram() != previousHistogram)
    return fail("An invalid NIfTI histogram range damaged the active statistics");

  plugin->clear();
  std::cout << "NIfTI plugin smoke passed" << std::endl;
  return 0;
}
