#ifndef BRICKSDIALOG_H
#define BRICKSDIALOG_H

#include "ui_brickswidget.h"

#include "bricks.h"

class BricksWidget : public QWidget
{
 Q_OBJECT

 public :
  BricksWidget(QWidget *parent=NULL,
	       Bricks *bricks=NULL);
  
 signals :
  void updateGL();
  void showMessage(QString, bool);

 public slots :
  void refresh();
  void setBrickZeroRotation(int, float);
  void keyPressEvent(QKeyEvent*);

 private slots :
  void on_m_showAxis_stateChanged(int);
  void on_m_translation_editingFinished();
  void on_m_pivotFromHitpoint_pressed();
  void on_m_pivot_editingFinished();
  void on_m_axisFromHitpoint_pressed();
  void on_m_axis_editingFinished();
  void on_m_angle_editingFinished();

  void fillInformation(int);
  void updateBrickInformation();

 private :
  Ui::BricksWidget ui;

  Bricks *m_bricks;
  int m_selected;

  bool getHitpoint(Vec&);
  void updateClipTable(int);

  void showHelp();
};

#endif
