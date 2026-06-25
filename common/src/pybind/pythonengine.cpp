#include "pythonengine.h"
#include <filesystem>
#include <iostream>

#include <QMessageBox>

namespace fs = std::filesystem;

//------------------------------
// A C++ class that will act as Python's sys.stdout/sys.stderr
class CoutRedirect {
public:
    void write(const std::string &msg) {
        std::cout << msg; // Send Python output to C++ std::cout
    }
    void flush() {
        std::cout << std::flush;
    }
};

class CerrRedirect {
public:
    void write(const std::string &msg) {
        std::cerr << msg; // Send Python output to C++ std::cerr
    }
    void flush() {
        std::cerr << std::flush;
    }
};
//------------------------------


//------------------------------
PYBIND11_EMBEDDED_MODULE(pyredir, m) {
    py::class_<CoutRedirect>(m, "CoutRedirect")
        .def(py::init<>())
        .def("write", &CoutRedirect::write)
        .def("flush", &CoutRedirect::flush);

    py::class_<CerrRedirect>(m, "CerrRedirect")
        .def(py::init<>())
        .def("write", &CerrRedirect::write)
        .def("flush", &CerrRedirect::flush);
}
//------------------------------


std::unique_ptr<PythonEngine> PythonEngine::m_instance = nullptr;

PythonEngine& PythonEngine::instance()
{
    if (m_instance == nullptr)
    {
        m_instance = std::make_unique<PythonEngine>();
    }

    return *m_instance;
}

PythonEngine::PythonEngine()
{
    m_pythonInstalled = false;

    m_guard = nullptr;
}

void
PythonEngine::init(bool flag)
{
    m_pythonInstalled = flag;

    m_guard = nullptr;

    if (m_pythonInstalled)
        m_guard = std::make_unique<py::scoped_interpreter>();
    else
        return;

    // Determine the path to the extracted Python library
    fs::path exePath = fs::current_path(); // Adjust if needed
    fs::path pythonLibDir = exePath; // Assuming the Python library is in the same directory as the executable";

    // Set the Python path to include the directory
    py::module sys = py::module::import("sys");
    py::object path = sys.attr("path");
    path.attr("insert")(0, pythonLibDir.string());
  
    // Example Python code execution
    try
    {
      py::exec(R"(
          import sys
          import pyredir
          
          # Redirect Python stdout/stderr to C++ streams
          sys.stdout = pyredir.CoutRedirect()
          sys.stderr = pyredir.CerrRedirect()
      )");
    }
    catch (const std::exception &e)
    {
        QMessageBox::critical(nullptr, "Python Error", e.what());
    }
}

PythonEngine::~PythonEngine() = default;

py::object PythonEngine::import(const std::string& name)
{
    return py::module_::import(name.c_str());
}

py::object PythonEngine::eval(const std::string& code)
{
    return py::eval(code);
}

void PythonEngine::exec(const std::string& code)
{
    py::exec(code);
}

void PythonEngine::print(const std::string& code)
{
    py::print(code);
}