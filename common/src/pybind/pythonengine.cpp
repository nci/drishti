#include "pythonengine.h"
#include <iostream>

#include <QCoreApplication>
#include <QMessageBox>

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


PythonEngine& PythonEngine::instance()
{
    // CPython extensions and the Qt-backed output redirect have process-wide
    // state. Finalizing them during static destruction is not reliably ordered.
    static PythonEngine *const engine = new PythonEngine();
    return *engine;
}

PythonEngine::PythonEngine()
{
    m_guard = std::make_unique<py::scoped_interpreter>();

    // Use the executable directory, independent of the caller's working directory.
    const std::string pythonLibDir =
        QCoreApplication::applicationDirPath().toStdString();

    py::module sys = py::module::import("sys");
    py::object path = sys.attr("path");
    path.attr("insert")(0, pythonLibDir);
    sys.attr("dont_write_bytecode") = true;
  
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
