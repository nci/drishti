#ifndef LIGHTINGWIDGET_H
#define LIGHTINGWIDGET_H

#include "lightinginformation.h"
//#include "directionvectorwidget.h"
#include "ui_lightingwidget.h"
#include "lightdisc.h"


class LightingWidget : public QWidget
{
 Q_OBJECT

 public :
  LightingWidget(QWidget *parent=NULL);

 signals :
  void applyLighting(bool);
  void highlights(Highlights);

  void softness(float);
  void edges(float);
  void shadowIntensity(float);
  void valleyIntensity(float);
  void peakIntensity(float);

  void lightDirectionChanged(Vec);
  
  void updateGL();
  
 public slots :
  void setLightInfo(LightingInformation);
  void keyPressEvent(QKeyEvent*);

 private slots :
  void on_ambient_valueChanged(int);
  void on_diffuse_valueChanged(int);
  void on_specular_valueChanged(int);
  void on_specularcoeff_valueChanged(int);

  void on_softness_valueChanged(int);
  void on_edges_valueChanged(int);
  void on_shadowIntensity_valueChanged(int);
  void on_valleyIntensity_valueChanged(int);
  void on_peakIntensity_valueChanged(int);

  void on_gamma_valueChanged(int);
    
  void highlightsChanged();

  void updateDirection(QPointF);
  
 private :
  Ui::LightingWidget ui;

  LightDisc *m_lightdisc;
  Vec m_direction;

  
  void showHelp();
};

#endif
