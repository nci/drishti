#ifndef PREFERENCESWIDGET_H
#define PREFERENCESWIDGET_H

#include "ui_preferenceswidget.h"
#include "tagcoloreditor.h"

class PreferencesWidget : public QWidget
{
 Q_OBJECT

 public :
  PreferencesWidget(QWidget *parent=NULL);

  void save(const char*);
  void load(const char*);
  void getTick(int&, int&, QString&, QString&, QString&);

 signals :
  void updateLookupTable();
  void updateGL();
  void stereoSettings(float, float, float);
  void tagColorChanged();

 public slots :
  void changeStill(int);
  void changeDrag(int);
  void setRenderQualityValues(float, float);
  void setImageQualitySliderValues(int, int);
  void setTick(int, int, QString, QString, QString);
  void enableStereo(bool);
  void updateStereoSettings(float, float, float);
  void updateFocusSetting(float);
  void updateTextureMemory();
  void updateTagColors();

 private slots :
  void on_m_textureMemorySize_valueChanged(int);
  void on_m_still_sliderReleased();
  void on_m_drag_sliderReleased();
  void on_m_tickSize_valueChanged(int);
  void on_m_tickStep_valueChanged(int);
  void on_m_labelX_editingFinished();
  void on_m_labelY_editingFinished();
  void on_m_labelZ_editingFinished();
  void on_m_focus_sliderReleased();
  void on_m_eyeSeparation_editingFinished();
  void on_m_screenWidth_valueChanged(double);
  void on_m_bgcolor_clicked();
  void on_m_newColorSet_clicked();
  void on_m_setOpacity_clicked();
  void keyPressEvent(QKeyEvent*);

 private :
  Ui::PreferencesWidget ui;

  TagColorEditor *m_tagColorEditor;

  float m_eyeSeparation;
  float m_focusDistance;
  float m_minFocusDistance;
  float m_maxFocusDistance;

  float imageQualityValue(int);
  int renderQualityValue(float);
  void setImageQualityStepsizes();  

  void showHelp();
};

#endif
