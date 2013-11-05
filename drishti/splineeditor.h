#ifndef SPLINE_EDITOR_H
#define SPLINE_EDITOR_H

#include <QtGui>
#include "splinetransferfunction.h"

class SplineEditor : public QWidget
{
  Q_OBJECT

 public :
    enum PointShape {
        CircleShape,
        RectangleShape
    };

    enum LockType {
        LockToLeft   = 0x01,
        LockToRight  = 0x02,
        LockToTop    = 0x04,
        LockToBottom = 0x08
    };

    enum SortType {
        NoSort,
        XSort,
        YSort
    };

    enum ConnectionType {
        NoConnection,
        LineConnection,
        CurveConnection
    };


    SplineEditor(QWidget*);

    QImage colorMapImage();

    void paintEvent(QPaintEvent*);
    void resizeEvent(QResizeEvent*);

    void mouseMoveEvent(QMouseEvent*);
    void mouseReleaseEvent(QMouseEvent*);
    void mousePressEvent(QMouseEvent*);
    void keyPressEvent(QKeyEvent*);
    void enterEvent(QEvent*);
    void leaveEvent(QEvent*);

    int border() const;
    void setBorder(const int);

    QSizeF pointSize() const;
    void setPointSize(const QSizeF&);
    
    QRectF boundingRect() const;
    
    ConnectionType connectionType();
    void setConnectionType(ConnectionType);
    
    void setConnectionPen(const QPen&);
    void setShapePen(const QPen&);
    void setShapeBrush(const QBrush&);

    void setHistogramImage(QImage, QImage);
    void setTransferFunction(SplineTransferFunction*);
    void setMapping(QPolygonF);

    void setGradLimits(int, int);
    void setOpmod(float, float);

    void show2D(bool);
    
    bool set16BitPoint(float, float);

 public slots :
    void setGradientStops(QGradientStops);

 private slots :
    void saveTransferFunctionImage_cb();

 signals :
    void refreshDisplay();
    void splineChanged();
    void selectEvent(QGradientStops);
    void deselectEvent();
    void applyUndo(bool);

 private :
    QWidget *m_parent;
    SplineTransferFunction *m_splineTF;

    bool m_showGrid;
    bool m_showOverlay;
    bool m_show2D;
    QImage m_histogramImage1D;
    QImage m_histogramImage2D;

    QPolygonF m_mapping;

    QRectF m_bounds;
    PointShape m_pointShape;
    ConnectionType m_connectionType;

    int m_border;
    bool m_dragging;
    bool m_moveSpine;

    QSizeF m_pointSize;
    int m_currentIndex;
    int m_prevCurrentIndex;
    int m_normalIndex;
    int m_hoverIndex;

    QPen m_pointPen;
    QPen m_connectionPen;
    QBrush m_pointBrush;

    QRectF pointBoundingRect(QPointF, int);
    QPointF convertLocalToWidget(QPointF);
    QPointF convertWidgetToLocal(QPointF);
    int checkNormalPressEvent(QMouseEvent*);
    void getPainterPath(QPainterPath*, QPolygonF);

    void paintPatchLines(QPainter*);
    void paintPatchEndPoints(QPainter*);
    void paintUnderlay(QPainter*);
    void paintOverlay(QPainter*);

    QAction *save_transferfunction_image_action;

    void showHelp();
};

#endif
