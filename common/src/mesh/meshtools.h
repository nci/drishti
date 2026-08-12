#ifndef MESHTOOLS_H
#define MESHTOOLS_H

#include <QString>
#include <QVector>
#include <QVector3D>

class MeshTools
{
 public:
  static void smoothMesh(QVector<QVector3D>&,
                         QVector<QVector3D>&,
                         QVector<int>&,
                         int,
                         bool showProgress=true);

  static bool saveToOBJ(QString,
                        QVector<QVector3D>,
                        QVector<QVector3D>,
                        QVector<int>,
                        bool showProgress=true);

  static bool saveToOBJ(QString,
                        QVector<QVector3D>,
                        QVector<QVector3D>,
                        QVector<QVector3D>,
                        QVector<int>,
                        bool showProgress=true);

  static bool saveToOBJ(QString,
                        int,
                        int, int);

  static bool saveToPLY(QString,
                        QVector<QVector3D>,
                        QVector<QVector3D>,
                        QVector<int>,
                        bool showProgress=true);

  static bool saveToPLY(QString,
                        QVector<QVector3D>,
                        QVector<QVector3D>,
                        QVector<QVector3D>,
                        QVector<int>,
                        bool showProgress=true,
                        bool binary=true);

  static bool saveToPLY(QString,
                        int,
                        int, int,
                        bool);

  static bool saveToSTL(QString,
                        QVector<QVector3D>,
                        QVector<QVector3D>,
                        QVector<int>,
                        bool showProgress=true);

  static bool saveToSTL(QString,
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
