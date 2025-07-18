#include "colormaps.h"

#include <QCoreApplication>
#include <QDirIterator>
#include <QGradientStops>
#include <QPainter>
#include <QProgressBar>
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QListView>
#include <QVBoxLayout>
#include <QStringListModel>
#include <QPushButton>
#include <QMessageBox>
#include <QGridLayout>


ColorMaps::ColorMaps() : QObject()
{
  m_colorMap.clear();
  m_colorImage.clear();
  m_comboBox = 0;
  m_comboBoxQualitative = 0;
}

void
ColorMaps::loadColorMaps()
{    
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


  if (!m_comboBox)
    {
      m_comboBox = new QComboBox();
      m_comboBox->setPlaceholderText("Select ColorMap");
      m_comboBox->setIconSize(QSize(100, 50));
      m_comboBoxQualitative = new QComboBox();
      m_comboBoxQualitative->setPlaceholderText("Select ColorMap");
      m_comboBoxQualitative->setIconSize(QSize(100, 50));
    }
  else
    {
      m_comboBox->clear();
      m_comboBoxQualitative->clear();
    }

  
  QString colormap_path = app.absolutePath();

  QStringList filters;
  filters << "*.rgb";

  QDirIterator it(colormap_path,
		  {"*.rgb"},
		  QDir::Files,
		  QDirIterator::Subdirectories);

  while (it.hasNext())
    {
      QFile f(it.next());
      f.open(QFile::ReadOnly);
      QTextStream in(&f);
      int ncolors = in.readLine().split("=", Qt::SkipEmptyParts)[1].toInt();
      in.readLine();
      QList<QColor> colors;
      QGradientStops cgrad;
      for (int i=0; i<ncolors; i++)
	{
	  QStringList rgb = in.readLine().split(" ", Qt::SkipEmptyParts);
	  uchar r = rgb[0].toInt();
	  uchar g = rgb[1].toInt();
	  uchar b = rgb[2].toInt();
	  colors << QColor(r, g, b);
	  cgrad << QGradientStop((float)i/(float)(ncolors-1), QColor(r,g,b));
	}

      // generate image for the color map
      QImage img(255,50, QImage::Format_ARGB32_Premultiplied);
      QPainter painter(&img);
      img.fill(Qt::transparent);
      QLinearGradient grd(0, 0, 255, 0);
      grd.setStops(cgrad);
      painter.fillRect(img.rect(), grd);
      painter.end();
      
      QString colorName = it.fileName();
      
      colorName.chop(4);
      m_colorMap[colorName] = colors;
      m_colorImage[colorName] = img;

      if (it.filePath().contains("qualitative", Qt::CaseInsensitive))
	m_comboBoxQualitative->addItem(QIcon(QPixmap::fromImage(img)), colorName);
      else
	m_comboBox->addItem(QIcon(QPixmap::fromImage(img)), colorName);
    }
}


QList<QColor>
ColorMaps::getColorMap(int flag)
{
  if (m_colorMap.count() == 0)
    loadColorMaps();


  
  // use QMessageBox to display the combobox
  QMessageBox *box = new QMessageBox(QMessageBox::Question,
				     "Color Map Selection",
				     "Select Color Map",
				     QMessageBox::Ok | QMessageBox::Cancel);
  QLayout *layout = box->layout();
  if (flag == 0) // qualitative color scheme
    ((QGridLayout*)layout)->addWidget(m_comboBoxQualitative, 1, 2);
  else
    ((QGridLayout*)layout)->addWidget(m_comboBox, 1, 2);

  int ret = box->exec();

  if (ret != QMessageBox::Cancel)
    {
      QString mapName;

      if (flag == 0) // qualitative color scheme
	mapName = m_comboBoxQualitative->currentText();
      else
	mapName = m_comboBox->currentText();

      return m_colorMap[mapName];
    }
  else
    return QList<QColor>();
}
