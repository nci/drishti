#include "pyversion.h"
#include "common.h"

#include <iostream>
#include <QFileInfo>
#include <QMessageBox>

QString
PyVersion::init(QString script)
{
    m_fileName.clear();
    m_4dvol = false;
    m_description = "";
    m_histogram.clear();
    m_headerBytes = 0;
    m_bytesPerVoxel = 1;
    m_depth = m_width = m_height = 0;
    m_voxelUnit = 0;
    m_voxelType = 0;
    m_voxelSizeX = m_voxelSizeY = m_voxelSizeZ = 1;

    PythonEngine &pythonGuard = PythonEngine::instance();
    (&pythonGuard)->init(true);
    m_script = script;

    py::gil_scoped_acquire gil;

    try {
        QString spath = QFileInfo(m_script).absolutePath();
        std::cout << "Script path:" << spath.toStdString() << "\n";
        QString ps = "import sys\nsys.path.insert(0, r\"" + spath + "\")";
        py::exec(ps.toStdString());

        QString scriptName = QFileInfo(m_script).baseName();
        std::cout << "Importing module:" << scriptName.toStdString() << "\n";

        m_pyModule = py::module_::import(scriptName.toStdString().c_str());
        //ps = "import "+scriptName+" as testmod\nprint('Available functions :', dir(testmod))";
        //py::exec(ps.toStdString());

        std::cout << "** Successfully imported module: "
                  << scriptName.toStdString() << "\n";
    }
    catch (const std::exception& e) {
      return QString(e.what());
    }
    
    return "true";
}

void
PyVersion::replaceFile(QString flnm)
{
    m_fileName.clear();
    m_fileName << flnm;
}

void
PyVersion::setFile(QStringList files)
{
  py::gil_scoped_acquire gil;

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
  //----------------------------------- 

  m_fileName = files;
}

QList<float>
PyVersion::voxelSize()
{
    QList<float> vxyz;
    vxyz << m_voxelSizeX << m_voxelSizeY << m_voxelSizeZ;
    return vxyz;
}

QList<int>
PyVersion::gridSize()
{
    QList<int> vxyz;
    vxyz << m_depth << m_width << m_height;
    return vxyz;
}

QList<float>
PyVersion::rawMinMax()
{
    QList<float> vxy;
    vxy << m_rawMin << m_rawMax;
    return vxy;
}

QList<uint>
PyVersion::histogram()
{
    py::gil_scoped_acquire gil;
   
    uint hist[65536] = {0};
  
    // Wrap C++ array as a NumPy array (no copy, shared memory)
    py::array_t<uint> pyHist(
      {65536},              // shape
      {sizeof(uint)},       // stride
      hist,                 // pointer to data
      py::cast(nullptr)     // no base object, raw memory
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
PyVersion::depthSlice(int slc, uchar *slice)
{
    py::gil_scoped_acquire gil;

    qint64 N = m_width*m_height;
    if (slc < 0 || slc >= m_depth)
      {
        memset(slice, 0, N*m_bytesPerVoxel);
        return;
      }

    //QMessageBox::information(0, "Info", QString("Getting depth slice %1").arg(slc));

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

    //QMessageBox::information(0, "Info", QString("Depth slice %1 retrieved successfully").arg(slc));
}
QVariant
PyVersion::rawValue(int d, int w, int h)
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
      //py::print("string");
      std::string s = result.cast<std::string>();
      return QVariant(QString::fromStdString(s));
  }

  if (m_voxelType == _UChar) 
  {
    uint val = result.cast<uchar>();
    return QVariant(val);
  }
  else if (m_voxelType == _Char)
  {
    int val = result.cast<char>();
    return QVariant(val);
  }
  else if (m_voxelType == _UShort) 
  {
    uint val = result.cast<ushort>();
    return QVariant(val);
  }
  else if (m_voxelType == _Short) 
  {
    int val = result.cast<short>();
    return QVariant(val);
  }
  else if (m_voxelType == _Int) 
  {
    int val = result.cast<int>();
    return QVariant(val);
  }
  else if (m_voxelType == _Float)
  { 
    float val = result.cast<float>();
    return QVariant(val);
  }

  return v;
}
