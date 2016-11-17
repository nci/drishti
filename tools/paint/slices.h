#ifndef SLICES_H
#define SLICES_H

#include <QtGui>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

#include "imagewidget.h"

class Slices : public QWidget
{
  Q_OBJECT
  
 public :
  Slices(QWidget *);

  void setByteSize(int);
  void setGridSize(int, int, int);
  void setSliceType(int);
  int currSlice() { return m_slider->value(); }
  
  void setVolPtr(uchar *);
  void setMaskPtr(uchar *);

  void updateTagColors();
  void resetSliceType();
  void setBox(int, int, int, int, int, int);
  void getBox(int&, int&, int&, int&, int&, int&);
  void processPrevSliceTags();
  void loadLookupTable(QImage);

  void saveImage();

  void setRawValue(QList<int>);

  void reloadSlice();

  void setModeType(int);

  void zoomToSelection() { m_imageWidget->zoom9Clicked(); }
 signals :
  void changeLayout();
  void sliceChanged(int);
  void xPos(int);
  void yPos(int);
  void updateBB(Vec, Vec);
  void saveWork();

  void getRawValue(int, int, int);
  void viewerUpdate();
  void tagDSlice(int, uchar*);
  void tagWSlice(int, uchar*);
  void tagHSlice(int, uchar*);

  void applyMaskOperation(int, int, int);
  void updateViewerBox(int, int, int, int, int, int);

  void saveMask();

  void shrinkwrap(Vec, Vec,
		  int, bool, int,
		  bool,
		  int, int, int,
		  int);
  void connectedRegion(int, int, int,
		       Vec, Vec,
		       int, int);

 public slots :
   void setHLine(int);
   void setVLine(int);
   void setSlice(int);
   //void bbupdated(Vec, Vec);
   void fitImage() { m_imageWidget->zoom9Clicked(); }

 private:
    ImageWidget *m_imageWidget;
    
    QPushButton *m_zoom0;
    QPushButton *m_zoom9;
    QPushButton *m_zoomUp;
    QPushButton *m_zoomDown;
    QPushButton *m_changeLayout;

    QLabel *m_mesg;

    QSlider *m_slider;

    void createMenu(QHBoxLayout*, QVBoxLayout*);
};



#endif
