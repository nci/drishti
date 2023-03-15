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

  static void saveToOBJ(QString,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<int>);

  static void saveToPLY(QString,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<int>);
  
  static void saveToPLY(QString,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<int>);

  static void saveToSTL(QString,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<int>);
};

#endif
