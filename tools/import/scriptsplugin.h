#ifndef SCRIPTSPLUGIN_H
#define SCRIPTSPLUGIN_H

#include "pythonengine.h"

#include "commonqtclasses.h"

namespace py = pybind11;


class ScriptsPlugin
{
  public :
    ScriptsPlugin();
    ~ScriptsPlugin();
    
    QStringList registerPlugin();
    
    bool start(QString);

    void init();
    void clear();

    void setValue(QString, float);
    void setValue(QString, QString);
      
    void set4DVolume(bool);
    
    bool setFile(QStringList);
    void replaceFile(QString);
    
    void gridSize(int&, int&, int&);
    void voxelSize(float&, float&, float&);
    QString description();
    int voxelUnit();
    int voxelType();
    int headerBytes();
    
    QList<uint> histogram();
    
    void setMinMax(float, float);
    float rawMin();
    float rawMax();
     
    void getDepthSlice(int, uchar*);
    
    QVariant rawValue(int, int, int);
    
  private :
    QString m_jsonflnm;
    QString m_script;
    QString m_interpreter;
    
    QStringList m_fileName;
    bool m_4dvol;
    int m_depth, m_width, m_height;
    int m_voxelUnit;
    int m_voxelType;
    int m_headerBytes;
    float m_voxelSizeX;
    float m_voxelSizeY;
    float m_voxelSizeZ;
    QString m_description;
    
    float m_rawMin, m_rawMax;
    QList<uint> m_histogram;
    
    int m_skipBytes;
    int m_bytesPerVoxel;

    py::object m_pyModule;
};

#endif
