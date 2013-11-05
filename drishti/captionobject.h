#ifndef CAPTIONOBJECT_H
#define CAPTIONOBJECT_H

#include <QtGui>

#include <fstream>
using namespace std;

class CaptionObject
{
 public :
  CaptionObject();
  ~CaptionObject();
  
  QImage image();

  void load(fstream&);
  void save(fstream&);

  void clear();

  bool hasCaption(QStringList);

  void set(QPointF, QString, QFont,
	   QColor, QColor, float);
  void setCaption(CaptionObject);
  void setPosition(QPointF);
  void setText(QString);
  void setFont(QFont);
  void setColor(QColor);
  void setHaloColor(QColor);
  void setAngle(float);

  QPointF position();
  QString text();
  QFont font();
  QColor color();
  QColor haloColor();
  int height();
  int width();
  float angle();

  static CaptionObject interpolate(CaptionObject&,
				   CaptionObject&,
				   float);

  static QList<CaptionObject> interpolate(QList<CaptionObject>,
					  QList<CaptionObject>,
					  float);
  
 private :
  QImage m_image;
  QPointF m_pos;
  QString m_text;
  QFont m_font;
  QColor m_color;
  QColor m_haloColor;
  int m_height, m_width;
  float m_angle;

  bool m_recreate;

  void createImage();
};

#endif
