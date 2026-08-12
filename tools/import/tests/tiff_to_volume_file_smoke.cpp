#include "../common.h"
#include "../volinterface.h"
#include "../volumefilemanager.h"

#include <QApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPluginLoader>
#include <QTemporaryDir>
#include <QTextStream>
#include <QVector>

#include <limits>

namespace
{
int fail(const QString& message)
{
  QTextStream(stderr) << "FAILED: " << message << Qt::endl;
  return 1;
}

QString pluginLastError(QObject *object)
{
  QString error;
  QMetaObject::invokeMethod(object, "lastError", Qt::DirectConnection,
                            Q_RETURN_ARG(QString, error));
  return error;
}

bool addFileToHash(const QString& path, QCryptographicHash& hash)
{
  QFile file(path);
  if (!file.open(QFile::ReadOnly))
    return false;

  QByteArray buffer(1024*1024, '\0');
  while (!file.atEnd())
    {
      const qint64 bytes = file.read(buffer.data(), buffer.size());
      if (bytes <= 0)
        return false;
      hash.addData(buffer.constData(), bytes);
    }
  return file.error() == QFileDevice::NoError;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc != 5)
    return fail("Usage: tiff_to_volume_file_smoke <tiffplugin.dll> "
                "<image-directory> <expected-stack-slices> <saved-slices>");

  bool validExpected = false;
  bool validSaved = false;
  const int expectedSlices =
    QString::fromLocal8Bit(argv[3]).toInt(&validExpected);
  const int savedSlices = QString::fromLocal8Bit(argv[4]).toInt(&validSaved);
  if (!validExpected || !validSaved || expectedSlices <= 0 ||
      savedSlices <= 0 || savedSlices > expectedSlices)
    return fail("The requested slice counts are invalid");

  const QString pluginPath =
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
  QDir runtimeDirectory = QFileInfo(pluginPath).absoluteDir();
  if (!runtimeDirectory.cdUp())
    return fail("Cannot locate the TIFF plugin runtime directory");
  const QString platformDirectory = runtimeDirectory.filePath("platforms");
  if (!QFileInfo::exists(QDir(platformDirectory).filePath("qoffscreen.dll")))
    return fail("The Qt offscreen platform plugin is missing");
  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
          QFile::encodeName(platformDirectory));

  QApplication application(argc, argv);
  QPluginLoader loader(pluginPath);
  QObject *pluginObject = loader.instance();
  if (!pluginObject)
    return fail("Cannot load TIFF plugin: "+loader.errorString());
  VolInterface *plugin = qobject_cast<VolInterface *>(pluginObject);
  if (!plugin)
    return fail("The loaded object does not implement VolInterface");
  plugin->init();

  const QString imageDirectory =
    QFileInfo(QString::fromLocal8Bit(argv[2])).absoluteFilePath();
  if (!plugin->setFile(QStringList() << imageDirectory))
    return fail("Cannot load TIFF stack: "+pluginLastError(pluginObject));

  int depth = 0;
  int width = 0;
  int height = 0;
  plugin->gridSize(depth, width, height);
  if (depth != expectedSlices || width <= 0 || height <= 0 ||
      plugin->voxelType() != _UShort)
    return fail(QString("Unexpected TIFF contract: %1 x %2 x %3, type %4")
                  .arg(depth).arg(width).arg(height).arg(plugin->voxelType()));

  const quint64 sliceBytes64 =
    static_cast<quint64>(width)*height*sizeof(quint16);
  if (sliceBytes64 == 0 ||
      sliceBytes64 > static_cast<quint64>(std::numeric_limits<int>::max()))
    return fail("The TIFF slice is outside the test's supported size");
  const qint64 sliceBytes = static_cast<qint64>(sliceBytes64);

  QTemporaryDir temporary;
  if (!temporary.isValid())
    return fail("Cannot create the volume transaction directory");
  const QString unicodeDirectory =
    QDir(temporary.path()).filePath(QString::fromUtf8("PVL \xE4\xB8\xAD\xE6\x96\x87 path"));
  if (!QDir().mkpath(unicodeDirectory))
    return fail("Cannot create the Unicode output directory");
  const QString base = QDir(unicodeDirectory).filePath("living ant.raw");

  const int slabSize = 37;
  VolumeFileManager manager;
  manager.setBaseFilename(base);
  manager.setHeaderSize(0);
  manager.setSlabSize(slabSize);
  manager.setVoxelType(VolumeFileManager::_UShort);
  manager.setDepth(savedSlices);
  manager.setWidth(width);
  manager.setHeight(height);
  if (!manager.createFile(false))
    return fail("Cannot create volume files: "+manager.lastError());

  QVector<quint16> slice(width*height, 0);
  QCryptographicHash decodedHash(QCryptographicHash::Sha256);
  for (int index=0; index<savedSlices; ++index)
    {
      plugin->getDepthSlice(index,
                            reinterpret_cast<uchar *>(slice.data()));
      const QString decodeError = pluginLastError(pluginObject);
      if (!decodeError.isEmpty())
        return fail(QString("Cannot decode TIFF slice %1: %2")
                      .arg(index).arg(decodeError));
      decodedHash.addData(reinterpret_cast<const char *>(slice.constData()),
                          sliceBytes);
      if (!manager.setSlice(index,
                            reinterpret_cast<uchar *>(slice.data())))
        return fail(QString("Cannot persist slice %1: %2")
                      .arg(index).arg(manager.lastError()));
    }

  if (!manager.commitFileCreation())
    return fail("Cannot commit volume files: "+manager.lastError());

  const int slabCount = 1+(savedSlices-1)/slabSize;
  QCryptographicHash persistedHash(QCryptographicHash::Sha256);
  for (int slab=0; slab<slabCount; ++slab)
    {
      const QString path = base+QString(".%1").arg(slab+1, 3, 10, QChar('0'));
      const int slicesInSlab = qMin(slabSize, savedSlices-slab*slabSize);
      const qint64 expectedBytes = slicesInSlab*sliceBytes;
      if (QFileInfo(path).size() != expectedBytes)
        return fail(QString("Slab %1 has %2 bytes, expected %3")
                      .arg(path).arg(QFileInfo(path).size()).arg(expectedBytes));
      if (!addFileToHash(path, persistedHash))
        return fail("Cannot hash persisted slab "+path);
    }

  QDir outputDirectory(unicodeDirectory);
  const QStringList leftovers = outputDirectory.entryList(
    QStringList() << "*.drishti-part-*" << "*.drishti-backup-*",
    QDir::Files);
  if (!leftovers.isEmpty())
    return fail("Committed transaction left temporary files: "+
                leftovers.join(", "));
  if (decodedHash.result() != persistedHash.result())
    return fail("Persisted slab payload differs from decoded TIFF data");

  QTextStream(stdout)
    << "TIFF-to-volume-file smoke passed"
    << " stack_slices=" << depth
    << " saved_slices=" << savedSlices
    << " dimensions=" << width << 'x' << height
    << " slabs=" << slabCount
    << " sha256=" << persistedHash.result().toHex()
    << Qt::endl;
  plugin->clear();
  return 0;
}
