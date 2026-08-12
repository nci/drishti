#include "../common.h"
#include "../volinterface.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QImageReader>
#include <QInputDialog>
#include <QMessageBox>
#include <QPluginLoader>
#include <QTemporaryDir>
#include <QTimer>
#include <QVector>

#include <cstring>
#include <iostream>

namespace
{
int fail(const QString &message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

QString lastError(QObject *pluginObject)
{
  QString error;
  QMetaObject::invokeMethod(pluginObject, "lastError", Qt::DirectConnection,
                            Q_RETURN_ARG(QString, error));
  return error;
}

bool writeFixture(const QString &fileName, int base)
{
  QImage image(3, 2, QImage::Format_ARGB32);
  for (int y=0; y<image.height(); ++y)
    for (int x=0; x<image.width(); ++x)
      {
        const int value = base+y*image.width()+x;
        image.setPixel(x, y, qRgb(value, value, value));
      }
  return image.save(fileName, "PNG");
}

bool writeFixture16(const QString &fileName, quint16 base)
{
  QImage image(3, 2, QImage::Format_Grayscale16);
  if (image.isNull())
    return false;
  for (int y=0; y<image.height(); ++y)
    {
      quint16 *scanline = reinterpret_cast<quint16*>(image.scanLine(y));
      if (!scanline)
        return false;
      for (int x=0; x<image.width(); ++x)
        scanline[x] = static_cast<quint16>(base+y*image.width()+x);
    }
  return image.save(fileName, "PNG");
}

bool writeCorruptFixture(const QString &fileName)
{
  if (!writeFixture(fileName, 50))
    return false;

  QFile file(fileName);
  if (!file.open(QIODevice::ReadWrite))
    return false;
  QByteArray corruptBytes = file.readAll();
  const int idatType = corruptBytes.indexOf("IDAT");
  if (idatType < 4 || idatType+8 > corruptBytes.size())
    return false;

  const uchar *lengthBytes = reinterpret_cast<const uchar *>(
    corruptBytes.constData()+idatType-4);
  const quint32 idatLength = (static_cast<quint32>(lengthBytes[0]) << 24) |
                             (static_cast<quint32>(lengthBytes[1]) << 16) |
                             (static_cast<quint32>(lengthBytes[2]) << 8) |
                              static_cast<quint32>(lengthBytes[3]);
  if (idatLength == 0 ||
      static_cast<quint64>(idatType)+8+idatLength >
        static_cast<quint64>(corruptBytes.size()))
    return false;

  const int corruptOffset = idatType+4+static_cast<int>(idatLength/2);
  corruptBytes[corruptOffset] = corruptBytes[corruptOffset] ^ 0x01;
  if (!file.seek(0) || file.write(corruptBytes) != corruptBytes.size() ||
      !file.resize(corruptBytes.size()))
    return false;
  file.close();

  QImageReader reader(fileName);
  const QSize declaredSize = reader.size();
  const QImage decodedImage = reader.read();
  return declaredSize == QSize(3, 2) && decodedImage.isNull();
}

bool checkSliceAt(VolInterface *plugin, QObject *pluginObject,
                  int sliceIndex, int base, QString *error)
{
  QVector<uchar> values(6, 0);
  plugin->getDepthSlice(sliceIndex, values.data());
  const QString decodeError = lastError(pluginObject);
  if (!decodeError.isEmpty())
    {
      *error = decodeError;
      return false;
    }
  for (int index=0; index<values.size(); ++index)
    if (values[index] != static_cast<uchar>(base+index))
      {
        *error = QString("pixel %1 was %2, expected %3")
          .arg(index).arg(values[index]).arg(base+index);
        return false;
      }
  return true;
}

bool checkSlice(VolInterface *plugin, QObject *pluginObject,
                int base, QString *error)
{
  return checkSliceAt(plugin, pluginObject, 0, base, error);
}

bool checkSlice16(VolInterface *plugin, QObject *pluginObject,
                  int sliceIndex, quint16 base, QString *error)
{
  QVector<quint16> values(6, 0);
  plugin->getDepthSlice(sliceIndex,
                        reinterpret_cast<uchar*>(values.data()));
  const QString decodeError = lastError(pluginObject);
  if (!decodeError.isEmpty())
    {
      *error = decodeError;
      return false;
    }
  for (int index=0; index<values.size(); ++index)
    if (values[index] != static_cast<quint16>(base+index))
      {
        *error = QString("16-bit pixel %1 was %2, expected %3")
          .arg(index).arg(values[index]).arg(base+index);
        return false;
      }
  return true;
}

bool verifyTrimmed16(const QString &fileName, QString *error)
{
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly))
    {
      *error = "cannot open the trimmed 16-bit output";
      return false;
    }
  const QByteArray bytes = file.readAll();
  const int expectedBytes = 13 + 2*2*2*static_cast<int>(sizeof(quint16));
  if (bytes.size() != expectedBytes ||
      static_cast<uchar>(bytes[0]) != static_cast<uchar>(_UShort))
    {
      *error = QString("trimmed header/type or file size is wrong (%1 bytes, type %2)")
        .arg(bytes.size()).arg(bytes.isEmpty() ? -1 :
                               static_cast<int>(static_cast<uchar>(bytes[0])));
      return false;
    }

  qint32 depth = 0;
  qint32 width = 0;
  qint32 height = 0;
  std::memcpy(&depth, bytes.constData()+1, sizeof(depth));
  std::memcpy(&width, bytes.constData()+5, sizeof(width));
  std::memcpy(&height, bytes.constData()+9, sizeof(height));
  if (depth != 2 || width != 2 || height != 2)
    {
      *error = QString("trimmed dimensions are %1 x %2 x %3")
        .arg(depth).arg(width).arg(height);
      return false;
    }

  const QVector<quint16> expected = {
    2001, 2002, 2004, 2005,
    1001, 1002, 1004, 1005
  };
  QVector<quint16> actual(expected.size(), 0);
  std::memcpy(actual.data(), bytes.constData()+13,
              static_cast<std::size_t>(expected.size())*sizeof(quint16));
  if (actual != expected)
    {
      *error = "trimmed 16-bit pixels were changed or cropped incorrectly";
      return false;
    }
  return true;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc != 2)
    return fail("Usage: imagestack_plugin_smoke <imagestackplugin.dll>");

  const QFileInfo pluginFile(
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath());
  QDir runtimeDirectory = pluginFile.absoluteDir();
  if (!runtimeDirectory.cdUp())
    return fail("Cannot locate the ImageStack plugin runtime directory");
  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
          QFile::encodeName(runtimeDirectory.filePath("platforms")));

  QApplication application(argc, argv);
  std::cerr << "checkpoint: application" << std::endl;
  QTemporaryDir directory;
  if (!directory.isValid())
    return fail("Cannot create the temporary image fixture directory");

  const QString first = directory.filePath("first.png");
  const QString second = directory.filePath("second.png");
  const QString corrupt = directory.filePath("corrupt.png");
  const QString first16 = directory.filePath("first-16.png");
  const QString second16 = directory.filePath("second-16.png");
  if (!writeFixture(first, 10) || !writeFixture(second, 30) ||
      !writeFixture16(first16, 1000) ||
      !writeFixture16(second16, 2000) ||
      !writeCorruptFixture(corrupt))
    return fail("Cannot write image fixtures");
  std::cerr << "checkpoint: fixtures" << std::endl;

  QPluginLoader loader(pluginFile.absoluteFilePath());
  QObject *pluginObject = loader.instance();
  if (!pluginObject)
    return fail(QString("Cannot load ImageStack plugin: %1")
                  .arg(loader.errorString()));
  VolInterface *plugin = qobject_cast<VolInterface *>(pluginObject);
  if (!plugin)
    return fail("Loaded object does not implement VolInterface");
  plugin->init();
  std::cerr << "checkpoint: plugin" << std::endl;

  QTimer dialogTimer;
  QObject::connect(&dialogTimer, &QTimer::timeout, []()
  {
    for (QWidget *widget : QApplication::topLevelWidgets())
      {
        if (QInputDialog *input = qobject_cast<QInputDialog *>(widget))
          input->accept();
        else if (QMessageBox *message = qobject_cast<QMessageBox *>(widget))
          message->done(QMessageBox::Ok);
      }
  });
  dialogTimer.start(10);

  if (!plugin->setFile(QStringList() << first))
    return fail(QString("Initial image load failed: %1").arg(lastError(pluginObject)));
  std::cerr << "checkpoint: initial load" << std::endl;

  int depth = 0;
  int width = 0;
  int height = 0;
  plugin->gridSize(depth, width, height);
  if (depth != 1 || width != 2 || height != 3 ||
      plugin->voxelType() != _UChar)
    return fail("Initial image-stack contract is incorrect");
  const QList<uint> initialHistogram = plugin->histogram();
  uint initialHistogramTotal = 0;
  for (uint count : initialHistogram) initialHistogramTotal += count;
  if (initialHistogram.size() != 256 || initialHistogramTotal != 6 ||
      plugin->rawMin() != 10 || plugin->rawMax() != 15 ||
      initialHistogram[10] != 1 || initialHistogram[15] != 1)
    return fail("Initial image-stack histogram is incorrect");

  QString error;
  if (!checkSlice(plugin, pluginObject, 10, &error))
    return fail(QString("Initial image slice is incorrect: %1").arg(error));
  std::cerr << "checkpoint: initial slice" << std::endl;

  if (plugin->setFile(QStringList() << directory.filePath("missing.png")))
    return fail("A missing image was accepted");
  std::cerr << "checkpoint: failed setFile" << std::endl;
  if (lastError(pluginObject).isEmpty())
    return fail("A failed image load did not expose lastError()");
  plugin->gridSize(depth, width, height);
  if (depth != 1 || width != 2 || height != 3 ||
      !checkSlice(plugin, pluginObject, 10, &error))
    return fail(QString("A failed setFile corrupted the active stack: %1").arg(error));
  std::cerr << "checkpoint: rollback slice" << std::endl;

  if (plugin->setFile(QStringList() << corrupt))
    return fail("An image with a valid header and corrupt payload was accepted");
  if (lastError(pluginObject).isEmpty() ||
      !checkSlice(plugin, pluginObject, 10, &error))
    return fail(QString("A corrupt payload damaged the active stack: %1")
                  .arg(error));
  std::cerr << "checkpoint: corrupt payload rollback" << std::endl;

  plugin->replaceFile(directory.filePath("missing-replacement.png"));
  if (lastError(pluginObject).isEmpty() ||
      !checkSlice(plugin, pluginObject, 10, &error))
    return fail(QString("A failed replacement corrupted the active stack: %1")
                  .arg(error));
  std::cerr << "checkpoint: failed replacement" << std::endl;

  plugin->replaceFile(corrupt);
  if (lastError(pluginObject).isEmpty() ||
      !checkSlice(plugin, pluginObject, 10, &error))
    return fail(QString("A corrupt replacement damaged the active stack: %1")
                  .arg(error));
  std::cerr << "checkpoint: corrupt replacement rollback" << std::endl;

  plugin->replaceFile(second);
  if (!lastError(pluginObject).isEmpty() ||
      !checkSlice(plugin, pluginObject, 30, &error))
    return fail(QString("A valid replacement failed: %1").arg(error));
  std::cerr << "checkpoint: valid replacement" << std::endl;

  const QString naturalDirectory = directory.filePath("natural-order");
  if (!QDir().mkpath(naturalDirectory) ||
      !writeFixture(QDir(naturalDirectory).filePath("slice-10.png"), 100) ||
      !writeFixture(QDir(naturalDirectory).filePath("slice-2.png"), 20) ||
      !writeFixture(QDir(naturalDirectory).filePath("slice-1.png"), 10))
    return fail("Cannot create natural-order image fixtures");

  plugin->clear();
  if (!plugin->setFile(QStringList() << naturalDirectory))
    return fail(QString("Natural-order image directory failed: %1")
                  .arg(lastError(pluginObject)));
  plugin->gridSize(depth, width, height);
  if (depth != 3 || width != 2 || height != 3 ||
      !checkSliceAt(plugin, pluginObject, 0, 10, &error) ||
      !checkSliceAt(plugin, pluginObject, 1, 20, &error) ||
      !checkSliceAt(plugin, pluginObject, 2, 100, &error))
    return fail(QString("Image directory numeric ordering is incorrect: %1")
                  .arg(error));
  std::cerr << "checkpoint: natural ordering" << std::endl;

  plugin->clear();
  if (!plugin->setFile(QStringList() << first16 << second16))
    return fail(QString("16-bit PNG stack failed: %1")
                  .arg(lastError(pluginObject)));
  plugin->gridSize(depth, width, height);
  const QList<uint> histogram16 = plugin->histogram();
  uint histogram16Total = 0;
  for (uint count : histogram16) histogram16Total += count;
  if (depth != 2 || width != 2 || height != 3 ||
      plugin->voxelType() != _UShort || plugin->rawMin() != 1000 ||
      plugin->rawMax() != 2005 || histogram16.size() != 65536 ||
      histogram16Total != 12 || histogram16[1000] != 1 ||
      histogram16[2005] != 1 ||
      !checkSlice16(plugin, pluginObject, 0, 1000, &error) ||
      !checkSlice16(plugin, pluginObject, 1, 2000, &error) ||
      plugin->rawValue(1, 1, 2).toUInt() != 2005)
    return fail(QString("16-bit PNG values were not preserved: %1").arg(error));

  plugin->replaceFile(first);
  if (lastError(pluginObject).isEmpty() ||
      !checkSlice16(plugin, pluginObject, 0, 1000, &error))
    return fail(QString("An 8-bit replacement damaged the 16-bit stack: %1")
                  .arg(error));

  const QString trimmed16 = directory.filePath("trimmed-16.raw");
  plugin->saveTrimmed(trimmed16, 0, 1, 0, 1, 1, 2);
  if (!verifyTrimmed16(trimmed16, &error))
    return fail(QString("16-bit trimmed export failed: %1").arg(error));
  std::cerr << "checkpoint: 16-bit stack and trimmed export" << std::endl;

  dialogTimer.stop();
  plugin->clear();
  std::cout << "ImageStack transactional plugin smoke passed" << std::endl;
  return 0;
}
