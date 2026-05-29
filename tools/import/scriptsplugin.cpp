//#include "pythonengine.h"
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <cstdint>

#include "scriptsplugin.h"

#include <QtGui>
#include "common.h"

#include <math.h>

#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDir>


#if defined(Q_OS_WIN32)
#define isnan _isnan
#endif


ScriptsPlugin::ScriptsPlugin() {}

ScriptsPlugin::~ScriptsPlugin(){}

QStringList
ScriptsPlugin::registerPlugin()
{
  QStringList regString;
  regString << "script";
  regString << "Script : ";
  
  return regString;
}

bool
ScriptsPlugin::start(QString jsonflnm)
{
  QString mesg = QString("Script Plugin', 'Received JSON file: %1").arg(jsonflnm);
  py::print(mesg.toStdString());

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

		      if (key == "executable")
		        m_executable = value.toString();		
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

    mesg = QString("Executable: {%1}\nInterpreter: %2\nScript: %3").arg(m_executable).arg(m_interpreter).arg(m_script);
    py::print(mesg.toStdString());

    try {
      QString spath = QFileInfo(m_script).absolutePath();
      py::print("Script path:", spath.toStdString());
      QString ps = "import sys\nsys.path.insert(0, r\"" + spath + "\")";
      //py::print("Executing Python code:\n", ps.toStdString());
      py::exec(ps.toStdString());
      QString scriptName = QFileInfo(m_script).baseName();
      py::print("Importing module:", scriptName.toStdString());
      m_pyModule = py::module_::import(scriptName.toStdString().c_str());
      ps = "import "+scriptName+" as testmod\nprint('functions in testmod:', dir(testmod))";
      //py::print("Executing Python code:\n", ps.toStdString());
      py::exec(ps.toStdString());
    }
    catch (const std::exception& e) {
      QMessageBox::information(0, "Error", "Failed to import module: " + QString(e.what()));
      return false;
    }
    return true;
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
  uint32_t hist[65536] = {0};
  
 // Wrap C++ array as a NumPy array (no copy, shared memory)
  py::array_t<uint32_t> pyHist(
    {65536},                 // shape
    {sizeof(uint32_t)},      // stride
    hist,                    // pointer to data
    py::cast(nullptr)        // no base object, raw memory
  );
    
  
  // Call Python function
  m_pyModule.attr("get_histogram")(pyHist);
  
  // hist[] should now contain histogram
  m_histogram.clear();
  if (m_voxelType == _UChar ||
      m_voxelType == _Char)
    {
      m_histogram.reserve(256);
      for (int i=0; i<256; i++)
        m_histogram.push_back(hist[i]);
    }
  else
    {
      m_histogram.reserve(65536);
      for (int i=0; i<65536; i++)
        m_histogram.push_back(hist[i]);
    }
  
  return m_histogram;
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
}

bool
ScriptsPlugin::setFile(QStringList files)
{
  std::vector<std::string> files_vec;
  files_vec.reserve(files.size());
  for (auto& f : files) files_vec.push_back(f.toStdString());
  m_pyModule.attr("set_files")(files_vec);

  m_description = m_pyModule.attr("get_description")().cast<std::string>().c_str();
  
  std::string vu = m_pyModule.attr("get_voxel_unit")().cast<std::string>();
  if (vu == "angstrom")
    m_voxelUnit = _Angstrom;
  else if (vu == "nanometer")
    m_voxelUnit = _Nanometer;
  else if (vu == "micron")
    m_voxelUnit = _Micron;
  else if (vu == "millimeter")
    m_voxelUnit = _Millimeter;
  else if (vu == "centimeter")
    m_voxelUnit = _Centimeter;
  else if (vu == "meter")
    m_voxelUnit = _Meter;
  else if (vu == "kilometer")
    m_voxelUnit = _Kilometer;
  else if (vu == "parsec")
    m_voxelUnit = _Parsec;
  else if (vu == "kiloparsec")
    m_voxelUnit = _Kiloparsec;
  else
    m_voxelUnit = _Nounit;


  py::tuple voxel_size = m_pyModule.attr("get_voxel_size")();
  m_voxelSizeX = voxel_size[0].cast<float>();
  m_voxelSizeY = voxel_size[1].cast<float>();
  m_voxelSizeZ = voxel_size[2].cast<float>();

  m_voxelType = m_pyModule.attr("get_voxel_type")().cast<int>();
  m_skipBytes = m_headerBytes = m_pyModule.attr("get_header_bytes")().cast<int>();

  py::tuple grid_size = m_pyModule.attr("get_grid_size")();
  m_height= grid_size[0].cast<int>();
  m_width = grid_size[1].cast<int>();
  m_depth = grid_size[2].cast<int>();

  py::tuple rminmax = m_pyModule.attr("get_raw_min_max")();
  m_rawMin = rminmax[0].cast<float>();
  m_rawMax = rminmax[1].cast<float>();

  m_bytesPerVoxel = 1;
  if (m_voxelType == _UChar) m_bytesPerVoxel = 1;
  else if (m_voxelType == _Char) m_bytesPerVoxel = 1;
  else if (m_voxelType == _UShort) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Short) m_bytesPerVoxel = 2;
  else if (m_voxelType == _Int) m_bytesPerVoxel = 4;
  else if (m_voxelType == _Float) m_bytesPerVoxel = 4;
  
  {
    QString mesg;
    mesg = QString("description : %1\n").arg(m_description);
    mesg += QString("voxelunit : %1\n").arg(m_voxelUnit);
    mesg += QString("voxeltype : %1\n").arg(m_voxelType);
    mesg += QString("voxel size : %1 %2 %3\n").arg(m_voxelSizeX).arg(m_voxelSizeY).arg(m_voxelSizeZ);
    mesg += QString("bytes per voxel : %1\n").arg(m_bytesPerVoxel);
    mesg += QString("header : %1\n").arg(m_headerBytes);
    mesg += QString("dim : %1 %2 %3\n").arg(m_depth).arg(m_width).arg(m_height);		    
    mesg += QString("raw min max : %1 %2\n").arg(m_rawMin).arg(m_rawMax);		      

    py::print("\nVolume Data Information:");
    py::print(mesg.toStdString());
  }

  //----------------------------------- 

  m_fileName = files;

  return true;
}

void
ScriptsPlugin::getDepthSlice(int slc, uchar *slice)
{
  qint64 N = m_width*m_height;
  if (slc < 0 || slc >= m_depth)
    {
      memset(slice, 0, N*m_bytesPerVoxel);
      return;
    }

    
  // Wrap C++ array as a NumPy array (no copy, shared memory)
  // [slice] is modified in-place by Python function and takes care of data type conversion if needed
  // On return, `slice` contains Python-modified data
  if (m_voxelType == _UChar)
  {
    py::array_t<uint8_t> pySlice(
      {N},               // shape
      {sizeof(uint8_t)},      // stride
      slice,                  // pointer to data
      py::cast(nullptr)       // no base object, raw memory
    );
    m_pyModule.attr("get_depth_slice")(slc, pySlice);
  }
  else if (m_voxelType == _Char) 
  {
    py::array_t<char> pySlice(
      {N},            // shape
      {sizeof(char)},      // stride
      (char*)slice,             // pointer to data
      py::cast(nullptr)    // no base object, raw memory
    );
    m_pyModule.attr("get_depth_slice")(slc, pySlice);
  }
  else if (m_voxelType == _UShort)
  {
    py::array_t<ushort> pySlice(
      {N},                  // shape
      {sizeof(ushort)},   // stride
      (ushort*)slice,     // pointer to data
      py::cast(nullptr)     // no base object, raw memory
    );
    m_pyModule.attr("get_depth_slice")(slc, pySlice);
  }
  else if (m_voxelType == _Short)
  {
    py::array_t<short> pySlice(
      {N},                // shape
      {sizeof(short)},    // stride
      (short*)slice,       // pointer to data
      py::cast(nullptr)   // no base object, raw memory
    );
    m_pyModule.attr("get_depth_slice")(slc, pySlice);
  }
  else if (m_voxelType == _Int)
  {
    py::array_t<int> pySlice(
      {N},                // shape
      {sizeof(int)},      // stride
      (int*)slice,        // pointer to data
      py::cast(nullptr)   // no base object, raw memory
    );
    m_pyModule.attr("get_depth_slice")(slc, pySlice);
  }
  else if (m_voxelType == _Float)
  {
    py::array_t<float> pySlice(
      {N},                  // shape
      {sizeof(float)},      // stride
      (float*)slice,        // pointer to data
      py::cast(nullptr)     // no base object, raw memory
    );
    m_pyModule.attr("get_depth_slice")(slc, pySlice);
  }

}


QVariant
ScriptsPlugin::rawValue(int d, int w, int h)
{
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }

  py::object result = m_pyModule.attr("get_rawvalue")(d, w, h);

  if (py::isinstance<py::str>(result)) {
      py::print("string");
      std::string s = result.cast<std::string>();
      return QVariant(QString::fromStdString(s));
  }

  if (m_voxelType == _UChar) return QVariant(result.cast<uchar>());
  else if (m_voxelType == _Char) return QVariant(result.cast<char>());
  else if (m_voxelType == _UShort) return QVariant(result.cast<ushort>());
  else if (m_voxelType == _Short) return QVariant(result.cast<short>());
  else if (m_voxelType == _Int) return QVariant(result.cast<int>());
  else if (m_voxelType == _Float) return QVariant(result.cast<float>());

  return v;
}
