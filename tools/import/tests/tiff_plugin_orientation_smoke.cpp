#include "../common.h"
#include "../sourcefilesprovider.h"
#include "../volinterface.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPluginLoader>
#include <QTemporaryDir>
#include <QTimer>
#include <QVector>

#include <tiffio.h>

#include <cmath>
#include <iostream>

namespace
{
const uint32_t kWidth = 4;
const uint32_t kHeight = 3;
const int kStressSliceCount = 128;

TIFF *openTiffForWrite(const QString &fileName)
{
#if defined(Q_OS_WIN)
  return TIFFOpenW(reinterpret_cast<const wchar_t *>(fileName.utf16()), "w");
#else
  const QByteArray encodedName = QFile::encodeName(fileName);
  return TIFFOpen(encodedName.constData(), "w");
#endif
}

bool writeFixture(const QString &fileName, uint16_t orientation, quint16 base)
{
  TIFF *image = openTiffForWrite(fileName);
  if (!image)
    return false;

  const bool configured =
    TIFFSetField(image, TIFFTAG_IMAGEWIDTH, kWidth) == 1 &&
    TIFFSetField(image, TIFFTAG_IMAGELENGTH, kHeight) == 1 &&
    TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 16) == 1 &&
    TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1) == 1 &&
    TIFFSetField(image, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT) == 1 &&
    TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG) == 1 &&
    TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK) == 1 &&
    TIFFSetField(image, TIFFTAG_ORIENTATION, orientation) == 1 &&
    TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE) == 1 &&
    TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, kHeight) == 1;

  bool written = configured;
  quint16 row[kWidth];
  for (uint32_t y=0; written && y<kHeight; ++y)
    {
      for (uint32_t x=0; x<kWidth; ++x)
        row[x] = static_cast<quint16>(base + y*kWidth + x);
      written = TIFFWriteScanline(image, row, y, 0) >= 0;
    }

  TIFFClose(image);
  return written;
}

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

bool checkSlice(VolInterface *plugin, QObject *pluginObject,
                int sliceIndex, quint16 base, QString *error)
{
  QVector<quint16> values(static_cast<int>(kWidth*kHeight), 0);
  plugin->getDepthSlice(sliceIndex,
                        reinterpret_cast<uchar *>(values.data()));
  const QString decodeError = lastError(pluginObject);
  if (!decodeError.isEmpty())
    {
      *error = decodeError;
      return false;
    }

  for (uint32_t y=0; y<kHeight; ++y)
    for (uint32_t x=0; x<kWidth; ++x)
      {
        const int outputIndex = static_cast<int>(y*kWidth+x);
        const quint16 expected = static_cast<quint16>(base + y*kWidth+x);
        if (values[outputIndex] != expected)
          {
            *error = QString("slice %1 coordinate (%2,%3) was %4, expected %5")
                       .arg(sliceIndex).arg(x).arg(y)
                       .arg(values[outputIndex]).arg(expected);
            return false;
          }
      }
  return true;
}

bool checkOrthogonalSlices(VolInterface *plugin, QObject *pluginObject,
                           quint16 base0, quint16 base1, QString *error)
{
  QVector<quint16> widthValues(static_cast<int>(2*kWidth), 0);
  plugin->getWidthSlice(1, reinterpret_cast<uchar *>(widthValues.data()));
  QString decodeError = lastError(pluginObject);
  if (!decodeError.isEmpty())
    {
      *error = decodeError;
      return false;
    }
  for (int depth=0; depth<2; ++depth)
    for (uint32_t x=0; x<kWidth; ++x)
      {
        const quint16 base = depth == 0 ? base0 : base1;
        const quint16 expected = static_cast<quint16>(base+kWidth+x);
        if (widthValues[depth*kWidth+x] != expected)
          {
            *error = QString("width slice (%1,%2) was %3, expected %4")
                       .arg(depth).arg(x)
                       .arg(widthValues[depth*kWidth+x]).arg(expected);
            return false;
          }
      }

  QVector<quint16> heightValues(static_cast<int>(2*kHeight), 0);
  plugin->getHeightSlice(2, reinterpret_cast<uchar *>(heightValues.data()));
  decodeError = lastError(pluginObject);
  if (!decodeError.isEmpty())
    {
      *error = decodeError;
      return false;
    }
  for (int depth=0; depth<2; ++depth)
    for (uint32_t y=0; y<kHeight; ++y)
      {
        const quint16 base = depth == 0 ? base0 : base1;
        const quint16 expected = static_cast<quint16>(base+y*kWidth+2);
        if (heightValues[depth*kHeight+y] != expected)
          {
            *error = QString("height slice (%1,%2) was %3, expected %4")
                       .arg(depth).arg(y)
                       .arg(heightValues[depth*kHeight+y]).arg(expected);
            return false;
          }
      }
  return true;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc != 2)
    return fail("Usage: tiff_plugin_orientation_smoke <tiffplugin.dll>");

  const QFileInfo pluginFile(
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath());
  QDir runtimeDirectory = pluginFile.absoluteDir();
  if (!runtimeDirectory.cdUp())
    return fail("Cannot locate the TIFF plugin runtime directory");

  const QString platformDirectory = runtimeDirectory.filePath("platforms");
  const QString offscreenPlugin =
    QDir(platformDirectory).filePath("qoffscreen.dll");
  if (!QFileInfo::exists(offscreenPlugin))
    return fail(QString("Cannot locate the Qt offscreen platform plugin: %1")
                  .arg(QDir::toNativeSeparators(offscreenPlugin)));

  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
          QFile::encodeName(platformDirectory));
  QApplication application(argc, argv);

  QTemporaryDir directory;
  if (!directory.isValid())
    return fail("Cannot create the temporary TIFF fixture directory");

  const QString bottom0 = directory.filePath("bottom-left-000.tiff");
  const QString bottom1 = directory.filePath("bottom-left-001.tiff");
  const QString top = directory.filePath("top-left.tiff");
  const QString unsupported = directory.filePath("bottom-right.tiff");
  if (!writeFixture(bottom0, ORIENTATION_BOTLEFT, 1) ||
      !writeFixture(bottom1, ORIENTATION_BOTLEFT, 101) ||
      !writeFixture(top, ORIENTATION_TOPLEFT, 201) ||
      !writeFixture(unsupported, ORIENTATION_BOTRIGHT, 301))
    return fail("Cannot write the TIFF orientation fixtures");

  QPluginLoader loader(pluginFile.absoluteFilePath());
  QObject *pluginObject = loader.instance();
  if (!pluginObject)
    return fail(QString("Cannot load TIFF plugin: %1").arg(loader.errorString()));

  VolInterface *plugin = qobject_cast<VolInterface *>(pluginObject);
  if (!plugin)
    return fail("Loaded object does not implement VolInterface");
  plugin->init();

  bool nestedLoadAttempted = false;
  bool nestedLoadAccepted = true;
  QString nestedLoadError;
  QTimer::singleShot(0, [&]()
    {
      nestedLoadAttempted = true;
      nestedLoadAccepted = plugin->setFile(QStringList() << top);
      nestedLoadError = lastError(pluginObject);
    });
  if (!plugin->setFile(QStringList() << bottom0 << bottom1))
    return fail(QString("Bottom-left TIFF stack was rejected: %1")
                  .arg(lastError(pluginObject)));
  if (!nestedLoadAttempted || nestedLoadAccepted ||
      !nestedLoadError.contains("already being loaded", Qt::CaseInsensitive))
    return fail("A nested TIFF volume load was not rejected safely");
  if (!lastError(pluginObject).isEmpty())
    return fail("A rejected nested TIFF load contaminated the outer result");

  int depth = 0;
  int width = 0;
  int height = 0;
  plugin->gridSize(depth, width, height);
  if (depth != 2 || width != static_cast<int>(kHeight) ||
      height != static_cast<int>(kWidth) || plugin->voxelType() != _UShort)
    return fail(QString("Unexpected bottom-left stack contract: %1 x %2 x %3, type %4")
                  .arg(depth).arg(width).arg(height).arg(plugin->voxelType()));

  const QList<uint> histogram = plugin->histogram();
  if (histogram.size() != 65536 || histogram[1] != 1 ||
      histogram[12] != 1 || histogram[101] != 1 || histogram[112] != 1 ||
      std::fabs(plugin->rawMin()-1.0f) > 0.01f ||
      std::fabs(plugin->rawMax()-112.0f) > 0.01f)
    return fail("Bottom-left TIFF statistics are incorrect");

  QVector<quint16> outerValues(static_cast<int>(kWidth*kHeight), 0);
  QVector<quint16> nestedValues(static_cast<int>(kWidth*kHeight), 0xffff);
  bool nestedAttempted = false;
  QString nestedError;
  QTimer::singleShot(0, [&]()
    {
      nestedAttempted = true;
      plugin->getDepthSlice(0,
                            reinterpret_cast<uchar *>(nestedValues.data()));
      nestedError = lastError(pluginObject);
    });
  plugin->getDepthSlice(0, reinterpret_cast<uchar *>(outerValues.data()));
  if (!nestedAttempted ||
      !nestedError.contains("already being loaded", Qt::CaseInsensitive))
    return fail("A nested TIFF preview was not rejected safely");
  if (!lastError(pluginObject).isEmpty())
    return fail("A rejected nested preview contaminated the outer result");
  for (uint32_t y=0; y<kHeight; ++y)
    for (uint32_t x=0; x<kWidth; ++x)
      {
        const int index = static_cast<int>(y*kWidth+x);
        const quint16 expected = static_cast<quint16>(1+y*kWidth+x);
        if (outerValues[index] != expected || nestedValues[index] != 0)
          return fail("Nested preview protection damaged a TIFF slice buffer");
      }

  QString sliceError;
  if (!checkSlice(plugin, pluginObject, 0, 1, &sliceError) ||
      !checkSlice(plugin, pluginObject, 1, 101, &sliceError))
    return fail(QString("Bottom-left scanline order changed: %1").arg(sliceError));
  if (!checkOrthogonalSlices(plugin, pluginObject, 1, 101, &sliceError))
    return fail(QString("Bottom-left orthogonal slices are incorrect: %1")
                  .arg(sliceError));

  const QVariant firstRawValue = plugin->rawValue(0, 0, 0);
  const QVariant lastRawValue = plugin->rawValue(0, 2, 3);
  if (firstRawValue.toUInt() != 1 || lastRawValue.toUInt() != 12)
    return fail(QString(
      "Bottom-left rawValue coordinates do not preserve storage order: "
      "first=%1, last=%2").arg(firstRawValue.toString(),
                                lastRawValue.toString()));

  if (plugin->setFile(QStringList() << bottom0 << top) ||
      lastError(pluginObject).isEmpty())
    return fail("A mixed top-left/bottom-left TIFF stack was accepted");

  // A rejected candidate must not damage the previously active stack.
  if (!checkSlice(plugin, pluginObject, 0, 1, &sliceError) ||
      !checkSlice(plugin, pluginObject, 1, 101, &sliceError))
    return fail(QString("Rejected mixed orientation damaged the active stack: %1")
                  .arg(sliceError));

  if (plugin->setFile(QStringList() << unsupported) ||
      lastError(pluginObject).isEmpty())
    return fail("An unsupported bottom-right TIFF orientation was accepted");
  if (!checkSlice(plugin, pluginObject, 0, 1, &sliceError) ||
      !checkSlice(plugin, pluginObject, 1, 101, &sliceError))
    return fail(QString("Rejected TIFF orientation damaged the active stack: %1")
                  .arg(sliceError));

  plugin->clear();
  if (!plugin->setFile(QStringList() << top))
    return fail("Top-left TIFF regression fixture was rejected");
  if (!checkSlice(plugin, pluginObject, 0, 201, &sliceError))
    return fail(QString("Top-left TIFF regression failed: %1").arg(sliceError));

  plugin->replaceFile(directory.filePath("missing-replacement.tiff"));
  if (lastError(pluginObject).isEmpty())
    return fail("A failed TIFF replacement did not propagate lastError()");
  if (!checkSlice(plugin, pluginObject, 0, 201, &sliceError))
    return fail(QString("A failed TIFF replacement corrupted the active stack: %1")
                  .arg(sliceError));

  const QString naturalDirectory = directory.filePath("natural-order");
  if (!QDir().mkpath(naturalDirectory) ||
      !writeFixture(QDir(naturalDirectory).filePath("slice-10.tiff"),
                    ORIENTATION_TOPLEFT, 510) ||
      !writeFixture(QDir(naturalDirectory).filePath("slice-2.tiff"),
                    ORIENTATION_TOPLEFT, 502) ||
      !writeFixture(QDir(naturalDirectory).filePath("slice-1.tiff"),
                    ORIENTATION_TOPLEFT, 501))
    return fail("Cannot write the natural-order TIFF fixtures");

  plugin->clear();
  if (!plugin->setFile(QStringList() << naturalDirectory))
    return fail("Natural-order TIFF directory was rejected");
  plugin->gridSize(depth, width, height);
  if (depth != 3 ||
      !checkSlice(plugin, pluginObject, 0, 501, &sliceError) ||
      !checkSlice(plugin, pluginObject, 1, 502, &sliceError) ||
      !checkSlice(plugin, pluginObject, 2, 510, &sliceError))
    return fail(QString("TIFF directory numeric ordering is incorrect: %1")
                  .arg(sliceError));

  const QStringList explicitOrder =
    QStringList() << QDir(naturalDirectory).filePath("slice-10.tiff")
                  << QDir(naturalDirectory).filePath("slice-1.tiff")
                  << QDir(naturalDirectory).filePath("slice-2.tiff");
  plugin->clear();
  if (!plugin->setFile(explicitOrder) ||
      !checkSlice(plugin, pluginObject, 0, 510, &sliceError) ||
      !checkSlice(plugin, pluginObject, 1, 501, &sliceError) ||
      !checkSlice(plugin, pluginObject, 2, 502, &sliceError))
    return fail(QString("TIFF explicit order was not preserved: %1")
                  .arg(sliceError));
  SourceFilesProvider *sourceProvider =
    qobject_cast<SourceFilesProvider *>(pluginObject);
  if (!sourceProvider || sourceProvider->sourceFiles() != explicitOrder)
    return fail("TIFF source provenance did not preserve explicit order");

  // Exercise the GUI-parent helper contract at a size large enough to catch
  // per-slice process/window regressions.  The plugin passes a Windows-only
  // helper assertion that rejects any child which still owns a console.
  QStringList stressFiles;
  stressFiles.reserve(kStressSliceCount);
  for (int index=0; index<kStressSliceCount; ++index)
    stressFiles.append(top);
  plugin->clear();
  if (!plugin->setFile(stressFiles))
    return fail(QString("Large TIFF helper stress stack was rejected: %1")
                  .arg(lastError(pluginObject)));
  plugin->gridSize(depth, width, height);
  if (depth != kStressSliceCount ||
      !checkSlice(plugin, pluginObject, kStressSliceCount-1, 201, &sliceError))
    return fail(QString("Large TIFF helper stress stack was corrupted: %1")
                  .arg(sliceError));

  plugin->clear();
  std::cout << "TIFF top-left/bottom-left orientation smoke passed" << std::endl;
  return 0;
}
