#include "../common.h"
#include "../metaimagepathutils.h"
#include "../volinterface.h"

#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QInputDialog>
#include <QMessageBox>
#include <QPluginLoader>
#include <QProgressDialog>
#include <QTemporaryDir>
#include <QTimer>
#include <QtEndian>

#include <netcdf.h>

#include <cmath>
#include <cstring>
#include <iostream>

namespace
{
QString g_failure;

bool check(bool condition, const QString& message)
{
  if (condition)
    return true;
  g_failure = message;
  return false;
}

bool writeBytes(const QString& fileName, const QByteArray& bytes)
{
  QFile file(fileName);
  return file.open(QFile::WriteOnly) && file.write(bytes) == bytes.size();
}

template<typename T>
void putInteger(QByteArray& bytes, int offset, T value, bool bigEndian = false)
{
  if (bigEndian)
    qToBigEndian<T>(value, reinterpret_cast<uchar*>(bytes.data()+offset));
  else
    qToLittleEndian<T>(value, reinterpret_cast<uchar*>(bytes.data()+offset));
}

void putFloat(QByteArray& bytes, int offset, float value,
              bool bigEndian = false)
{
  quint32 bits = 0;
  static_assert(sizeof(bits) == sizeof(value), "Unexpected float size");
  std::memcpy(&bits, &value, sizeof(bits));
  putInteger(bytes, offset, bits, bigEndian);
}

quint64 histogramTotal(const QList<uint>& histogram)
{
  quint64 total = 0;
  for (uint count : histogram)
    total += count;
  return total;
}

bool decodedSliceValue(const QByteArray& slice, int voxelType,
                       int width, int height, int w, int h, double& value)
{
  int bytesPerVoxel = 0;
  if (voxelType == _UChar || voxelType == _Char)
    bytesPerVoxel = 1;
  else if (voxelType == _UShort || voxelType == _Short)
    bytesPerVoxel = 2;
  else if (voxelType == _Int || voxelType == _Float)
    bytesPerVoxel = 4;
  if (bytesPerVoxel == 0 || w < 0 || w >= width || h < 0 || h >= height)
    return false;

  const qint64 offset =
    (static_cast<qint64>(w)*height+h)*bytesPerVoxel;
  if (offset < 0 || offset+bytesPerVoxel > slice.size())
    return false;
  const char *source = slice.constData()+offset;

  if (voxelType == _UChar)
    value = static_cast<quint8>(*source);
  else if (voxelType == _Char)
    value = static_cast<qint8>(*source);
  else if (voxelType == _UShort)
    {
      quint16 decoded = 0;
      std::memcpy(&decoded, source, sizeof(decoded));
      value = decoded;
    }
  else if (voxelType == _Short)
    {
      qint16 decoded = 0;
      std::memcpy(&decoded, source, sizeof(decoded));
      value = decoded;
    }
  else if (voxelType == _Int)
    {
      qint32 decoded = 0;
      std::memcpy(&decoded, source, sizeof(decoded));
      value = decoded;
    }
  else
    {
      float decoded = 0.0f;
      std::memcpy(&decoded, source, sizeof(decoded));
      value = decoded;
    }
  return true;
}

QString lastError(QObject *object)
{
  QString error;
  if (!QMetaObject::invokeMethod(object, "lastError", Qt::DirectConnection,
                                 Q_RETURN_ARG(QString, error)))
    return "<lastError unavailable>";
  return error;
}

class DialogResponder : public QObject
{
public:
  explicit DialogResponder(QObject *parent = nullptr) : QObject(parent)
  {
    connect(&m_timer, &QTimer::timeout, this, [this]() { respond(); });
    m_timer.start(2);
  }

  void expect(const QString& title)
  {
    m_expectedTitle = title;
    m_expectedSeen = false;
    m_unexpected.clear();
  }

  bool expectedSeen() const { return m_expectedSeen; }
  QString unexpected() const { return m_unexpected; }

private:
  void respond()
  {
    for (QWidget *widget : QApplication::topLevelWidgets())
      {
        if (!widget->isVisible() || qobject_cast<QProgressDialog*>(widget))
          continue;
        QDialog *dialog = qobject_cast<QDialog*>(widget);
        if (!dialog)
          continue;
        if (!m_expectedTitle.isEmpty() &&
            dialog->windowTitle() == m_expectedTitle)
          {
            m_expectedSeen = true;
            dialog->accept();
          }
        else if (qobject_cast<QMessageBox*>(dialog) ||
                 qobject_cast<QInputDialog*>(dialog))
          {
            m_unexpected = dialog->windowTitle();
            dialog->reject();
          }
      }
  }

  QTimer m_timer;
  QString m_expectedTitle;
  bool m_expectedSeen = false;
  QString m_unexpected;
};

struct LoadedPlugin
{
  QPluginLoader loader;
  QObject *object = nullptr;
  VolInterface *volume = nullptr;

  explicit LoadedPlugin(const QString& fileName) : loader(fileName)
  {
    object = loader.instance();
    volume = qobject_cast<VolInterface*>(object);
    if (volume)
      volume->init();
  }

  ~LoadedPlugin()
  {
    if (volume)
      {
        volume->clear();
        delete volume;
      }
    loader.unload();
  }
};

bool verifyVolume(LoadedPlugin& plugin, const QStringList& files,
                  int depth, int width, int height, int voxelType,
                  double expectedValue, int valueD, int valueW, int valueH,
                  qint64 sliceBytes, DialogResponder& dialogs,
                  const QString& expectedDialog = QString())
{
  if (!check(plugin.volume != nullptr,
             QString("Cannot load plugin: %1").arg(plugin.loader.errorString())))
    return false;
  dialogs.expect(expectedDialog);
  const bool accepted = plugin.volume->setFile(files);
  const QString importError = lastError(plugin.object);
  if (!check(accepted,
             QString("setFile failed: %1").arg(importError)))
    return false;
  if (!check(dialogs.unexpected().isEmpty(),
             QString("Unexpected modal dialog: %1").arg(dialogs.unexpected())))
    return false;
  if (!expectedDialog.isEmpty() &&
      !check(dialogs.expectedSeen(),
             QString("Expected dialog was not shown: %1").arg(expectedDialog)))
    return false;

  int actualDepth = 0, actualWidth = 0, actualHeight = 0;
  plugin.volume->gridSize(actualDepth, actualWidth, actualHeight);
  if (!check(actualDepth == depth && actualWidth == width &&
             actualHeight == height,
             QString("Unexpected grid %1 x %2 x %3")
               .arg(actualDepth).arg(actualWidth).arg(actualHeight)) ||
      !check(plugin.volume->voxelType() == voxelType,
             QString("Unexpected voxel type %1").arg(plugin.volume->voxelType())) ||
      !check(histogramTotal(plugin.volume->histogram()) ==
               static_cast<quint64>(depth)*width*height,
             "Histogram voxel total is incorrect"))
    return false;

  const QVariant value = plugin.volume->rawValue(valueD, valueW, valueH);
  if (!check(value.canConvert<double>() &&
             std::fabs(value.toDouble()-expectedValue) < 0.0001,
             QString("Unexpected raw value %1, expected %2")
               .arg(value.toString()).arg(expectedValue)))
    return false;

  QByteArray invalidSlice(static_cast<int>(sliceBytes), char(0x5a));
  plugin.volume->getDepthSlice(-1,
    reinterpret_cast<uchar*>(invalidSlice.data()));
  if (!check(invalidSlice == QByteArray(static_cast<int>(sliceBytes), 0),
             "Out-of-range depth slice was not cleared") ||
      !check(!lastError(plugin.object).isEmpty(),
             "Out-of-range depth slice did not report an error"))
    return false;

  QByteArray recoveredSlice(static_cast<int>(sliceBytes), char(0x5a));
  plugin.volume->getDepthSlice(valueD,
    reinterpret_cast<uchar*>(recoveredSlice.data()));
  double recoveredValue = 0.0;
  if (!check(lastError(plugin.object).isEmpty(),
              "A successful depth read retained the previous slice error") ||
      !check(recoveredSlice !=
               QByteArray(static_cast<int>(sliceBytes), char(0x5a)),
             "A successful depth read did not replace the output buffer") ||
      !check(decodedSliceValue(recoveredSlice, voxelType, width, height,
                               valueW, valueH, recoveredValue) &&
               std::fabs(recoveredValue-expectedValue) < 0.0001,
             QString("Recovered slice value %1 does not match %2")
               .arg(recoveredValue).arg(expectedValue)))
    return false;

  return check(plugin.volume->rawValue(depth, 0, 0).toString() == "OutOfBounds",
               "Out-of-range rawValue did not report OutOfBounds");
}

bool createAnalyze(const QString& baseName, bool truncated = false)
{
  QByteArray header(348, 0);
  putInteger<qint32>(header, 0, 348);
  putInteger<qint16>(header, 40, 4);
  putInteger<qint16>(header, 42, 3);
  putInteger<qint16>(header, 44, 2);
  putInteger<qint16>(header, 46, 2);
  putInteger<qint16>(header, 70, 4);
  putInteger<qint16>(header, 72, 16);
  putFloat(header, 80, 0.5f);
  putFloat(header, 84, 0.75f);
  putFloat(header, 88, 1.25f);
  putFloat(header, 108, 4.0f);

  const qint16 values[12] = {
    -32768, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 32767
  };
  QByteArray image(4 + static_cast<int>(sizeof(values)), 0);
  for (int i=0; i<12; ++i)
    putInteger<qint16>(image, 4+2*i, values[i]);
  if (truncated)
    image.chop(1);
  return writeBytes(baseName+".HDR", header) &&
         writeBytes(baseName+".img", image);
}

bool createMetaImage(const QString& headerDirectory, const QString& stem,
                     bool truncated = false,
                     const QString& rawDirectory = QString())
{
  QByteArray raw;
  raw.resize(24);
  const quint16 values[12] = {
    0, 1, 2, 3, 4, 5, 100, 101, 102, 103, 104, 65535
  };
  for (int i=0; i<12; ++i)
    putInteger<quint16>(raw, 2*i, values[i]);
  if (truncated)
    raw.chop(1);
  const QString dataDirectory = rawDirectory.isEmpty() ?
    headerDirectory : rawDirectory;
  const QString headerFile = QDir(headerDirectory).filePath(stem+".mhd");
  const QString rawFile = QDir(dataDirectory).filePath(stem+".raw");
  const QByteArray dataReference =
    MetaImagePathUtils::elementDataFileReference(headerFile, rawFile)
      .toLocal8Bit();
  const QByteArray header =
    "ObjectType = Image\n"
    "NDims = 3\n"
    "BinaryData = True\n"
    "BinaryDataByteOrderMSB = False\n"
    "CompressedData = False\n"
    "ElementSpacing = 0.5 0.75 1.25\n"
    "DimSize = 3 2 2\n"
    "ElementType = MET_USHORT\n"
    "ElementDataFile = " + dataReference + "\n";
  return writeBytes(rawFile, raw) && writeBytes(headerFile, header);
}

bool createVgi(const QString& directory)
{
  QByteArray raw(24, 0);
  const qint16 values[12] = {
    -100, -50, -2, -1, 0, 1, 2, 3, 4, 5, 6, 1234
  };
  for (int i=0; i<12; ++i)
    putInteger<qint16>(raw, 2*i, values[i]);
  const QByteArray header =
    "[volume]\n"
    "name = wrong.vol\n"
    "size = 3 2 2\n"
    "datatype = integer\n"
    "bitsperelement = 16\n"
    "unit = mm\n"
    "[file1]\n"
    "name = \"VolumeCase.VOL\"\n";
  return writeBytes(directory+"/VolumeCase.VOL", raw) &&
         writeBytes(directory+"/volume.vgi", header);
}

bool createTom(const QString& fileName)
{
  QByteArray bytes(512+12, 0);
  putInteger<qint16>(bytes, 0, 3);
  putInteger<qint16>(bytes, 2, 2);
  putInteger<qint16>(bytes, 4, 2);
  putFloat(bytes, 84, 0.5f);
  for (int i=0; i<12; ++i)
    bytes[512+i] = static_cast<char>(i);
  return writeBytes(fileName, bytes);
}

bool createGrdSlice(const QString& fileName, float firstValue)
{
  QByteArray bytes(56+6*4, 0);
  bytes.replace(0, 4, "GRD1");
  putInteger<quint16>(bytes, 4, 2);
  putInteger<quint16>(bytes, 6, 3);
  for (int i=0; i<6; ++i)
    putFloat(bytes, 56+4*i, firstValue+i);
  return writeBytes(fileName, bytes);
}

bool createNc(const QString& fileName, const QVector<quint16>& values,
              int depth, bool unsupportedDouble = false)
{
  int file = -1;
  int dimensions[3] = { -1, -1, -1 };
  int variable = -1;
  const QByteArray encoded = QFile::encodeName(fileName);
  if (nc_create(encoded.constData(), NC_CLOBBER | NC_NETCDF4, &file) != NC_NOERR ||
      nc_def_dim(file, "z", depth, &dimensions[0]) != NC_NOERR ||
      nc_def_dim(file, "y", 2, &dimensions[1]) != NC_NOERR ||
      nc_def_dim(file, "x", 3, &dimensions[2]) != NC_NOERR ||
      nc_def_var(file, "volume", unsupportedDouble ? NC_DOUBLE : NC_USHORT,
                 3, dimensions, &variable) != NC_NOERR ||
      nc_enddef(file) != NC_NOERR)
    {
      if (file >= 0) nc_close(file);
      return false;
    }
  int status = NC_NOERR;
  if (unsupportedDouble)
    {
      QVector<double> converted(values.size());
      for (int i=0; i<values.size(); ++i) converted[i] = values[i];
      status = nc_put_var_double(file, variable, converted.constData());
    }
  else
    status = nc_put_var_ushort(file, variable, values.constData());
  return status == NC_NOERR && nc_close(file) == NC_NOERR;
}

bool testAnalyze(const QString& pluginDir, const QString& temp,
                 DialogResponder& dialogs)
{
  std::cout << "Analyze fixture" << std::endl;
  const QString base = temp+"/analyze";
  if (!check(createAnalyze(base), "Cannot create Analyze fixture")) return false;
  {
    std::cout << "Analyze load" << std::endl;
    LoadedPlugin plugin(pluginDir+"/analyzeplugin.dll");
    std::cout << "Analyze setFile" << std::endl;
    if (!verifyVolume(plugin, QStringList() << base+".HDR", 2, 2, 3,
                      _Short, 32767, 1, 1, 2, 12, dialogs)) return false;
  }

  const QString truncated = temp+"/analyze_truncated";
  std::cout << "Analyze truncated fixture" << std::endl;
  if (!check(createAnalyze(truncated, true),
             "Cannot create truncated Analyze fixture")) return false;
  {
    LoadedPlugin bad(pluginDir+"/analyzeplugin.dll");
    std::cout << "Analyze truncated setFile" << std::endl;
    return check(!bad.volume->setFile(QStringList() << truncated+".HDR") &&
                 !lastError(bad.object).isEmpty(),
                 "Truncated Analyze volume was not rejected with an error");
  }
}

bool testMetaImage(const QString& pluginDir, const QString& temp,
                   DialogResponder& dialogs)
{
  const QString headerDirectory = QDir(temp).filePath(QString::fromUtf8(
    "MHD headers \xe4\xb8\xad\xe6\x96\x87 space"));
  const QString rawDirectory = QDir(temp).filePath(QString::fromUtf8(
    "RAW data \xe4\xb8\xad\xe6\x96\x87 space"));
  if (!check(QDir().mkpath(headerDirectory) && QDir().mkpath(rawDirectory),
             "Cannot create cross-directory MetaImage fixture paths") ||
      !check(createMetaImage(headerDirectory, "meta", false, rawDirectory),
             "Cannot create MetaImage fixture") ||
      !check(createMetaImage(headerDirectory, "meta_truncated", true,
                             rawDirectory),
             "Cannot create truncated MetaImage fixture")) return false;
  {
    LoadedPlugin plugin(pluginDir+"/metaimageplugin.dll");
    if (!verifyVolume(plugin,
                      QStringList() << QDir(headerDirectory).filePath("meta.mhd"),
                      2, 2, 3,
                      _UShort, 65535, 1, 1, 2, 12, dialogs)) return false;
    plugin.volume->replaceFile(
      QDir(headerDirectory).filePath("meta_truncated.mhd"));
    if (!check(!lastError(plugin.object).isEmpty(),
               "Truncated MetaImage replacement was accepted") ||
        !check(plugin.volume->rawValue(1, 1, 2).toUInt() == 65535,
               "Failed MetaImage replacement changed the active volume"))
      return false;
  }

  {
    LoadedPlugin fourD(pluginDir+"/metaimageplugin.dll");
    fourD.volume->set4DVolume(true);
    return check(!fourD.volume->setFile(QStringList() <<
                 QDir(headerDirectory).filePath("meta_truncated.mhd")) &&
                 !lastError(fourD.object).isEmpty(),
                 "4D MetaImage accepted truncated element data");
  }
}

bool testVgi(const QString& pluginDir, const QString& temp,
             DialogResponder& dialogs)
{
  if (!check(createVgi(temp), "Cannot create VGI fixture")) return false;
  LoadedPlugin plugin(pluginDir+"/vgiplugin.dll");
  return verifyVolume(plugin, QStringList() << temp+"/volume.vgi",
                      2, 2, 3, _Short, 1234, 1, 1, 2, 12, dialogs);
}

bool testTom(const QString& pluginDir, const QString& temp,
             DialogResponder& dialogs)
{
  if (!check(createTom(temp+"/volume.tom"), "Cannot create TOM fixture"))
    return false;
  LoadedPlugin plugin(pluginDir+"/tomplugin.dll");
  return verifyVolume(plugin, QStringList() << temp+"/volume.tom",
                      2, 2, 3, _UChar, 11, 1, 1, 2, 6, dialogs,
                      "Histogram");
}

bool testGrd(const QString& pluginDir, const QString& temp,
             DialogResponder& dialogs)
{
  const QString directory = temp+"/grd";
  QDir().mkpath(directory);
  if (!check(createGrdSlice(directory+"/slice01.grd", 0),
             "Cannot create first GRD fixture") ||
      !check(createGrdSlice(directory+"/slice02.grd", 10),
             "Cannot create second GRD fixture")) return false;
  LoadedPlugin plugin(pluginDir+"/grdplugin.dll");
  return verifyVolume(plugin, QStringList() << directory,
                      2, 2, 3, _Float, 15, 1, 1, 2, 24, dialogs,
                      "Load Raw Plugin Dialog");
}

bool testNc4(const QString& pluginDir, const QString& temp,
             DialogResponder& dialogs)
{
  const QVector<quint16> first = { 0, 1, 2, 3, 4, 5 };
  const QVector<quint16> second = { 100, 101, 102, 103, 104, 105 };
  if (!check(createNc(temp+"/one.nc", first, 1),
             "Cannot create first NetCDF fixture") ||
      !check(createNc(temp+"/two.nc", second, 1),
             "Cannot create second NetCDF fixture") ||
      !check(createNc(temp+"/double.nc", first, 1, true),
             "Cannot create unsupported NetCDF fixture")) return false;
  {
    LoadedPlugin plugin(pluginDir+"/nc4plugin.dll");
    if (!verifyVolume(plugin,
                      QStringList() << temp+"/one.nc" << temp+"/two.nc",
                      2, 2, 3, _UShort, 105, 1, 1, 2, 12, dialogs))
      return false;
  }

  {
    LoadedPlugin bad(pluginDir+"/nc4plugin.dll");
    dialogs.expect(QString());
    const bool accepted = bad.volume->setFile(QStringList() << temp+"/double.nc");
    return check(!accepted && !lastError(bad.object).isEmpty(),
                 "Unsupported double NetCDF variable was not rejected") &&
           check(dialogs.unexpected().isEmpty(),
                 QString("NetCDF rejection opened modal dialog %1")
                   .arg(dialogs.unexpected()));
  }
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc == 2)
    {
      const QDir runtimeDirectory(
        QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteDir());
      qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
              runtimeDirectory.filePath("platforms").toLocal8Bit());
    }
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QApplication application(argc, argv);
  if (argc != 2)
    {
      std::cerr << "Usage: legacy_plugins_smoke <plugin-directory>" << std::endl;
      return 2;
    }

  QTemporaryDir temporary;
  if (!temporary.isValid())
    {
      std::cerr << "Cannot create temporary test directory" << std::endl;
      return 2;
    }
  const QString pluginDir = QFileInfo(QString::fromLocal8Bit(argv[1]))
                              .absoluteFilePath();
  DialogResponder dialogs;

  struct Test { const char *name; bool (*run)(const QString&, const QString&,
                                               DialogResponder&); };
  const Test tests[] = {
    { "Analyze", testAnalyze }, { "GRD", testGrd },
    { "MetaImage", testMetaImage }, { "NC4", testNc4 },
    { "TOM", testTom }, { "VGI", testVgi }
  };
  for (const Test& test : tests)
    {
      std::cout << test.name << " starting" << std::endl;
      g_failure.clear();
      if (!test.run(pluginDir, temporary.path(), dialogs))
        {
          std::cerr << test.name << " failed: "
                    << g_failure.toStdString() << std::endl;
          return 1;
        }
      std::cout << test.name << " passed" << std::endl;
    }
  std::cout << "All legacy plugin data-path tests passed" << std::endl;
  return 0;
}
