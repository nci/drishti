#ifndef MESHTOOLS_H
#define MESHTOOLS_H

#include <QVector>
#include <QVector3D>

class MeshTools
{
 public :
  static void smoothMesh(QVector<QVector3D>&,
			 QVector<QVector3D>&,
			 QVector<int>&,
			 int);
    
  static void saveToOBJ(QString,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<int>);

  static void saveToPLY(QString flnm,
			QVector<QVector3D> V,
			QVector<QVector3D> N,
			QVector<int> T);
  
  static void saveToPLY(QString flnm,
			QVector<QVector3D> V,
			QVector<QVector3D> N,
			QVector<QVector3D> C,
			QVector<int> T);

  static void saveToSTL(QString flnm,
			QVector<QVector3D> V,
			QVector<QVector3D> N,
			QVector<int> T);
};

#endif
