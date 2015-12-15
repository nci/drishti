#ifndef KEYFRAMEEDITOR_H
#define KEYFRAMEEDITOR_H

#include <QWidget>
#include <QPushButton>
#include "commonqtclasses.h"

class SelectRegion
{
 public :
  bool valid;
  int frame0, frame1;
  int keyframe0, keyframe1;
};

class KeyFrameEditor : public QWidget
{
  Q_OBJECT

 public :
  KeyFrameEditor(QWidget *parent=0);

  void paintEvent(QPaintEvent*);
  void resizeEvent(QResizeEvent*);

  void enterEvent(QEvent*);
  void leaveEvent(QEvent*);
  void mouseReleaseEvent(QMouseEvent*);
  void mousePressEvent(QMouseEvent*);
  void mouseMoveEvent(QMouseEvent*);
  void keyPressEvent(QKeyEvent*);

  QSize sizeHint() const;

  int startFrame();
  int endFrame();
  int currentFrame();

 signals :
  void showMessage(QString, bool);
  void setKeyFrame(int);
  void removeKeyFrame(int);
  void removeKeyFrames(int, int);
  void reorder(QList<int>);
  void setKeyFrameNumber(int, int);
  void setPlay(bool);
  void playFrameNumber(int);
  void updateGL();
  void startPlay();
  void endPlay();
  void copyFrame(int);
  void pasteFrame(int);
  void pasteFrameOnTop(int);
  void pasteFrameOnTop(int, int);
  void editFrameInterpolation(int);
  void setKeyFrameNumbers(QList<int>);
  void checkKeyFrameNumbers();

 public slots :
  void loadKeyframes(QList<int>, QList<QImage>);
  void clear();
  void setImage(int, QImage);
  void setHiresMode(bool);
  void resetCurrentFrame();
  void setPlayFrames(bool);
  void playKeyFrames(int, int, int);
  void addKeyFrameNumbers(QList<int>);
  void moveTo(int);
  void setKeyFrame();

 private slots :
  void increaseFrameStep();
  void decreaseFrameStep();
  void removeKeyFrame();
  void playKeyFrames();
  void playPressed();

 private :
  QWidget *m_parent;

  QPushButton *m_set, *m_remove;
  QPushButton *m_plus, *m_minus;
  QPushButton *m_play, *m_reset;

  SelectRegion m_selectRegion;
  QPoint m_p0, m_p1;
  int m_lineHeight, m_tickStep, m_tickHeight;
  int m_minFrame, m_maxFrame, m_frameStep;
  int m_imgSize, m_imgSpacer;
  int m_editorHeight;
  int m_prevX;
  bool m_reordered;
  int m_currFrame;
  bool m_draggingCurrFrame;
  int m_selected;
  int m_pressed;
  int m_pressedMinFrame;
  bool m_playFrames;
  bool m_hiresMode;

  int m_copyFno;
  QImage m_copyImage;

  QList<int> m_fno;
  QList<QRect> m_fRect;
  QList<QImage> m_fImage;


  int m_modifiers;
  QList<float> m_ratioBefore;
  QList<float> m_ratioAfter;

  void reorder();
  void calcRect();
  void calcMaxFrame();
  void updateSelectRegion();
  void drawSelectRegion(QPainter*);
  void drawVisibleRange(QPainter*);
  void drawTicks(QPainter*);
  void drawKeyFrames(QPainter*);
  void drawkeyframe(QPainter*, QColor, QColor, int, QRect, QImage);
  void drawCurrentFrame(QPainter*);
  int frameUnderPoint(QPoint);
  void moveCurrentFrame(QPoint); 
  void moveGrid(int);
  void preShift();
  void applyShift();
  void applyMove(int, int);

  void showHelp();
};

#endif
