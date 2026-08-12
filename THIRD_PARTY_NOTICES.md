# Third-party notices

This portable distribution contains dynamically linked or embedded third-party
software. Drishti's own license files are `LICENSE` and `license.md`. Copyright
and license texts supplied by the dependency projects are collected under the
`licenses` directory in the distribution. Those texts, rather than this
summary, govern the corresponding components.

Major bundled components include:

- Qt 5.15.2 (Qt Core, GUI, Widgets, OpenGL, Network, Multimedia, SVG, XML, and
  deployment plugins)
- CPython 3.13.2 and NumPy 2.4.1
- libQGLViewer 2.6.4
- ITK 5.0.1
- OpenVDB 11.0.0 and oneTBB
- FFmpeg 9.0
- Assimp 6.0.4
- Gmsh 4.15.2
- NetCDF-C 4.9.3 and NetCDF-C++4 4.3.1
- HDF5, libtiff, OpenEXR, Imath, OpenJPEG, OpenJPH, OpenBLAS, Boost, zlib,
  zstd, lz4, Blosc, bzip2, liblzma, libdeflate, Snappy, cURL, OpenSSL,
  SQLite, GLEW, FreeGLUT, pugixml, TinyXML2, and their transitive libraries
- Microsoft Visual C++ runtime redistributable libraries

The Qt libraries are loaded dynamically and are not modified by this build.
Optional Mesh/Paint Python scripts are not self-contained: when selected, they
run an external Python installation and are governed by the licenses of the
packages installed into that external environment.

The dependency inventory and file-level SHA-256 list generated for this exact
archive are included beside this notice. This build is not code-signed.
