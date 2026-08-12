#include <Python.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>

namespace
{
std::string lower(std::string value)
{
  std::transform(value.begin(), value.end(), value.begin(),
                 [](unsigned char character) { return std::tolower(character); });
  return value;
}

std::string pythonString(PyObject *value)
{
  const char *text = value ? PyUnicode_AsUTF8(value) : nullptr;
  return text ? std::string(text) : std::string();
}
}

int main(int argc, char **argv)
{
  if (argc != 3)
    {
      std::cerr << "Usage: python_portable_runtime_smoke "
                   "<expected-runtime-prefix> <expected-package-prefix>\n";
      return 2;
    }

  Py_Initialize();
  if (!Py_IsInitialized())
    {
      std::cerr << "Python initialization failed\n";
      return 3;
    }

  PyObject *sys = PyImport_ImportModule("sys");
  PyObject *encodings = PyImport_ImportModule("encodings");
  PyObject *numpy = PyImport_ImportModule("numpy");
  if (!sys || !encodings || !numpy)
    {
      PyErr_Print();
      return 4;
    }

  PyObject *prefixObject = PyObject_GetAttrString(sys, "prefix");
  PyObject *numpyPathObject = PyObject_GetAttrString(numpy, "__file__");
  const std::string prefix = pythonString(prefixObject);
  const std::string numpyPath = pythonString(numpyPathObject);
  const std::string expectedRuntimePrefix = lower(argv[1]);
  const std::string expectedPackagePrefix = lower(argv[2]);

  Py_XDECREF(prefixObject);
  Py_XDECREF(numpyPathObject);
  Py_DECREF(numpy);
  Py_DECREF(encodings);
  Py_DECREF(sys);

  if (lower(prefix).find(expectedRuntimePrefix) != 0 ||
      lower(numpyPath).find(expectedPackagePrefix) != 0)
    {
      std::cerr << "Python escaped the portable runtime: prefix=" << prefix
                << " numpy=" << numpyPath << "\n";
      return 5;
    }

  if (Py_FinalizeEx() < 0)
    return 6;

  std::cout << "Portable Python and NumPy smoke passed\n"
            << "prefix=" << prefix << "\n"
            << "numpy=" << numpyPath << "\n";
  return 0;
}
