#ifndef GRADIENTEDITOR_H
#define GRADIENTEDITOR_H

#include <QtGui>

class GradientEditor : public QWidget
{
  Q_OBJECT

 public :
    enum LockType {
        NoLock       = 0x00,
        LockToLeft   = 0x01,
        LockToRight  = 0x02,
        LockToTop    = 0x04,
        LockToBottom = 0x08
    };

    GradientEditor(QWidget *parent=NULL);

    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);

    void mouseMoveEvent(QMouseEvent*);
    void mouseDoubleClickEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void keyPressEvent(QKeyEvent*);
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);

    int border() const;
    void setBorder(const int);

    void setGradientStops(QGradientStops);
    QGradientStops gradientStops();

    QSizeF pointSize() const;
    void setPointSize(const QSizeF&);
    
    void setPointLock(int, LockType);
    void setGeneralLock(LockType);
    
    QRectF boundingRect() const;
    
    void setConnectionPen(const QPen&);
    void setShapePen(const QPen&);
    

 signals :
    void gradientChanged();
    void refreshGradient();

 private :
    QWidget *m_parent;

    QPolygonF m_points;
    QVector<QColor> m_colors;
    QVector<int> m_locks;
    
    int m_generalLock;

    QRectF m_bounds;

    int m_border;

    QSizeF m_pointSize;
    int m_currentIndex;
    int m_hoverIndex;

    QPen m_pointPen;
    QPen m_connectionPen;


    bool m_firedGradientChanged;
    QTimer m_timer;

    QRectF pointBoundingRect(int);
    QPointF convertLocalToWidget(QPointF);
    QPointF convertWidgetToLocal(QPointF);
    QPointF bound_point(QPointF, int);
    void movePoint(int, QPointF);
    void askGradientChoice();
    void saveGradientStops();      
    void copyGradientFile(QString);
    void flipGradientStops();      
    void scaleOpacity(float);
};

#endif
