#include "scriptsplugin.h"
#include <cstdint>


#include <QtGui>
#include "common.h"

#include <math.h>

#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDir>

#include <cstring>
#include <limits>
#include <new>
#include <stdexcept>
#include <vector>


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
  //py::print(mesg.toStdString());

  m_jsonflnm = jsonflnm;
  m_interpreter.clear();
  m_script.clear();
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

    try {
      PythonEngine::instance();
      QString spath = QFileInfo(m_script).absolutePath();
      py::print("Script path:", spath.toStdString());
      py::module_ sys = py::module_::import("sys");
      sys.attr("path").attr("insert")(0, spath.toStdString());
      QString scriptName = QFileInfo(m_script).baseName();
      py::print("Importing module:", scriptName.toStdString());
      m_pyModule = py::module_::import(scriptName.toStdString().c_str());
      py::object availableFunctions =
        py::module_::import("builtins").attr("dir")(m_pyModule);
      py::print("Available functions :", availableFunctions);

      //QMessageBox::information(0, "Success", "Successfully imported module: " + scriptName);
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
  clear();
}

void
ScriptsPlugin::clear()
{
  try
    {
      if (m_pyModule.ptr() && py::hasattr(m_pyModule, "close"))
	m_pyModule.attr("close")();
    }
  catch (const std::exception &e)
    {
      qWarning() << "Cannot close Python volume mapping:" << e.what();
    }

  m_fileName.clear();
  m_description.clear();
  m_depth = m_width = m_height = 0;
  m_voxelType = _UChar;
  m_voxelUnit = _Micron;
  m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;
  m_skipBytes = 0;
  m_headerBytes = 0;
  m_bytesPerVoxel = 1;
  m_rawMin = m_rawMax = 0;
  m_histogram.clear();
  m_lastError.clear();
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
  m_lastError.clear();
  std::vector<uint> hist(65536, 0);

  try
    {
      // Wrap C++ array as a NumPy array (no copy, shared memory).
      py::array_t<uint> pyHist(
	{65536},
	{sizeof(uint)},
	hist.data(),
	py::cast(nullptr));

      m_pyModule.attr("get_histogram")(pyHist);
    }
  catch (const std::exception &e)
    {
      m_lastError = QString("Cannot read the script histogram: %1")
	.arg(e.what());
      return QList<uint>();
    }
  
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

bool
ScriptsPlugin::replaceFile(QString flnm)
{
  if (flnm.isEmpty())
    {
      m_lastError = "The replacement volume filename is empty.";
      return false;
    }
  return setFile(QStringList() << flnm);
}

bool
ScriptsPlugin::setFile(QStringList files)
{
  m_lastError.clear();
  if (files.isEmpty())
    {
      m_lastError = "No volume file was selected.";
      return false;
    }

  const QStringList previousFiles = m_fileName;
  bool scriptStateChanged = false;
  bool transactionalSetFiles = false;
  try
    {
      std::vector<std::string> files_vec;
      files_vec.reserve(files.size());
      for (const auto &f : files)
	files_vec.push_back(f.toStdString());

      if (py::hasattr(m_pyModule, "SET_FILES_TRANSACTIONAL"))
        transactionalSetFiles =
          m_pyModule.attr("SET_FILES_TRANSACTIONAL").cast<bool>();
      m_pyModule.attr("set_files")(files_vec);
      scriptStateChanged = true;

      const QString candidateDescription = QString::fromStdString(
	m_pyModule.attr("get_description")().cast<std::string>());

      std::string vu = m_pyModule.attr("get_voxel_unit")().cast<std::string>();
      int candidateVoxelUnit = _Nounit;
      if (vu == "angstrom")
	candidateVoxelUnit = _Angstrom;
      else if (vu == "nanometer")
	candidateVoxelUnit = _Nanometer;
      else if (vu == "micron")
	candidateVoxelUnit = _Micron;
      else if (vu == "millimeter")
	candidateVoxelUnit = _Millimeter;
      else if (vu == "centimeter")
	candidateVoxelUnit = _Centimeter;
      else if (vu == "meter")
	candidateVoxelUnit = _Meter;
      else if (vu == "kilometer")
	candidateVoxelUnit = _Kilometer;
      else if (vu == "parsec")
	candidateVoxelUnit = _Parsec;
      else if (vu == "kiloparsec")
	candidateVoxelUnit = _Kiloparsec;

      py::tuple voxel_size = m_pyModule.attr("get_voxel_size")();
      if (voxel_size.size() != 3)
	throw std::runtime_error("get_voxel_size() must return three values");
      const float candidateVoxelSizeX = voxel_size[0].cast<float>();
      const float candidateVoxelSizeY = voxel_size[1].cast<float>();
      const float candidateVoxelSizeZ = voxel_size[2].cast<float>();

      const int candidateVoxelType =
        m_pyModule.attr("get_voxel_type")().cast<int>();
      const int candidateHeaderBytes =
	m_pyModule.attr("get_header_bytes")().cast<int>();

      py::tuple grid_size = m_pyModule.attr("get_grid_size")();
      if (grid_size.size() != 3)
	throw std::runtime_error("get_grid_size() must return three values");
      const int candidateHeight = grid_size[0].cast<int>();
      const int candidateWidth = grid_size[1].cast<int>();
      const int candidateDepth = grid_size[2].cast<int>();

      py::tuple rminmax = m_pyModule.attr("get_raw_min_max")();
      if (rminmax.size() != 2)
	throw std::runtime_error("get_raw_min_max() must return two values");
      const float candidateRawMin = rminmax[0].cast<float>();
      const float candidateRawMax = rminmax[1].cast<float>();

      int candidateBytesPerVoxel = 0;
      if (candidateVoxelType == _UChar || candidateVoxelType == _Char)
	candidateBytesPerVoxel = 1;
      else if (candidateVoxelType == _UShort || candidateVoxelType == _Short)
	candidateBytesPerVoxel = 2;
      else if (candidateVoxelType == _Int || candidateVoxelType == _Float)
	candidateBytesPerVoxel = 4;
      else
	throw std::runtime_error("the script returned an unsupported voxel type");

      if (candidateDepth <= 0 || candidateWidth <= 0 ||
          candidateHeight <= 0 || candidateHeaderBytes < 0)
	throw std::runtime_error("the script returned invalid volume dimensions or header size");

      const quint64 width = static_cast<quint64>(candidateWidth);
      const quint64 height = static_cast<quint64>(candidateHeight);
      const quint64 bytesPerVoxel =
        static_cast<quint64>(candidateBytesPerVoxel);
      if (width > std::numeric_limits<quint64>::max()/height ||
	  width*height > std::numeric_limits<quint64>::max()/bytesPerVoxel ||
	  width*height*bytesPerVoxel >
	    static_cast<quint64>(std::numeric_limits<size_t>::max()))
	throw std::runtime_error("the script slice size exceeds the process address space");

      m_description = candidateDescription;
      m_voxelUnit = candidateVoxelUnit;
      m_voxelSizeX = candidateVoxelSizeX;
      m_voxelSizeY = candidateVoxelSizeY;
      m_voxelSizeZ = candidateVoxelSizeZ;
      m_voxelType = candidateVoxelType;
      m_skipBytes = m_headerBytes = candidateHeaderBytes;
      m_height = candidateHeight;
      m_width = candidateWidth;
      m_depth = candidateDepth;
      m_rawMin = candidateRawMin;
      m_rawMax = candidateRawMax;
      m_bytesPerVoxel = candidateBytesPerVoxel;
      m_fileName = files;
      return true;
    }
  catch (const std::exception &e)
    {
      QString error = QString("Cannot open the selected volume:\n%1")
	.arg(e.what());

      if (!previousFiles.isEmpty() &&
          (scriptStateChanged || !transactionalSetFiles))
        {
          try
            {
              std::vector<std::string> previousFilesVector;
              previousFilesVector.reserve(previousFiles.size());
              for (const QString &fileName : previousFiles)
                previousFilesVector.push_back(fileName.toStdString());
              m_pyModule.attr("set_files")(previousFilesVector);
            }
          catch (const std::exception &rollbackError)
            {
              error += QString("\nThe previous Python volume state could not "
                               "be restored: %1").arg(rollbackError.what());
            }
        }
      else if (previousFiles.isEmpty() && scriptStateChanged &&
               py::hasattr(m_pyModule, "close"))
        {
          try
            {
              m_pyModule.attr("close")();
            }
          catch (...)
            {
            }
        }

      m_lastError = error;
      return false;
    }
}

bool
ScriptsPlugin::getDepthSlice(int slc, uchar *slice)
{
  m_lastError.clear();
  const qint64 N = static_cast<qint64>(m_width)*m_height;
  if (!slice || N <= 0 ||
      static_cast<quint64>(N) >
	static_cast<quint64>(std::numeric_limits<size_t>::max())/
	static_cast<quint64>(qMax(1, m_bytesPerVoxel)))
    {
      m_lastError = "The Python volume slice buffer size is invalid.";
      return false;
    }

  if (slc < 0 || slc >= m_depth)
    {
      std::memset(slice, 0,
	static_cast<size_t>(N)*static_cast<size_t>(m_bytesPerVoxel));
      m_lastError = QString("Slice index %1 is outside [0, %2).")
	.arg(slc).arg(m_depth);
      return false;
    }

  //QMessageBox::information(0, "Info", QString("Getting depth slice %1").arg(slc));

  // Wrap C++ array as a NumPy array (no copy, shared memory)
  // [slice] is modified in-place by Python function and takes care of data type conversion if needed
  // On return, `slice` contains Python-modified data
  try
    {
      if (m_voxelType == _UChar)
	{
	  py::array_t<uint8_t> pySlice(
	    {N}, {sizeof(uint8_t)}, slice, py::cast(nullptr));
	  m_pyModule.attr("get_depth_slice")(slc, pySlice);
	}
      else if (m_voxelType == _Char)
	{
	  py::array_t<char> pySlice(
	    {N}, {sizeof(char)}, reinterpret_cast<char*>(slice), py::cast(nullptr));
	  m_pyModule.attr("get_depth_slice")(slc, pySlice);
	}
      else if (m_voxelType == _UShort)
	{
	  py::array_t<ushort> pySlice(
	    {N}, {sizeof(ushort)}, reinterpret_cast<ushort*>(slice), py::cast(nullptr));
	  m_pyModule.attr("get_depth_slice")(slc, pySlice);
	}
      else if (m_voxelType == _Short)
	{
	  py::array_t<short> pySlice(
	    {N}, {sizeof(short)}, reinterpret_cast<short*>(slice), py::cast(nullptr));
	  m_pyModule.attr("get_depth_slice")(slc, pySlice);
	}
      else if (m_voxelType == _Int)
	{
	  py::array_t<int> pySlice(
	    {N}, {sizeof(int)}, reinterpret_cast<int*>(slice), py::cast(nullptr));
	  m_pyModule.attr("get_depth_slice")(slc, pySlice);
	}
      else if (m_voxelType == _Float)
	{
	  py::array_t<float> pySlice(
	    {N}, {sizeof(float)}, reinterpret_cast<float*>(slice), py::cast(nullptr));
	  m_pyModule.attr("get_depth_slice")(slc, pySlice);
	}
      return true;
    }
  catch (const std::exception &e)
    {
      std::memset(slice, 0,
	static_cast<size_t>(N)*static_cast<size_t>(m_bytesPerVoxel));
      m_lastError = QString("Cannot read Python volume slice %1: %2")
	.arg(slc).arg(e.what());
      qWarning().noquote() << m_lastError;
      return false;
    }

  //QMessageBox::information(0, "Info", QString("Depth slice %1 retrieved successfully").arg(slc));
}

QString ScriptsPlugin::lastError() const { return m_lastError; }


QVariant
ScriptsPlugin::rawValue(int d, int w, int h)
{
  m_lastError.clear();
  QVariant v;

  if (d < 0 || d >= m_depth ||
      w < 0 || w >= m_width ||
      h < 0 || h >= m_height)
    {
      v = QVariant("OutOfBounds");
      return v;
    }

  try
    {
      py::object result = m_pyModule.attr("get_rawvalue")(d, w, h);

      if (py::isinstance<py::str>(result))
	return QVariant(QString::fromStdString(result.cast<std::string>()));

      if (m_voxelType == _UChar)
	return QVariant(static_cast<uint>(result.cast<uchar>()));
      if (m_voxelType == _Char)
	return QVariant(static_cast<int>(result.cast<char>()));
      if (m_voxelType == _UShort)
	return QVariant(static_cast<uint>(result.cast<ushort>()));
      if (m_voxelType == _Short)
	return QVariant(static_cast<int>(result.cast<short>()));
      if (m_voxelType == _Int)
	return QVariant(result.cast<int>());
      if (m_voxelType == _Float)
	return QVariant(result.cast<float>());
    }
  catch (const std::bad_alloc&)
    {
      m_lastError = "The Python volume decoder ran out of memory while "
                    "reading a voxel value.";
    }
  catch (const std::exception &e)
    {
      m_lastError = QString("Cannot read Python volume value: %1").arg(e.what());
    }
  catch (...)
    {
      m_lastError = "The Python volume decoder raised an unknown exception "
                    "while reading a voxel value.";
    }

  if (!m_lastError.isEmpty())
    qWarning().noquote() << m_lastError;
  return v;
}
