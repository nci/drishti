#include "../remapwidget.h"
#include "../global.h"

#include <QApplication>
#include <QFileInfo>
#include <QImage>
#include <QPainter>
#include <QStatusBar>
#include <QTextStream>

namespace
{
int fail(const QString& message)
{
  QTextStream(stderr) << "FAILED: " << message << Qt::endl;
  return 1;
}

void renderWidget(RemapWidget *widget)
{
  QImage image(widget->size(), QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::transparent);
  QPainter painter(&image);
  widget->render(&painter);
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", "offscreen");
  if (argc < 4)
    return fail("Usage: remap_tiff_integration_smoke "
                "<tiffplugin.dll> <expected-slices> <tiff> [tiff ...]");

  bool validCount = false;
  const int expectedSlices =
    QString::fromLocal8Bit(argv[2]).toInt(&validCount);
  if (!validCount || expectedSlices <= 0 || argc-3 != expectedSlices)
    return fail("The TIFF slice count does not match the arguments");

  const QString pluginPath =
    QFileInfo(QString::fromLocal8Bit(argv[1])).absoluteFilePath();
  QStringList files;
  for (int index=3; index<argc; ++index)
    files.append(QFileInfo(QString::fromLocal8Bit(argv[index])).absoluteFilePath());

  QApplication application(argc, argv);
  QStatusBar statusBar;
  Global::setStatusBar(&statusBar);
  RemapWidget widget;
  widget.resize(900, 700);
  widget.show();
  application.processEvents();

  if (!widget.setFile(files, pluginPath))
    return fail("RemapWidget rejected the TIFF stack");

  // Force the initial histogram paint and each orthogonal preview path.  This
  // reproduces the UI/plugin composition that previously became reentrant.
  renderWidget(&widget);
  application.processEvents();
  widget.getSlice(expectedSlices/2);
  renderWidget(&widget);
  application.processEvents();
  widget.on_butY_clicked();
  widget.getSlice(32);
  renderWidget(&widget);
  application.processEvents();
  widget.on_butX_clicked();
  widget.getSlice(32);
  renderWidget(&widget);
  application.processEvents();
  widget.on_butZ_clicked();
  widget.getSlice(expectedSlices-1);
  renderWidget(&widget);
  application.processEvents();

  QTextStream(stdout) << "Remap/TIFF integration smoke passed slices="
                      << expectedSlices << Qt::endl;
  Global::setStatusBar(nullptr);
  return 0;
}
