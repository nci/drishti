#include "colormaps.h"

#include <QCoreApplication>
#include <QDirIterator>
#include <QGradientStops>
#include <QPainter>
#include <QProgressBar>
#include <QIcon>
#include <QFile>
#include <QTextStream>


ColorMaps::ColorMaps() : QObject()
{
  m_colorMap.clear();
  m_colorImage.clear();
  m_comboBox = 0;
}

void
ColorMaps::loadColorMaps()
{
  m_comboBox = new QComboBox();
  m_comboBox->setPlaceholderText("Select ColorMap");
  
#if defined(Q_OS_LINUX)
  QDir app = QCoreApplication::applicationDirPath();
  app.cd("assets");
  app.cd("colormaps");
#elif defined(Q_OS_MAC)
  QDir app = QCoreApplication::applicationDirPath();
  app.cdUp();
  app.cdUp();
  app.cd("Shared");
  app.cd("assets");
  app.cd("colormaps");
#elif defined(Q_OS_WIN32)

  QDir app = QCoreApplication::applicationDirPath();
  app.cd("assets");
  app.cd("colormaps");
#else
  #error Unsupported platform.
#endif

  QString colormap_path = app.absolutePath();

  QStringList filters;
  filters << "*.rgb";

  QDirIterator it(colormap_path,
		  {"*.rgb"},
		  QDir::Files,
		  QDirIterator::Subdirectories);

  while (it.hasNext())
    {
      QString colorName = it.fileName();
      colorName.chop(4);
      
      QFile f(it.next());
      f.open(QFile::ReadOnly);
      QTextStream in(&f);
      int ncolors = in.readLine().split("=")[1].toInt();
      in.readLine();
      QList<QColor> colors;
      QGradientStops cgrad;
      for (int i=0; i<ncolors; i++)
	{
	  QStringList rgb = in.readLine().split(" ");
	  uchar r = rgb[0].toInt();
	  uchar g = rgb[1].toInt();
	  uchar b = rgb[2].toInt();
	  colors << QColor(r, g, b);
	  cgrad << QGradientStop((float)i/(float)(ncolors-1), QColor(r,g,b));
	}
      
      m_colorMap[colorName] = colors;

      QImage img(255,10, QImage::Format_ARGB32_Premultiplied);
      QPainter painter(&img);
      img.fill(Qt::transparent);
      QLinearGradient grd(0, 0, 255, 0);
      grd.setStops(cgrad);
      painter.fillRect(img.rect(), grd);
      painter.end();
      
      m_colorImage[colorName] = img;

      m_comboBox->addItem(QIcon(QPixmap::fromImage(img)), colorName);
    }
}

//void
//ColorMaps::colorMapSelection()
//{
//}

QList<QColor>
ColorMaps::getColorMap()
{
  if (m_colorMap.count() == 0)
    {
      loadColorMaps();
    }
  
  m_comboBox->show();
  m_comboBox->setCurrentIndex(1);

  return m_colorMap["rainbow_light"];
}
