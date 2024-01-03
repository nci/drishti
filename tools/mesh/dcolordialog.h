#ifndef DCOLORDIALOG_H
#define DCOLORDIALOG_H

#include "dcolorwheel.h"
#include <QDialog>
#include <QDialogButtonBox>

class DColorDialog : public QDialog
{
  Q_OBJECT

 public :
  DColorDialog(QColor color=Qt::white,
	       QWidget* parent=0,
	       Qt::WindowFlags f=Qt::MSWindowsFixedSizeDialogHint);
 
  void setColor(QColor);
  QColor color();

  static QColor getColor(const QColor&);

 public slots :
  void more(bool);
  void colorChanged(QColor);

 private :
  QColor m_color;
  DColorWheel *colorwheel;

  QDialogButtonBox *buttonBox;
  QPushButton *okButton;
  QPushButton *cancelButton;
  QPushButton *moreButton;
};

#endif
