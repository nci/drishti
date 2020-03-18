#ifndef LIGHTINGWIDGET_H
#define LIGHTINGWIDGET_H

#include "lightinginformation.h"
#include "directionvectorwidget.h"
#include "ui_lightingwidget.h"


class LightingWidget : public QWidget
{
 Q_OBJECT

 public :
  LightingWidget(QWidget *parent=NULL);

 signals :
  void applyLighting(bool);
  void applyEmissive(bool);
  void highlights(Highlights);

  void applyShadow(bool);
  void shadowBlur(float);
  void shadowScale(float);
  void shadowFOV(float);
  void shadowIntensity(float);

  void directionChanged(Vec);
  void lightDistanceOffset(float);

  void peel(bool);
  void peelInfo(int, float, float, float);

  void updateGL();
  
 public slots :
  void setLightInfo(LightingInformation);
  void keyPressEvent(QKeyEvent*);

 private slots :
  void lightDirectionChanged(float, Vec);

  void on_lightposition_clicked(bool);
  void on_applyemissive_clicked(bool);
  void on_applylighting_clicked(bool);

  void on_peel_clicked(bool);
  void on_peelmin_sliderReleased();
  void on_peelmax_sliderReleased();
  void on_peelmix_sliderReleased();
  void on_peeltype_currentIndexChanged(int);

  void on_ambient_valueChanged(int);
  void on_diffuse_valueChanged(int);
  void on_specular_valueChanged(int);
  void on_specularcoeff_valueChanged(int);

  void on_shadowblur_valueChanged(int);
  void on_shadowcontrast_valueChanged(int);

  void on_gamma_valueChanged(int);
    
  void setFlat();
  void highlightsChanged();

 private :
  Ui::LightingWidget ui;

  //DirectionVectorWidget *m_lightPosition;

  void peelSliderReleased();
  void showHelp();
};

#endif
