#ifndef SCALEBAROBJECT_H
#define SCALEBAROBJECT_H

#include <QGLViewer/qglviewer.h>
using namespace qglviewer;

#include <fstream>
using namespace std;

#include "commonqtclasses.h"

class ScaleBarObject
{
 public :
  ScaleBarObject();
  ~ScaleBarObject();
  
  void load(fstream&);
  void save(fstream&);

  void clear();

  ScaleBarObject& operator=(const ScaleBarObject&);

  void set(QPointF, float, bool, bool);
  void setScaleBar(ScaleBarObject);
  void setPosition(QPointF);
  void setVoxels(float);
  void setType(bool);
  void setTextpos(bool);

  QPointF position();
  float voxels();
  bool type();
  bool textpos();
  
  void draw(float, int, int, int, int, bool);

  static ScaleBarObject interpolate(ScaleBarObject&,
				    ScaleBarObject&,
				    float);

  static QList<ScaleBarObject> interpolate(QList<ScaleBarObject>,
					   QList<ScaleBarObject>,
					   float);
  
 private :
  QPointF m_pos;  
  float m_voxels;
  bool m_type;
  bool m_textpos;
};

#endif
