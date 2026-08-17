#include "scriptsplugin.h"
#include <cstdint>


#include <QtGui>
#include "common.h"

#include <math.h>
#include <iostream>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDir>


#if defined(Q_OS_WIN32)
#define isnan _isnan
#endif


ScriptsPlugin::ScriptsPlugin() 
{
  m_plugin = nullptr;
}

ScriptsPlugin::~ScriptsPlugin()
{
  if (m_plugin)
    delete m_plugin;
      
  m_plugin = nullptr;
}

QStringList
ScriptsPlugin::registerPlugin()
{
  QStringList regString;
  regString << "script";
  regString << "Script : ";
  
  return regString;
}

bool
ScriptsPlugin::start(QString pyver, QString pydir, QString jsonflnm)
{
  QString mesg = QString("Script Plugin', 'Received JSON file: %1").arg(jsonflnm);
  //std::cout << mesg.toStdString() << "\n";

  m_jsonflnm = jsonflnm;
  QString m_scriptDir = QFileInfo(m_jsonflnm).absolutePath();
  
  QFile fl(m_jsonflnm);
  if (fl.open(QIODevice::ReadOnly))
	{
	  QByteArray bytes = fl.readAll();
	  fl.close();
  
	  QJsonParseError jsonError;
	  QJsonDocument document = QJsonDocument::fromJson( bytes, &jsonError );
	  if (jsonError.error != QJsonParseError::NoError )
	    {
	      QMessageBox::information(0, "Error",
				       QString("fromJson failed: %1").	\
				       arg(jsonError.errorString()));
	      return false;
	    }

	  if (document.isObject() )
	    {
	      QJsonObject jsonObj = document.object(); 
	      QStringList keys = jsonObj.keys();
	      for (auto key : keys)
		    {
		      auto value = jsonObj.take(key);
	
		      if (key == "interpreter")
		        m_interpreter = value.toString();
		      if (key == "script")
            m_script = m_scriptDir + QDir::separator() + value.toString();
		    }	  
	    }
	  else
	    {
	      QMessageBox::information(0, "Error", "Error in json "+m_jsonflnm);
	      return false;
	    }

    if (m_interpreter.isEmpty() || m_script.isEmpty())
      {
        QMessageBox::information(0, "Error", "Interpreter or script not specified in JSON file.");
        return false;
      }
    if (m_interpreter != "python")
      {
        QMessageBox::information(0, "Error", "Only Python interpreter is supported currently.");
        return false;
      } 


    //-----------------------------------    
    QString pythonDir, dllPath, libPath, scriptsPath;
    QString newPath;
    //if (pyver.contains("3.10"))
    //  pythonDir = "C:/Apps/Python310";
    //if (pyver.contains("3.11"))
    //  pythonDir = "C:/Apps/Python311";
    //if (pyver.contains("3.12"))
    //  pythonDir = "C:/Apps/Python312";
    //if (pyver.contains("3.14"))
    //  pythonDir = "C:/Apps/Python314";
    pythonDir = pydir;
        
    dllPath = pythonDir + "/DLLs";
    libPath = pythonDir + "/Lib";
    scriptsPath = pythonDir + "/Scripts";
    newPath = pythonDir + ";" + dllPath + ";" + scriptsPath + QString::fromLocal8Bit(qgetenv("PATH"));
    
    qputenv("PATH", newPath.toLocal8Bit());
    qputenv("PYTHONHOME", pythonDir.toLocal8Bit());
    qputenv("PYTHONPATH", libPath.toLocal8Bit());
    std::cout << "\n\n\n----------------\n";
    std::cout << " Using Python : " << qgetenv("PYTHONHOME").data() << "\n";
    std::cout << "----------------\n";
    //-----------------------------------

    
    m_loader.setFileName(pyver);

    m_pluginInstance = m_loader.instance();
    if (m_pluginInstance)
    {
        m_plugin = qobject_cast<PyPluginInterface*>(m_pluginInstance);
        if (m_plugin)
        {
          QString mesg = m_plugin->init(m_script);
          if (mesg != "true")
            {
              QMessageBox::information(0, "Error", "Failed to import module: " + mesg);
              return false;
            }
          else
            {  
              std::cout << "\n** Script initialization completed : " 
                        << m_script.toStdString()
                        << "\n";
              return true;
            }
        }
	else
	{
	  QMessageBox::information(0, "Error", "Cannot load the plugin");
	  std::cout << "\n ** Cannot load the plugin. **\n";
	}
    }
    
    QMessageBox::information(nullptr, "Plugin", "Failed to load python version : " + pyver + "\nfor script: " + m_script);
    std::cout << "** Failed to load python version - " << pyver.toLatin1().data() << "\n";

    return false;
  }
  return false;
}



void
ScriptsPlugin::init()
{
  m_fileName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
}

void
ScriptsPlugin::clear()
{
  m_fileName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_4dvol = false;
}

void
ScriptsPlugin::set4DVolume(bool flag)
{
  m_4dvol = flag;
}

void
ScriptsPlugin::voxelSize(float& vx, float& vy, float& vz)
  {
    vx = m_voxelSizeX;
    vy = m_voxelSizeY;
    vz = m_voxelSizeZ;
  }
QString ScriptsPlugin::description() { return m_description; }
int ScriptsPlugin::voxelType() { return m_voxelType; }
int ScriptsPlugin::voxelUnit() { return m_voxelUnit; }
int ScriptsPlugin::headerBytes() { return m_headerBytes; }

void
ScriptsPlugin::setMinMax(float rmin, float rmax)
{
  m_rawMin = rmin;
  m_rawMax = rmax;
}

float ScriptsPlugin::rawMin() { return m_rawMin; }
float ScriptsPlugin::rawMax() { return m_rawMax; }


QList<uint>
ScriptsPlugin::histogram()
{
  return m_plugin->histogram();
}

void
ScriptsPlugin::gridSize(int& d, int& w, int& h)
{
  d = m_depth;
  w = m_width;
  h = m_height;
}

void
ScriptsPlugin::replaceFile(QString flnm)
{
  m_fileName.clear();
  m_fileName << flnm;
  m_plugin->replaceFile(flnm);
}

bool
ScriptsPlugin::setFile(QStringList files)
{
  m_plugin->setFile(files);


  m_fileName = files;
  
  m_description = m_plugin->description();

  m_voxelUnit = m_plugin->voxelUnit();
  m_voxelType = m_plugin->voxelType();


  QList<float> vxyz = m_plugin->voxelSize();
  m_voxelSizeX = vxyz[0];
  m_voxelSizeY = vxyz[1];
  m_voxelSizeZ = vxyz[2];


  QList<int> nxyz = m_plugin->gridSize();
  m_depth = nxyz[0];
  m_width = nxyz[1];
  m_height = nxyz[2];


  QList<float> vxy = m_plugin->rawMinMax();
  m_rawMin = vxy[0];
  m_rawMax = vxy[1];


  m_headerBytes = m_skipBytes = m_plugin->headerBytes();
  m_bytesPerVoxel = m_plugin->bytesPerVoxel();


  return true;
}

void
ScriptsPlugin::getDepthSlice(int slc, uchar *slice)
{
  return m_plugin->depthSlice(slc, slice);
}


QVariant
ScriptsPlugin::rawValue(int d, int w, int h)
{
  return m_plugin->rawValue(d,w,h);
}
