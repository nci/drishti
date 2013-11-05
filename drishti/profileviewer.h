#ifndef PROFILEVIEWER_H
#define PROFILEVIEWER_H

#include <QtGui>
#include "ui_profileviewer.h"

class ProfileViewer : public QWidget
{
  Q_OBJECT

 public :
  ProfileViewer(QWidget *parent=NULL);

  void setValueRange(float, float);
  void setGraphValues(float, float,
		      QList<uint>,
		      QList<float>);
  void setGraphValues(float, float,
		      QList<uint>,
		      QList<float>, QList<float>, QList<float>);

  void resizeEvent(QResizeEvent*);

 public slots :
  void generateScene();

 private slots :
  void on_saveImage_clicked();
  void on_caption_editingFinished();

 private :
  Ui::ProfileViewer ui;

  QGraphicsScene *m_scene;
  int m_height, m_width;
  QGraphicsTextItem *m_captionTextItem;
  QString m_captionString;

  float m_minV, m_maxV;
  QList<uint> m_index;
  QList<float> m_graphValues;
  QList<float> m_graphValMin;
  QList<float> m_graphValMax;
};

#endif
