#ifndef RAWPLUGIN_H
#define RAWPLUGIN_H

#include <QObject>
#include "volinterface.h"
#include <QProcess>
#include <QUdpSocket>
#include <QTcpSocket>
#include <QProcess>
#include <QFile>

class ScriptsPlugin : public QObject, VolInterface
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "drishti.import.Plugin.VolInterface/1.0")
  Q_INTERFACES(VolInterface)

  public :  
    ~ScriptsPlugin();
    
    QStringList registerPlugin();
    
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
    QProcess m_process;
    QUdpSocket m_sendingSocket;
    QUdpSocket m_listeningSocket;
    QFile m_sharedFile;
  
  
    QString m_jsonflnm;
    QString m_script;
    QString m_executable;
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
};

#endif
