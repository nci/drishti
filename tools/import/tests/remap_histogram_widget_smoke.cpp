#include <QApplication>
#include <QGradient>
#include <QImage>
#include <QKeyEvent>
#include <QPainter>
#include <QTextStream>

#include <cmath>

#define private public
#include "../remaphistogramwidget.h"
#undef private

namespace
{
int fail(const QString& message)
{
  QTextStream(stderr) << "FAILED: " << message << Qt::endl;
  return 1;
}

bool finiteMap(const QList<float>& values)
{
  for(float value : values)
    if (!std::isfinite(value))
      return false;
  return true;
}

void renderWidget(RemapHistogramWidget& widget)
{
  QImage image(widget.size(), QImage::Format_ARGB32_Premultiplied);
  image.fill(Qt::black);
  QPainter painter(&image);
  widget.render(&painter);
}
}

int main(int argc, char **argv)
{
  qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
  QApplication application(argc, argv);

  RemapHistogramWidget widget;
  widget.resize(640, 360);
  int histogramRequests = 0;
  QObject::connect(&widget, &RemapHistogramWidget::getHistogram,
                   [&histogramRequests]() { ++histogramRequests; });
  widget.setGradientStops(QGradientStops()
                          << QGradientStop(0.0, Qt::black)
                          << QGradientStop(1.0, Qt::white));

  renderWidget(widget);
  if (histogramRequests != 0)
    return fail("Painting an empty histogram requested synchronous volume I/O.");

  QList<uint> singlePeak;
  for(int i=0; i<256; ++i)
    singlePeak.append(0);
  singlePeak[123] = 100;
  widget.setRawMinMax(12.0f, 240.0f, 0, 255);
  widget.setHistogram(singlePeak);
  widget.setRawMinMax(123.0f, 123.0f, 0, 255);
  widget.setHistogram(singlePeak);

  if (widget.m_histMax != 100 || widget.m_histogramScaled.size() != 256 ||
      widget.m_histogramScaled[123] != 200)
    return fail("A single histogram peak did not receive a finite scale.");
  if (widget.m_Line->ticks().size() != 2 ||
      widget.m_Line->ticksOriginal().size() != 2)
    return fail("A constant integer volume collapsed the endpoint ticks.");

  const QList<float> constantMap = widget.rawMap();
  const QList<int> constantPvlMap = widget.pvlMap();
  if (constantMap.size() != 2 || constantPvlMap.size() != 2 ||
      !finiteMap(constantMap) || constantMap[0] != 123.0f ||
      constantMap[1] != 123.0f)
    return fail("A constant integer volume produced an invalid value map.");

  QKeyEvent homeEvent(QEvent::KeyPress, Qt::Key_H, Qt::NoModifier);
  QKeyEvent rightEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier);
  QKeyEvent leftEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
  widget.keyPressEvent(&homeEvent);
  widget.keyPressEvent(&rightEvent);
  widget.keyPressEvent(&leftEvent);
  renderWidget(widget);

  QList<uint> allZero;
  for(int i=0; i<256; ++i)
    allZero.append(0);
  widget.setHistogram(allZero);
  renderWidget(widget);
  if (widget.m_histMax != 0 || widget.m_histogramScaled.size() != 256)
    return fail("An all-zero histogram did not remain a valid zero plot.");
  for(uint value : widget.m_histogramScaled)
    if (value != 0)
      return fail("An all-zero histogram produced a nonzero plot value.");

  widget.setRawMinMax(7.0f, 7.0f, -1, -1);
  widget.setHistogram(QList<uint>() << 9);
  renderWidget(widget);
  if (widget.m_histMax != 9 || widget.m_histogramScaled.size() != 1 ||
      widget.m_histogramScaled[0] != 200 || !finiteMap(widget.rawMap()))
    return fail("A one-bin floating-point histogram was not handled safely.");

  widget.resize(1, 1);
  renderWidget(widget);
  if (widget.m_lineWidth < 1)
    return fail("A narrow widget produced a non-positive histogram width.");

  QTextStream(stdout) << "Remap histogram widget smoke passed" << Qt::endl;
  return 0;
}
