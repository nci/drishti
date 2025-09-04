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
			 int,
			 bool showProgress=true);


  
  static void saveToOBJ(QString,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<int>,
			bool showProgress=true);

  static void saveToOBJ(QString,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<int>,
			bool showProgress=true);

  static void saveToOBJ(QString,
			int,
			int, int);

  

  static void saveToPLY(QString,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<int>,
			bool showProgress=true);
  
  static void saveToPLY(QString,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<int>,
			bool showProgress=true);

  static void saveToPLY(QString,
			int,
			int, int,
			bool);


  
  static void saveToSTL(QString,
			QVector<QVector3D>,
			QVector<QVector3D>,
			QVector<int>,
			bool showProgress=true);

  static void saveToSTL(QString,
			int,
			int, int);
  

  
  static bool saveToTetrahedralMesh(QString,
				    QVector<QVector3D>,
				    QVector<int>,
				    bool showProgress=true);

  static bool saveToTetrahedralMesh(QString,
				    int,
				    int, int);

};

#endif
