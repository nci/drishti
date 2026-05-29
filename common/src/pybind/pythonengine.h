#pragma once
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

#include <memory>
#include <string>

namespace py = pybind11;

class PythonEngine
{
public:
    static PythonEngine& instance();

    PythonEngine();
    ~PythonEngine();

    // Import a Python module
    pybind11::object import(const std::string& name);

    // Optional: evaluate a string
    pybind11::object eval(const std::string& code);

    // Optional: execute a string (no return)
    void exec(const std::string& code);

    void print(const std::string& code);

private:
    static std::unique_ptr<PythonEngine> m_instance;

    //PythonEngine(const PythonEngine&) = delete;
    //PythonEngine& operator=(const PythonEngine&) = delete;

    std::unique_ptr<pybind11::scoped_interpreter> m_guard;
};
