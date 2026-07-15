#ifndef PYVERSION_H
#define PYVERSION_H

#include "pythonengine.h"
#include "pyplugininterface.h"

class PyVersion : public QObject, public PyPluginInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "drishti.import.Plugin.PyPluginInterface/1.0")
  Q_INTERFACES(PyPluginInterface)

  public :
    QString init(QString);

    void setFile(QStringList);
    void replaceFile(QString);
    QString description() { return m_description; }

    int voxelUnit() { return m_voxelUnit; }
    QList<float> voxelSize();
    int voxelType() {return m_voxelType; }

    int headerBytes() { return m_headerBytes; }
    int bytesPerVoxel() { return m_bytesPerVoxel; }
    QList<int> gridSize();
    QList<float> rawMinMax();
    QList<uint> histogram();
    void depthSlice(int, uchar*);
    QVariant rawValue(int,int, int);

  private :
    QString m_script;

    QStringList m_fileName;
    bool m_4dvol;
    int m_depth, m_width, m_height;
    int m_voxelUnit;
    int m_voxelType;
    int m_headerBytes;
    int m_bytesPerVoxel;
    float m_voxelSizeX;
    float m_voxelSizeY;
    float m_voxelSizeZ;
    QString m_description;
    
    float m_rawMin, m_rawMax;
    QList<uint> m_histogram;
    
    int m_skipBytes;

    py::object m_pyModule;
};

#endif // PYVERSION_H