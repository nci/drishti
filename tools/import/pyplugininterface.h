#ifndef PYPLUGININTERFACE_H
#define PYPLUGININTERFACE_H

#include <QObject>
#include <QString>
#include <QList>
#include <QVariant>

class PyPluginInterface
{
  public :
    virtual ~PyPluginInterface() {}

    virtual QString init(QString) = 0;

    virtual void setFile(QStringList) = 0;
    virtual void replaceFile(QString) = 0;
    virtual QString description() = 0;

    virtual int voxelUnit() = 0;
    virtual QList<float> voxelSize() = 0;
    virtual int voxelType() = 0;

    virtual int headerBytes() = 0;
    virtual int bytesPerVoxel() = 0;
    virtual QList<int> gridSize() = 0;
    virtual QList<float> rawMinMax() = 0;
    virtual QList<uint> histogram() = 0;
    virtual void depthSlice(int, uchar*) = 0;
    virtual QVariant rawValue(int, int, int) = 0;
};

Q_DECLARE_INTERFACE(PyPluginInterface,
		    "drishti.import.Plugin.PyPluginInterface/1.0")

#endif // PYPLUGININTERFACE_H