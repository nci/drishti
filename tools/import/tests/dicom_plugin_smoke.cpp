#include "../common.h"
#include "../volinterface.h"

#include <QAbstractButton>
#include <QApplication>
#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPluginLoader>
#include <QProgressDialog>
#include <QTemporaryDir>
#include <QTimer>
#include <QVector>

#include <iostream>
#include <limits>

namespace
{
int fail(const QString &message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

QByteArray paddedValue(QByteArray value, char padding)
{
  if (value.size()%2 != 0)
    value.append(padding);
  return value;
}

QByteArray textValue(const char *value, const QByteArray &vr)
{
  return paddedValue(QByteArray(value), vr == "UI" ? '\0' : ' ');
}

QByteArray unsignedShortValue(quint16 value)
{
  QByteArray bytes;
  bytes.append(static_cast<char>(value & 0xff));
  bytes.append(static_cast<char>((value >> 8) & 0xff));
  return bytes;
}

QByteArray unsignedLongValue(quint32 value)
{
  QByteArray bytes;
  for (int shift=0; shift<32; shift += 8)
    bytes.append(static_cast<char>((value >> shift) & 0xff));
  return bytes;
}

bool writeElement(QDataStream &stream, quint16 group, quint16 element,
                  const QByteArray &vr, const QByteArray &value)
{
  if (vr.size() != 2 || value.size() < 0)
    return false;

  stream << group << element;
  if (stream.writeRawData(vr.constData(), vr.size()) != vr.size())
    return false;

  const bool longLength = vr == "OB" || vr == "OD" || vr == "OF" ||
                          vr == "OL" || vr == "OW" || vr == "SQ" ||
                          vr == "UC" || vr == "UN" || vr == "UR" ||
                          vr == "UT";
  if (longLength)
    stream << static_cast<quint16>(0) << static_cast<quint32>(value.size());
  else
    {
      if (value.size() > std::numeric_limits<quint16>::max())
        return false;
      stream << static_cast<quint16>(value.size());
    }

  return stream.status() == QDataStream::Ok &&
         stream.writeRawData(value.constData(), value.size()) == value.size();
}

template <typename PixelType>
bool writeDicomSlice(const QString &fileName, int instance, double z,
                     const QVector<PixelType> &pixels)
{
  const int rows = 2;
  const int columns = 3;
  if (pixels.size() != rows*columns)
    return false;

  const QByteArray sopClassUid =
    textValue("1.2.840.10008.5.1.4.1.1.2", "UI");
  const QByteArray sopInstanceUid = textValue(
    QString("1.2.826.0.1.3680043.10.543.100.%1").arg(instance)
      .toLatin1().constData(), "UI");
  const QByteArray studyUid =
    textValue("1.2.826.0.1.3680043.10.543.200", "UI");
  const QByteArray seriesUid =
    textValue("1.2.826.0.1.3680043.10.543.300", "UI");

  QByteArray metaBytes;
  QBuffer metaBuffer(&metaBytes);
  if (!metaBuffer.open(QIODevice::WriteOnly))
    return false;
  QDataStream metaStream(&metaBuffer);
  metaStream.setByteOrder(QDataStream::LittleEndian);
  if (!writeElement(metaStream, 0x0002, 0x0001, "OB",
                    QByteArray::fromHex("0001")) ||
      !writeElement(metaStream, 0x0002, 0x0002, "UI", sopClassUid) ||
      !writeElement(metaStream, 0x0002, 0x0003, "UI", sopInstanceUid) ||
      !writeElement(metaStream, 0x0002, 0x0010, "UI",
                    textValue("1.2.840.10008.1.2.1", "UI")) ||
      !writeElement(metaStream, 0x0002, 0x0012, "UI",
                    textValue("1.2.826.0.1.3680043.10.543.1", "UI")) ||
      !writeElement(metaStream, 0x0002, 0x0013, "SH",
                    textValue("DRISHTI_TEST", "SH")))
    return false;
  metaBuffer.close();

  QFile output(fileName);
  if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate) ||
      output.write(QByteArray(128, '\0')) != 128 ||
      output.write("DICM", 4) != 4)
    return false;

  QDataStream stream(&output);
  stream.setByteOrder(QDataStream::LittleEndian);
  if (!writeElement(stream, 0x0002, 0x0000, "UL",
                    unsignedLongValue(metaBytes.size())) ||
      stream.writeRawData(metaBytes.constData(), metaBytes.size()) !=
        metaBytes.size() ||
      !writeElement(stream, 0x0008, 0x0016, "UI", sopClassUid) ||
      !writeElement(stream, 0x0008, 0x0018, "UI", sopInstanceUid) ||
      !writeElement(stream, 0x0008, 0x0060, "CS", textValue("CT", "CS")) ||
      !writeElement(stream, 0x0018, 0x0050, "DS", textValue("1.5", "DS")) ||
      !writeElement(stream, 0x0020, 0x000d, "UI", studyUid) ||
      !writeElement(stream, 0x0020, 0x000e, "UI", seriesUid) ||
      !writeElement(stream, 0x0020, 0x0013, "IS",
                    textValue(QByteArray::number(instance).constData(), "IS")) ||
      !writeElement(stream, 0x0020, 0x0032, "DS",
                    textValue(QString("0\\0\\%1").arg(z, 0, 'f', 1)
                                .toLatin1().constData(), "DS")) ||
      !writeElement(stream, 0x0020, 0x0037, "DS",
                    textValue("1\\0\\0\\0\\1\\0", "DS")) ||
      !writeElement(stream, 0x0028, 0x0002, "US", unsignedShortValue(1)) ||
      !writeElement(stream, 0x0028, 0x0004, "CS",
                    textValue("MONOCHROME2", "CS")) ||
      !writeElement(stream, 0x0028, 0x0010, "US",
                    unsignedShortValue(rows)) ||
      !writeElement(stream, 0x0028, 0x0011, "US",
                    unsignedShortValue(columns)) ||
      !writeElement(stream, 0x0028, 0x0030, "DS",
                    textValue("0.5\\0.6", "DS")) ||
      !writeElement(stream, 0x0028, 0x0100, "US",
                    unsignedShortValue(16)) ||
      !writeElement(stream, 0x0028, 0x0101, "US",
                    unsignedShortValue(16)) ||
      !writeElement(stream, 0x0028, 0x0102, "US",
                    unsignedShortValue(15)) ||
      !writeElement(stream, 0x0028, 0x0103, "US",
                    unsignedShortValue(
                      std::numeric_limits<PixelType>::is_signed ? 1 : 0)) ||
      !writeElement(stream, 0x0028, 0x1052, "DS", textValue("0", "DS")) ||
      !writeElement(stream, 0x0028, 0x1053, "DS", textValue("1", "DS")))
    return false;

  QByteArray pixelBytes;
  pixelBytes.reserve(pixels.size()*2);
  for (PixelType pixel : pixels)
    {
      const quint16 bits = static_cast<quint16>(pixel);
      pixelBytes.append(static_cast<char>(bits & 0xff));
      pixelBytes.append(static_cast<char>((bits >> 8) & 0xff));
    }
  return writeElement(stream, 0x7fe0, 0x0010, "OW", pixelBytes) &&
         stream.status() == QDataStream::Ok;
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
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc != 2)
    return fail("Usage: dicom_plugin_smoke <dicomplugin.dll>");

  const QFileInfo pluginFile(
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath());
  QDir runtimeDirectory = pluginFile.absoluteDir();
  if (!runtimeDirectory.cdUp())
    return fail("Cannot locate the DICOM plugin runtime directory");
  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
          QFile::encodeName(runtimeDirectory.filePath("platforms")));

  QApplication application(argc, argv);
  QPluginLoader loader(pluginFile.absoluteFilePath());
  QObject *pluginObject = loader.instance();
  if (!pluginObject)
    return fail(QString("Cannot load DICOM plugin: %1")
                  .arg(loader.errorString()));
  VolInterface *plugin = qobject_cast<VolInterface*>(pluginObject);
  if (!plugin)
    return fail("Loaded object does not implement VolInterface");

  plugin->init();
  if (plugin->headerBytes() != 0)
    return fail("DICOM headerBytes was not initialized");

  QTemporaryDir fixtures;
  QString unicodeDirectoryName;
  unicodeDirectoryName.append(QChar(0x663e));
  unicodeDirectoryName.append(QChar(0x5fae));
  unicodeDirectoryName.append(QChar(0x6570));
  unicodeDirectoryName.append(QChar(0x636e));
  const QString firstSeriesDirectory =
    fixtures.filePath(unicodeDirectoryName + "/series-a");
  const QString secondSeriesDirectory = fixtures.filePath("sibling/series-b");
  if (!fixtures.isValid() ||
      !QDir().mkpath(firstSeriesDirectory) ||
      !QDir().mkpath(secondSeriesDirectory))
    return fail("Cannot create the DICOM fixture directory");

  const QVector<qint16> first = {
    std::numeric_limits<qint16>::min(), -1, 0, 1, 2, 3
  };
  const QVector<qint16> second = {
    4, 5, 6, 7, 8, std::numeric_limits<qint16>::max()
  };
  if (!writeDicomSlice(QDir(firstSeriesDirectory).filePath("slice-001.dcm"),
                       1, 0.0, first) ||
      !writeDicomSlice(QDir(secondSeriesDirectory).filePath("slice-002"),
                       2, 1.5, second))
    return fail("Cannot write DICOM fixtures");

  if (!plugin->setFile(QStringList() << fixtures.path()))
    return fail(QString("Synthetic DICOM series was rejected: %1")
                  .arg(pluginError(pluginObject)));
  if (pluginCanceled(pluginObject))
    return fail("Successful DICOM import retained cancellation state");

  int depth = 0;
  int width = 0;
  int height = 0;
  plugin->gridSize(depth, width, height);
  if (depth != 2 || width != 2 || height != 3 ||
      plugin->voxelType() != _Short || plugin->voxelUnit() != _Millimeter ||
      plugin->headerBytes() != 0 || plugin->rawMin() != -32768 ||
      plugin->rawMax() != 32767)
    return fail(QString("Unexpected DICOM contract: %1 x %2 x %3, type %4, "
                        "unit %5, range %6..%7")
                  .arg(depth).arg(width).arg(height).arg(plugin->voxelType())
                  .arg(plugin->voxelUnit()).arg(plugin->rawMin())
                  .arg(plugin->rawMax()));

  const QList<uint> histogram = plugin->histogram();
  quint64 histogramTotal = 0;
  for (uint count : histogram)
    histogramTotal += count;
  if (histogram.size() != 65536 || histogramTotal != 12 ||
      histogram.first() != 1 || histogram.last() != 1)
    return fail("DICOM signed-short histogram is incorrect");

  QVector<qint16> slice(6, 0);
  plugin->getDepthSlice(0, reinterpret_cast<uchar*>(slice.data()));
  if (slice != first || plugin->rawValue(1, 1, 2).toInt() != 32767)
    return fail("DICOM voxel order or rawValue lookup is incorrect");

  const QString cancellationDirectory = fixtures.filePath("cancel-scan");
  if (!QDir().mkpath(cancellationDirectory))
    return fail("Cannot create the DICOM cancellation fixture directory");
  for (int index=0; index<4096; ++index)
    {
      QFile dummy(QDir(cancellationDirectory).filePath(
        QString("not-dicom-%1.bin").arg(index, 4, 10, QChar('0'))));
      if (!dummy.open(QIODevice::WriteOnly) || dummy.write("x", 1) != 1)
        return fail("Cannot create DICOM cancellation scan fixtures");
    }

  bool sawCancelDialog = false;
  QTimer cancelTimer;
  QObject::connect(&cancelTimer, &QTimer::timeout, [&]()
  {
    for (QWidget *widget : QApplication::topLevelWidgets())
      if (QProgressDialog *progress = qobject_cast<QProgressDialog*>(widget))
        if (progress->isVisible())
          if (QAbstractButton *button = progress->findChild<QAbstractButton*>())
            {
              sawCancelDialog = true;
              button->click();
            }
  });
  cancelTimer.start(1);
  const bool canceledLoad =
    plugin->setFile(QStringList() << cancellationDirectory);
  cancelTimer.stop();
  if (canceledLoad || !sawCancelDialog || !pluginCanceled(pluginObject))
    return fail(QString("DICOM progress cancellation was not reported "
                        "correctly (loaded=%1, dialog=%2, canceled=%3, "
                        "error=%4)")
                  .arg(canceledLoad).arg(sawCancelDialog)
                  .arg(pluginCanceled(pluginObject))
                  .arg(pluginError(pluginObject)));

  plugin->gridSize(depth, width, height);
  slice.fill(0);
  plugin->getDepthSlice(0, reinterpret_cast<uchar*>(slice.data()));
  if (depth != 2 || width != 2 || height != 3 || slice != first)
    return fail("Canceling DICOM import damaged the active volume");

  const QString unsignedDirectory = fixtures.filePath("unsigned-series");
  if (!QDir().mkpath(unsignedDirectory))
    return fail("Cannot create the unsigned DICOM fixture directory");
  const QVector<quint16> unsignedFirst = {
    0, 1, 32767, 32768, 50000, 65535
  };
  const QVector<quint16> unsignedSecond = {
    100, 200, 300, 400, 500, 600
  };
  if (!writeDicomSlice(QDir(unsignedDirectory).filePath("slice-001"),
                       1, 0.0, unsignedFirst) ||
      !writeDicomSlice(QDir(unsignedDirectory).filePath("slice-002.dcm"),
                       2, 1.5, unsignedSecond))
    return fail("Cannot write unsigned DICOM fixtures");

  if (!plugin->setFile(QStringList() << unsignedDirectory))
    return fail(QString("Unsigned DICOM series was rejected: %1")
                  .arg(pluginError(pluginObject)));
  plugin->gridSize(depth, width, height);
  const QList<uint> unsignedHistogram = plugin->histogram();
  quint64 unsignedHistogramTotal = 0;
  for (uint count : unsignedHistogram)
    unsignedHistogramTotal += count;
  QVector<quint16> unsignedSlice(6, 0);
  plugin->getDepthSlice(0, reinterpret_cast<uchar*>(unsignedSlice.data()));
  if (depth != 2 || width != 2 || height != 3 ||
      plugin->voxelType() != _UShort || plugin->rawMin() != 0 ||
      plugin->rawMax() != 65535 || unsignedHistogram.size() != 65536 ||
      unsignedHistogramTotal != 12 || unsignedHistogram[0] != 1 ||
      unsignedHistogram[32768] != 1 || unsignedHistogram[65535] != 1 ||
      unsignedSlice != unsignedFirst ||
      plugin->rawValue(0, 1, 2).toUInt() != 65535)
    return fail("Unsigned DICOM values were narrowed or reordered");

  plugin->clear();
  plugin->gridSize(depth, width, height);
  if (depth != 0 || width != 0 || height != 0 || plugin->headerBytes() != 0)
    return fail("DICOM clear did not reset plugin state");

  std::cout << "DICOM plugin integration smoke passed" << std::endl;
  return 0;
}
