#ifndef VIEWSEDITOR_H
#define VIEWSEDITOR_H

#include "glewinitialisation.h"
#include "viewinformation.h"

#include <QtGui>
#include "ui_viewseditor.h"

class ViewsEditor : public QWidget
{
 Q_OBJECT

 public :
  ViewsEditor(QWidget *parent=0);

  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);
  void mousePressEvent(QMouseEvent*);

  void load(fstream&);
  void save(fstream&);

 signals :
  void showMessage(QString, bool);
  void currentView();
  void updateVolInfo(int);
  void updateVolInfo(int, int);
  void updateVolumeBounds(int, Vec, Vec);
  void updateVolumeBounds(int, int, Vec, Vec);
  void updateVolumeBounds(int, int, int, Vec, Vec);
  void updateVolumeBounds(int, int, int, int, Vec, Vec);
  void updateLookFrom(Vec, Quaternion, float);
  void updateLightInfo(LightingInformation);
  void updateBrickInfo(QList<BrickInformation>);
  void updateTransferFunctionManager(QList<SplineInformation>);
  void updateParameters(float, float,
			int,
			bool, bool, Vec,
			QString,
			int, int, QString, QString, QString);
  void updateTagColors();
  void updateGL();

 public slots :
  void clear();
  void setHiresMode(bool);
  void addView(float,
	       float,
	       bool, bool,
	       Vec,
	       Vec, Quaternion,
	       float,
	       QImage,
	       int,
	       LightingInformation,
	       QList<BrickInformation>,
	       Vec, Vec,
	       QList<SplineInformation>,
	       int, int, QString, QString, QString);
  
 private slots :
  void on_add_pressed();
  void on_remove_pressed();
  void on_restore_pressed();
  void on_up_pressed();
  void on_down_pressed();
  void updateTransition();

 private :
  Ui::ViewsEditorWidget ui;

  int m_leftMargin, m_topMargin;
  int m_row, m_maxRows;
  int m_imgSize;

  int m_selected;
  QList<QRect> m_Rect;
  QList<ViewInformation*> m_viewInfo;

  bool m_hiresMode;

  bool m_inTransition;
  int m_fade;

  void calcRect();
  void drawView(QPainter*, QColor, QColor, int, QRect, QImage);
};


#endif
