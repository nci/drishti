#ifndef NETWORKOBJECT_H
#define NETWORKOBJECT_H

#include <QFile>

#include "networkinformation.h"
#include "cropobject.h"
#include <QDomDocument>

class NetworkObject
{
 public :
  NetworkObject();
  ~NetworkObject();

  void gridSize(int&, int&, int&);

  bool show() { return m_show; }
  void setShow(bool s) { m_show = s; }

  Vec centroid() { return m_tcentroid; }
  void enclosingBox(Vec&, Vec&);

  QString filename() { return m_fileName; }

  QString vertexAttributeString(int);
  QString edgeAttributeString(int);

  int vertexAttributeCount() { return m_vertexAttribute.count(); }
  int edgeAttributeCount() { return m_edgeAttribute.count(); }

  int vertexAttribute() { return m_Vatt; }
  void setVertexAttribute(int va) { if (va < m_vertexAttribute.count()) m_Vatt = va; }

  int edgeAttribute() { return m_Eatt; }
  void setEdgeAttribute(int ea) { if (ea < m_edgeAttribute.count()) m_Eatt = ea; }

  bool vMinmax(int, float&, float&);
  void setVminmax(int, float, float);

  bool eMinmax(int, float&, float&);
  void setEminmax(int, float, float);

  bool userVminmax(int, float&, float&);
  void setUserVminmax(int, float, float);
  void setUserVminmax(float, float);

  bool userEminmax(int, float&, float&);
  void setUserEminmax(int, float, float);
  void setUserEminmax(float, float);

  float vopacity() { return m_Vopacity; }
  void setVOpacity(float op) { m_Vopacity = op; }

  float eopacity() { return m_Eopacity; }
  void setEOpacity(float op) { m_Eopacity = op; }

  QGradientStops vstops() { return m_Vstops; }
  void setVstops(QGradientStops stops);

  QGradientStops estops() { return m_Estops; }
  void setEstops(QGradientStops stops);

  bool load(QString);
  bool save(QString);

  bool set(NetworkInformation);
  NetworkInformation get();

  bool fromDomElement(QDomElement);
  QDomElement domElement(QDomDocument&);

  void clear();

  void predraw(QGLViewer*,
	       double*,
	       Vec,
	       QList<Vec>, QList<Vec>,
	       QList<CropObject>,
	       Vec);
  void draw(QGLViewer*,
	    bool,
	    float, float,
	    bool);
  void postdraw(QGLViewer*,
		int, int, bool, int);

  float scaleE();
  float scaleV();
  void setScale(float);
  void setScaleE(float);
  void setScaleV(float);
 private :
  bool m_show;

  QString m_fileName;
  int m_nX, m_nY, m_nZ;
  Vec m_centroid;
  Vec m_enclosingBox[8];
  float m_Vopacity;
  float m_Eopacity;
  int m_Vatt, m_Eatt;
  QVector< QPair<float, float> > m_Vminmax;
  QVector< QPair<float, float> > m_Eminmax;
  QVector< QPair<float, float> > m_userVminmax;
  QVector< QPair<float, float> > m_userEminmax;

  QGradientStops m_Vstops;
  QGradientStops m_resampledVstops;
  QGradientStops m_Estops;
  QGradientStops m_resampledEstops;

  Vec m_tcentroid;
  Vec m_tenclosingBox[8];

  QVector<Vec> m_Evertices;
  QVector<Vec> m_Eovertices;
  QVector<Vec> m_Eocolor;
  QVector<float> m_EtexValues;

  QVector<Vec> m_Vvertices;
  QVector<Vec> m_Vovertices;
  QVector<Vec> m_Vocolor;
  QVector<float> m_VtexValues;


  //-------------------
  float m_scaleV, m_scaleE;
  int m_vertexRadiusAttribute;
  int m_edgeRadiusAttribute;
  QVector< QPair<QString, QVector<float> > > m_vertexAttribute;
  QVector< QPair<QString, QVector<float> > > m_edgeAttribute;
  QVector<Vec> m_vertexCenters;
  QVector< QPair<int, int> > m_edgeNeighbours;
  QStringList m_nodeId;
  QStringList m_nodeAtt;
  QStringList m_edgeAtt;
  //-------------------


  void drawNetwork(float, float, bool);
  void drawVertices(float, float);
  void drawEdges(float, float);

  void predrawVertices(QGLViewer*,
		       double*,
		       Vec,
		       QList<Vec>, QList<Vec>,
		       QList<CropObject>);

  void predrawEdges(QGLViewer*,
		    double*,
		    Vec,
		    QList<Vec>, QList<Vec>,
		    QList<CropObject> );

  void generateSphereSpriteTexture(Vec);
  void generateCylinderSpriteTexture(Vec);

  bool loadTextNetwork(QString);
  bool loadGraphML(QString);
  bool loadNetCDF(QString);

  void loadEdgeInfo(QDomNodeList);
  void loadNodeInfo(QDomNodeList);
  void getKey(QDomElement);
};

#endif
