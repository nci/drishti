#ifndef CAPTIONDIALOG_H
#define CAPTIONDIALOG_H

#include "ui_captiondialog.h"

class CaptionDialog : public QDialog
{
  Q_OBJECT

 public :
  CaptionDialog(QWidget*,
		QString, QFont, QColor, float angle = 0);
  CaptionDialog(QWidget*,
		QString, QFont, QColor, QColor, float angle = 0);

  QString text();
  QFont font();
  QColor color();
  QColor haloColor();
  float angle();

  void hideOpacity(bool);
  void hideText(bool);
  void hideAngle(bool);

 private slots :
  void on_font_pressed();
  void on_color_pressed();
  void on_haloColor_pressed();

 private :
  Ui::CaptionInputDialog ui;

  QFont m_font;
  QColor m_color;
  QColor m_haloColor;
  float m_angle;
};

#endif
