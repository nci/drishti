#include "pythonengine.h"
#include "pybridge.h"
#include "pyversion.h"

#include <iostream>
#include <QFileInfo>
#include <QMessageBox>


void
PyVersion::clear()
{
  if (PaintVolMask::global_paint_vol_mask)
  {
    delete PaintVolMask::global_paint_vol_mask;
    PaintVolMask::global_paint_vol_mask = 0;
  }
}
QString
PyVersion::scriptName()
{
  return PaintVolMask::global_paint_vol_mask->scriptName;
}

bool PyVersion::hasInit() {return m_hasInit;}
bool PyVersion::hasDataAllocator() {return m_hasDataAllocator;}
bool PyVersion::hasSliceProcessor() {return m_hasSliceProcessor;}
bool PyVersion::hasVolumeProcessor() {return m_hasVolumeProcessor;}

void
PyVersion::init(QString script,
                uchar *vol, ushort *mask, uchar *lut, 
                int depth, int width, int height)
{    
  PythonEngine &pythonGuard = PythonEngine::instance();
  (&pythonGuard)->init(true);
  
  m_script = script;
  
  if (!PaintVolMask::global_paint_vol_mask)
    PaintVolMask::global_paint_vol_mask = new PaintVolMask();

  PaintVolMask::global_paint_vol_mask->volume = vol;
  PaintVolMask::global_paint_vol_mask->mask = mask;
  PaintVolMask::global_paint_vol_mask->lut = lut;
  PaintVolMask::global_paint_vol_mask->depth = depth;
  PaintVolMask::global_paint_vol_mask->width = width;
  PaintVolMask::global_paint_vol_mask->height = height;
}

void
PyVersion::setMask(ushort *mask)
{
  if (PaintVolMask::global_paint_vol_mask)
    PaintVolMask::global_paint_vol_mask->mask = mask;
}

QString 
PyVersion::initScript()
{
  PaintVolMask::global_paint_vol_mask->scriptActive = false;

  m_hasInit = false;
  m_hasDataAllocator = false;
  m_hasSliceProcessor = false;
  m_hasVolumeProcessor = false;

  py::gil_scoped_acquire gil;

  try {
    QString spath = QFileInfo(m_script).absolutePath();
    QString ps = "import sys\nsys.path.insert(0, r\"" + spath + "\")";
    py::exec(ps.toStdString());

    QString scriptName = QFileInfo(m_script).baseName();
    std::cout << "Importing module:" << scriptName.toStdString() << "\n";
    PaintVolMask::global_pyModule = py::module_::import(scriptName.toStdString().c_str());
    PaintVolMask::global_paint_vol_mask->scriptName = scriptName;
    
    if (py::hasattr(PaintVolMask::global_pyModule, "init"))
      m_hasInit = true;

    if (py::hasattr(PaintVolMask::global_pyModule, "set_paint_data"))
      m_hasDataAllocator = true;

    if (py::hasattr(PaintVolMask::global_pyModule, "process_slice"))
      m_hasSliceProcessor = true;

    if (py::hasattr(PaintVolMask::global_pyModule, "process_volume"))
      m_hasVolumeProcessor = true;

    std::cout << "** init " << (m_hasInit?"found":"NOT FOUND") << "\n";
    std::cout << "** set_paint_data " << (m_hasDataAllocator?"found":"NOT FOUND") << "\n";
    std::cout << "** process_slice " << (m_hasSliceProcessor?"found":"NOT FOUND") << "\n";
    std::cout << "** process_volume " << (m_hasVolumeProcessor?"found":"NOT FOUND") << "\n";

    std::cout << "** Successfully imported module: " << scriptName.toStdString() << "\n";
    py::object py_paintvolmask = py::cast(PaintVolMask::global_paint_vol_mask);
      
    std::cout << "** Setting paint data in Python module... ";
    PaintVolMask::global_pyModule.attr("set_paint_data")(py_paintvolmask);
    
    if (m_hasInit)
      PaintVolMask::global_pyModule.attr("init")();

    PaintVolMask::global_paint_vol_mask->scriptActive = true;

    std::cout << "done.\n";

    return "true";
  }
  catch (const std::exception& e) {
    PaintVolMask::global_paint_vol_mask->scriptActive = false;
    std::cout << "*** Error ***\n";
    std::cout << "Failed to import module: " + QString(e.what()).toStdString();
    return QString(e.what());
  }

}

bool
PyVersion::process_slice(uchar *image, ushort *mask, int width, int height, int tag)
{
  if (!m_hasSliceProcessor)
    {
      std::cout << "** NO SLICE PROCESSOR FOUND\n";
      return false;
    }
  
  py::gil_scoped_acquire gil;
  int size = width* height;
  py::array_t<uint8_t> py_img = py::array_t<uint8_t>({size}, 
                                                     {sizeof(uint8_t)},
                                                     image, 
                                                     py::cast(nullptr));
  py::array_t<uint16_t> py_mask = py::array_t<uint16_t>({size}, 
                                                      {sizeof(uint16_t)},
                                                      mask, 
                                                      py::cast(nullptr));

  std::cout << "** Calling process_slice in Python module...\n";


  PaintVolMask::global_pyModule.attr("process_slice")(py_img, py_mask, 
                                                      width, height,
                                                      tag);
  return true;
}

bool
PyVersion::process_volume()
{
  if (!m_hasVolumeProcessor)
    {
      std::cout << "** NO VOLUME PROCESSOR FOUND\n";
      return false;
    }

  std::cout << "** Calling process_volume in Python module...\n";
  
  py::gil_scoped_acquire gil;
  
  PaintVolMask::global_pyModule.attr("process_volume")();
  
  return true;
}

void
PyVersion::populateArguments(QHash<QString, QVariant> arguments)
{
  // population the arguments dictionary in the global paint vol mask object
  PaintVolMask::global_paint_vol_mask->pyDict.clear();

  for (auto it = arguments.begin(); it != arguments.end(); ++it)
  {
    if (it.value().type() == QVariant::Int)
      PaintVolMask::global_paint_vol_mask->pyDict[py::str(it.key().toLatin1().data())] = 
                                                                    py::int_(it.value().toInt());
    else if (it.value().type() == QVariant::Double)
      PaintVolMask::global_paint_vol_mask->pyDict[py::str(it.key().toLatin1().data())] = 
                                                                py::float_(it.value().toDouble());
    else if (it.value().type() == QVariant::Bool)
      PaintVolMask::global_paint_vol_mask->pyDict[py::str(it.key().toLatin1().data())] = 
                                                                  py::bool_(it.value().toBool());
    else
      PaintVolMask::global_paint_vol_mask->pyDict[py::str(it.key().toLatin1().data())] = 
                                                py::str(it.value().toString().toLatin1().data());
  }
}
