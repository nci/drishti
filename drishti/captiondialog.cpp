#include "captiondialog.h"
#include "dcolordialog.h"
#include <QFontDialog>

float CaptionDialog::angle() { return ui.angle->value(); }
QString CaptionDialog::text() { return ui.text->text(); }
QFont CaptionDialog::font() { return m_font; }
QColor CaptionDialog::color()
{
  QColor color = QColor::fromRgbF(m_color.redF(),
				  m_color.greenF(),
				  m_color.blueF(),
				  ui.opacity->value());

  return color;
}
QColor CaptionDialog::haloColor()
{
  QColor color = QColor::fromRgbF(m_haloColor.redF(),
				  m_haloColor.greenF(),
				  m_haloColor.blueF(),
				  ui.opacity->value());

  return color;
}

CaptionDialog::CaptionDialog(QWidget *parent,
			     QString text,
			     QFont font,
			     QColor color,
			     float angle) :
  QDialog(parent)
{
  ui.setupUi(this);
  
  m_font = font;
  m_color = color;

  ui.text->setText(text);

  float a = color.alphaF();
  ui.opacity->setValue(a);

  ui.haloColor->hide();

  ui.angle->setValue(angle);
  ui.angle->hide();
  ui.angleLabel->hide();
}

CaptionDialog::CaptionDialog(QWidget *parent,
			     QString text,
			     QFont font,
			     QColor color,
			     QColor haloColor,
			     float angle) :
  QDialog(parent)
{
  ui.setupUi(this);
  
  m_font = font;
  m_color = color;
  m_haloColor = haloColor;

  ui.text->setText(text);

  float a = color.alphaF();
  ui.opacity->setValue(a);

  ui.angle->setValue(angle);
  ui.angle->hide();
  ui.angleLabel->hide();
}

void
CaptionDialog::hideOpacity(bool flag)
{
  if (flag)
    {
      ui.opacity->hide();
      ui.opacityLabel->hide();
    }
  else
    {
      ui.opacity->show();
      ui.opacityLabel->show();
    }
}

void
CaptionDialog::hideText(bool flag)
{
  if (flag)
    ui.text->hide();
  else
    ui.text->show();
}

void
CaptionDialog::hideAngle(bool flag)
{
  if (flag)
    {
      ui.angle->hide();
      ui.angleLabel->hide();
    }
  else
    {
      ui.angle->show();
      ui.angleLabel->show();
    }
}

void
CaptionDialog::on_font_pressed()
{
  bool ok;
  QFont fnt = QFontDialog::getFont(&ok, m_font, this);
  if (ok)
    m_font = fnt;
}

void
CaptionDialog::on_color_pressed()
{
  QColor color = DColorDialog::getColor(m_color);
  if (color.isValid())
    m_color = color;
}

void
CaptionDialog::on_haloColor_pressed()
{
  QColor color = DColorDialog::getColor(m_haloColor);
  if (color.isValid())
    m_haloColor = color;
}
