#ifndef GRIDOBJECT_H
#define GRIDOBJECT_H

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

class GridObjectUndo
{
 public :
  GridObjectUndo();
  ~GridObjectUndo();
  
  void clear();
  void append(int, int, QList<Vec>);

  void undo();
  void redo();

  void colrow(int&, int&);
  QList<Vec> points();

 private :
  int m_index;
  QList< QList<Vec> > m_points;
  QList<int> m_cols;
  QList<int> m_rows;

  void clearTop();
};

class GridObject
{
 public :
  GridObject();
  ~GridObject();
  
  void load(fstream&);
  void save(fstream&);

  void set(const GridObject&);  
  GridObject get();

  GridObject& operator=(const GridObject&);

  void undo();
  void redo();
  void updateUndo();

  bool showPointNumbers();
  bool showPoints();
  Vec color();
  float opacity();
  QList<Vec> points();
  int columns();
  int rows();

  Vec getPoint(int);
  void setPoint(int, Vec);

  void regrid();
  QList<Vec> gridPoints();
  QList< QPair<Vec, Vec> > getPointsAndNormals();

  void setShowPointNumbers(bool);
  void setShowPoints(bool);
  void setColor(Vec);
  void setOpacity(float);
  void normalize();
  void setColRow(int, int);
  void setPoints(QList<Vec>);

  void setPointPressed(int);
  int getPointPressed();

  void draw(QGLViewer*, bool, bool, Vec);
  void postdraw(QGLViewer*, int, int, bool);

  void insertRow(int);
  void insertCol(int);
  void removeRow(int);
  void removeCol(int);

 private :
  GridObjectUndo m_undo;

  int m_cols, m_rows;
  bool m_showPointNumbers;
  bool m_showPoints;
  Vec m_color;
  float m_opacity;
  QList<Vec> m_points;
  QList<Vec> m_grid;
  int m_pointPressed;
  bool m_updateFlag;
  
  void drawLines(QGLViewer*, bool, bool);

  void postdrawPointNumbers(QGLViewer*);
  void postdrawGrab(QGLViewer*, int, int);
};

#endif
