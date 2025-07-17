#ifndef COLORMAPS_H
#define COLORMAPS_H

#include <QWidget>
#include <QString>
#include <QList>
#include <QComboBox>
#include <QColor>

class ColorMaps : public QObject
{
  Q_OBJECT
  
 public :
  ColorMaps();
  
  QList<QColor> getColorMap();
  
 private :
  QMap<QString, QList<QColor>> m_colorMap;
  QMap<QString, QImage> m_colorImage;

  QComboBox* m_comboBox;

  void loadColorMaps();
};

#endif
