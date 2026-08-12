#include "../common.h"
#include "../volinterface.h"

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPluginLoader>
#include <QSpinBox>
#include <QTemporaryDir>
#include <QTimer>

#include <cstring>
#include <iostream>
#include <limits>

namespace
{
int fail(const QString& message)
{
  std::cerr << message.toStdString() << std::endl;
  return 1;
}

QString pluginError(QObject *pluginObject)
{
  QString error;
  if (!QMetaObject::invokeMethod(pluginObject, "lastError",
                                 Qt::DirectConnection,
                                 Q_RETURN_ARG(QString, error)))
    return "lastError() could not be invoked";
  return error;
}

bool pluginCanceled(QObject *pluginObject)
{
  bool canceled = false;
  return QMetaObject::invokeMethod(pluginObject, "wasCanceled",
                                   Qt::DirectConnection,
                                   Q_RETURN_ARG(bool, canceled)) && canceled;
}

bool writeBytes(const QString& fileName, const QByteArray& bytes)
{
  QFile file(fileName);
  return file.open(QFile::WriteOnly | QFile::Truncate) &&
         file.write(bytes) == bytes.size();
}

bool writeSlab(const QString& fileName, uchar code, qint16 value)
{
  const qint32 dimension = 1;
  QFile file(fileName);
  if (!file.open(QFile::WriteOnly | QFile::Truncate))
    return false;
  return file.write(reinterpret_cast<const char*>(&code), sizeof(code)) ==
           sizeof(code) &&
         file.write(reinterpret_cast<const char*>(&dimension),
                    sizeof(dimension)) == sizeof(dimension) &&
         file.write(reinterpret_cast<const char*>(&dimension),
                    sizeof(dimension)) == sizeof(dimension) &&
         file.write(reinterpret_cast<const char*>(&dimension),
                    sizeof(dimension)) == sizeof(dimension) &&
         file.write(reinterpret_cast<const char*>(&value), sizeof(value)) ==
           sizeof(value);
}

class DialogDriver : public QObject
{
public:
  enum Mode { Slices, Slabs };

  explicit DialogDriver(Mode mode) : m_mode(mode)
  {
    connect(&m_timer, &QTimer::timeout, this, &DialogDriver::drive);
    m_timer.start(5);
  }

  void stop() { m_timer.stop(); }
  bool complete() const
  {
    return m_sawHistogram &&
           ((m_mode == Slices && m_sawConfiguration) ||
            (m_mode == Slabs && m_sawFileDialog)) &&
           m_unexpectedMessage.isEmpty();
  }
  QString unexpectedMessage() const { return m_unexpectedMessage; }

private:
  void drive()
  {
    const QWidgetList widgets = QApplication::topLevelWidgets();
    for (QWidget *widget : widgets)
      {
        if (!widget->isVisible())
          continue;
        if (QMessageBox *message = qobject_cast<QMessageBox*>(widget))
          {
            m_unexpectedMessage = message->text();
            message->accept();
            continue;
          }
        if (QFileDialog *fileDialog = qobject_cast<QFileDialog*>(widget))
          {
            m_sawFileDialog = true;
            fileDialog->reject();
            continue;
          }
        if (QInputDialog *inputDialog = qobject_cast<QInputDialog*>(widget))
          {
            m_sawHistogram = true;
            inputDialog->accept();
            continue;
          }

        QDialog *dialog = qobject_cast<QDialog*>(widget);
        QLineEdit *grid = dialog ?
          dialog->findChild<QLineEdit*>("gridSize") : 0;
        if (dialog && grid)
          {
            QComboBox *voxelType =
              dialog->findChild<QComboBox*>("voxelType");
            QSpinBox *headerBytes =
              dialog->findChild<QSpinBox*>("headerBytes");
            if (!voxelType || !headerBytes)
              {
                m_unexpectedMessage = "RAW slices dialog controls are missing";
                dialog->reject();
                continue;
              }
            m_sawConfiguration = true;
            voxelType->setCurrentIndex(_Char);
            headerBytes->setValue(0);
            grid->setText("1 1 0");
            dialog->accept();
          }
      }
  }

  Mode m_mode;
  QTimer m_timer;
  bool m_sawConfiguration = false;
  bool m_sawFileDialog = false;
  bool m_sawHistogram = false;
  QString m_unexpectedMessage;
};

class CancelDialogDriver : public QObject
{
public:
  CancelDialogDriver()
  {
    connect(&m_timer, &QTimer::timeout, this, &CancelDialogDriver::drive);
    m_timer.start(5);
  }

  void stop() { m_timer.stop(); }
  bool sawDialog() const { return m_sawDialog; }

private:
  void drive()
  {
    for (QWidget *widget : QApplication::topLevelWidgets())
      if (QDialog *dialog = qobject_cast<QDialog*>(widget))
        if (dialog->isVisible())
          {
            m_sawDialog = true;
            dialog->reject();
          }
  }

  QTimer m_timer;
  bool m_sawDialog = false;
};

VolInterface *loadPlugin(const QString& fileName, QPluginLoader& loader,
                         QString& error)
{
  loader.setFileName(fileName);
  QObject *pluginObject = loader.instance();
  if (!pluginObject)
    {
      error = loader.errorString();
      return 0;
    }
  VolInterface *plugin = qobject_cast<VolInterface*>(pluginObject);
  if (!plugin)
    error = "loaded object does not implement VolInterface";
  return plugin;
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc != 3)
    return fail("usage: native_raw_collections_plugin_smoke "
                "<rawslicesplugin.dll> <rawslabsplugin.dll>");

  const QFileInfo slicesPluginFile(
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath());
  QDir runtimeDirectory = slicesPluginFile.absoluteDir();
  if (!runtimeDirectory.cdUp())
    return fail("cannot locate the RAW collection plugin runtime directory");
  qputenv("QT_QPA_PLATFORM_PLUGIN_PATH",
          QFile::encodeName(runtimeDirectory.filePath("platforms")));

  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  QApplication application(argc, argv);

  QTemporaryDir fixtures;
  if (!fixtures.isValid())
    return fail("cannot create temporary fixture directory");

  const QString lowSlice = fixtures.filePath("slice-low.raw");
  const QString highSlice = fixtures.filePath("slice-high.raw");
  if (!writeBytes(lowSlice, QByteArray(1, static_cast<char>(0x80))) ||
      !writeBytes(highSlice, QByteArray(1, static_cast<char>(0x7f))))
    return fail("cannot write RAW slice fixtures");

  QString error;
  QPluginLoader slicesLoader;
  VolInterface *slices = loadPlugin(QString::fromLocal8Bit(argv[1]),
                                    slicesLoader, error);
  if (!slices)
    return fail(QString("cannot load RAW slices plugin: %1").arg(error));

  slices->init();
  CancelDialogDriver cancelDialogs;
  const bool canceledSlicesLoad =
    slices->setFile(QStringList() << lowSlice << highSlice);
  cancelDialogs.stop();
  if (canceledSlicesLoad || !cancelDialogs.sawDialog() ||
      !pluginCanceled(slicesLoader.instance()))
    return fail("canceling the RAW slices dialog was not reported as cancellation");

  slices->init();
  slices->set4DVolume(false);
  DialogDriver slicesDialogs(DialogDriver::Slices);
  const bool slicesLoaded =
    slices->setFile(QStringList() << lowSlice << highSlice);
  slicesDialogs.stop();
  if (!slicesLoaded || !slicesDialogs.complete())
    return fail(QString("RAW slices import failed: %1 %2")
                  .arg(slicesDialogs.unexpectedMessage(),
                       pluginError(slicesLoader.instance())));
  if (pluginCanceled(slicesLoader.instance()))
    return fail("a successful RAW slices load retained a stale cancellation state");

  int depth = 0;
  int width = 0;
  int height = 0;
  slices->gridSize(depth, width, height);
  const QList<uint> slicesHistogram = slices->histogram();
  if (depth != 2 || width != 1 || height != 1 ||
      slices->rawMin() != -128 || slices->rawMax() != 127 ||
      slicesHistogram.size() != 256 ||
      slicesHistogram[0] != 1 || slicesHistogram[255] != 1 ||
      slices->rawValue(0, 0, 0).toInt() != -128 ||
      slices->rawValue(1, 0, 0).toInt() != 127)
    return fail("RAW slices signed extrema or histogram are incorrect");

  const QString lowSlab = fixtures.filePath("slab-low.raw");
  const QString highSlab = fixtures.filePath("slab-high.raw");
  if (!writeSlab(lowSlab, 3, std::numeric_limits<qint16>::min()) ||
      !writeSlab(highSlab, 3, std::numeric_limits<qint16>::max()))
    return fail("cannot write RAW slab fixtures");

  QPluginLoader slabsLoader;
  VolInterface *slabs = loadPlugin(QString::fromLocal8Bit(argv[2]),
                                   slabsLoader, error);
  if (!slabs)
    return fail(QString("cannot load RAW slabs plugin: %1").arg(error));
  slabs->init();
  slabs->set4DVolume(false);
  DialogDriver slabsDialogs(DialogDriver::Slabs);
  const bool slabsLoaded = slabs->setFile(QStringList() << lowSlab << highSlab);
  slabsDialogs.stop();
  if (!slabsLoaded || !slabsDialogs.complete())
    return fail(QString("RAW slabs import failed: %1 %2")
                  .arg(slabsDialogs.unexpectedMessage(),
                       pluginError(slabsLoader.instance())));

  slabs->gridSize(depth, width, height);
  const QList<uint> slabsHistogram = slabs->histogram();
  if (depth != 2 || width != 1 || height != 1 ||
      slabs->rawMin() != -32768 || slabs->rawMax() != 32767 ||
      slabsHistogram.size() != 65536 ||
      slabsHistogram[0] != 1 || slabsHistogram[65535] != 1 ||
      slabs->rawValue(0, 0, 0).toInt() != -32768 ||
      slabs->rawValue(1, 0, 0).toInt() != 32767)
    return fail("RAW slabs signed extrema or cross-file histogram are incorrect");

  QFile truncated(highSlab);
  if (!truncated.open(QFile::WriteOnly | QFile::Truncate) ||
      truncated.write("x", 1) != 1)
    return fail("cannot truncate RAW slab fixture");
  truncated.close();
  uchar slice[2] = { 0xff, 0xff };
  slabs->getDepthSlice(1, slice);
  if (slice[0] != 0 || slice[1] != 0 ||
      pluginError(slabsLoader.instance()).isEmpty())
    return fail("a post-load RAW slab short read was not cleared and reported");

  std::cout << "Native RAW collection plugins smoke passed" << std::endl;
  return 0;
}
