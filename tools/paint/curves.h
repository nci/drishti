#ifndef CURVES_H
#define CURVES_H

#include <QtGui>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QLineEdit>

#include "curveswidget.h"
#include "myslider.h"

class Curves : public QWidget
{
  Q_OBJECT
  
 public :
  Curves(QWidget*, QStatusBar*);

  void setInFocus() { m_curvesWidget->setInFocus(); }
  bool inFocus() { return m_curvesWidget->inFocus(); }
  
  void setByteSize(int);
  void setGridSize(int, int, int);
  void setSliceType(int);
  void resetSliceType();
  int currSlice() { return m_slider->value(); }
  int currentSliceNumber() { return m_curvesWidget->currentSliceNumber(); }
  
  void setVolPtr(uchar *);
  //void setMaskPtr(uchar *);

  void setBox(int, int, int, int, int, int);
  void getBox(int&, int&, int&, int&, int&, int&);
  void loadLookupTable(QImage);
  
  void setImage(uchar*, uchar*);
  void setMaskImage(uchar*);
  
  void setRawValue(QList<int>);

  void setModeType(int);

  void setLarge(bool);
  bool enlarged() { return m_maximized; }

  void setSliderValue(int slc) { m_slider->setValue(slc); }
  void setSliderRange(int, int, int, int, int);
  
  void saveCurves();
  void saveCurves(QString);
  void loadCurves();
  void loadCurves(QString);

  void setCurve(bool);
  void setLivewire(bool);

  void freezeLivewire(bool);
  void sliceChanged();

  void depthUserRange(int&, int&);
  void widthUserRange(int&, int&);
  void heightUserRange(int&, int&);

  void resetCurves();

  void setShowPosition(bool);
  
  void setGradThresholdType(int);
  void setMinGrad(float);
  void setMaxGrad(float);

  void setSliceLOD(int);
  void setPointSize(int);
  void setGradType(int);
  void setSmoothType(int);
  void newCurve();
  void endCurve();
  void morphCurves();
  void deleteAllCurves();

  bool curvesPresent();
  //bool dCurvesPresent();
  //bool wCurvesPresent();
  //bool hCurvesPresent();
  
  void paintUsingCurves(int, int, int, int, uchar*);

 signals :
  void changeLayout();
  void xPos(int);
  void yPos(int);

  void saveWork();
  void viewerUpdate();
  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);
  void showEndCurve();
  void hideEndCurve();
  void getSlice(int);
  void getRawValue(int, int, int);

  void updateViewerBox(int, int, int, int, int, int);

  void setPropagation(bool);
  void saveMask();

  void gotFocus();
		 
 public slots :
   void updateTagColors();
   void setHLine(int);
   void setVLine(int);
   void setSlice(int);

   void releaseFocus();
   void setSliceNumber(int);
   void updateSliderValue(int slc) { m_slider->updateValue(slc); }
   
   void sliceNumChanged();
   
   void fitImage() { m_curvesWidget->zoom9Clicked(); }

   void zoomToSelection() { m_curvesWidget->zoom9Clicked(); }
  
  
 private:
    CurvesWidget *m_curvesWidget;
    
    QPushButton *m_zoom0;
    QPushButton *m_zoom9;
    QPushButton *m_zoomUp;
    QPushButton *m_zoomDown;
    QPushButton *m_changeLayout;

    QLineEdit *m_sliceNum;

    MySlider *m_slider;

    bool m_maximized;

    int m_Depth, m_Width, m_Height;
    int m_s0, m_s1;
  
    void createMenu(QHBoxLayout*, QVBoxLayout*);
};



#endif
