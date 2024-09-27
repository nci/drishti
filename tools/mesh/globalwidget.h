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
  void on_voxelUnit_currentIndexChanged(int);
  void on_voxelSize_editingFinished();
  
 signals :
  void bgColor();
  void shadowBox(bool);
  void newVoxelUnit();
  void newVoxelSize();
  
 private :
  Ui::GlobalWidget ui;

};

#endif
