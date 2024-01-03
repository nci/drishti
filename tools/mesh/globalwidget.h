#ifndef GLOBALWIDGET_H
#define GLOBALWIDGET_H

#include "ui_globalwidget.h"

class GlobalWidget : public QWidget
{
  Q_OBJECT


 public :
  GlobalWidget(QWidget *parent=NULL);

  void addWidget(QWidget*);
  void setShadowBox(bool);
			  
 private slots :
  void on_bgColor_pressed();  
  void on_shadowBox_clicked(bool);
  
 signals :
  void bgColor();
  void shadowBox(bool);

 private :
  Ui::GlobalWidget ui;

};

#endif
