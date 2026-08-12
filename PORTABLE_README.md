# Drishti CPU + integrated-GPU portable build

Build date: 2026-08-12

## Run

1. Extract the complete ZIP to a normal local directory. Do not run programs
   from inside the ZIP.
   If Windows shows an Unblock checkbox in the ZIP file's Properties dialog,
   unblock the ZIP before extracting it. The binaries are not code-signed, so
   keep any Defender or SmartScreen warning and quarantine details for
   diagnosis instead of silently restoring individual DLLs.
2. Install the current Intel or AMD graphics driver supplied by the laptop or
   GPU vendor.
3. In Windows Graphics settings, select the power-saving integrated GPU for
   `drishti.exe`, `drishtiimport.exe`, `drishtipaint.exe`, and
   `drishtimesh.exe`.
4. Start the required executable directly. Core Drishti functions and Import
   do not require Qt, Visual Studio, Python, or build tools to be installed.

## Programs

- `drishtiimport.exe`: CPU image/volume import and conversion.
- `drishti.exe`: volume viewing and rendering.
- `drishtipaint.exe`: segmentation and annotation.
- `drishtimesh.exe`: surface-mesh viewing, editing, and presentation.

## Hardware contract

Import and heavy data processing use the CPU. Interactive 3D display uses the
integrated GPU through Windows Desktop OpenGL. Drishti requires OpenGL 4.5
Compatibility; Paint and Mesh require OpenGL 4.2 Compatibility. Microsoft
Basic Render Driver, GDI Generic, and Remote Desktop software rendering are not
supported 3D renderers.

## Included scope

The package includes the Qt/VC runtime libraries, four main programs, 16 native
Import plugins, six native Render plugins, one MOP plugin, production assets,
and five PDF help documents. It includes common import formats, mesh
generation, five ITK processing plugins, and ITK segmentation operations.

This is the complete current top-level build, not every historical or optional
project in the repository. HDF4, legacy NetCDF-3, Mesh Repaint, Mesh Simplify,
VED (vessel-enhancing diffusion), and OpenVR are not built or validated in this
package. Modern NetCDF support is included through the NC4 plugin.

Python 3.13.2 and NumPy 2.4.1 are embedded only for the memory-mapped RAW and
NumPy `.npy` Import scripts. Compressed `.npz` archives are not supported.
Python is initialized only when one of those scripts is selected, so native
TIFF and other import plugins do not depend on Python startup. The embedded
interpreter ignores system and user Python package directories. Both Python
path configuration files disable automatic `site` import, while listing the
bundled NumPy directory explicitly.

Optional Mesh/Paint scripts are a separate feature. They launch an external
`python` executable from `PATH` and require their listed packages, such as
PyQt5, PyVista, SciPy, OpenCV, TensorFlow, or blosc2. Those large packages are
not bundled and those optional scripts are not part of the portable core
guarantee.

The TIFF importer accepts scalar top-left and bottom-left TIFF stacks. It keeps
the stored scanline order as the volume axis and does not silently flip bottom-
left microscopy/CT acquisition data.

For 8-bit or 16-bit microscopy/CT TIFF data, select `Grayscale TIFF Image
Files` or `Grayscale TIFF Image Directory`. Do not select `Standard Image
Files`: that general color-image path converts through Qt ARGB32 and therefore
does not preserve 16-bit grayscale intensity.

## Import regression

The release TIFF plugin was tested offscreen, without OpenGL, against the full
Living Ant dataset supplied for this issue: 1024 TIFF files, 1024 x 1024,
unsigned 16-bit, bottom-left orientation. It completed metadata/statistics and
decoded all 1,073,741,824 voxels in 65.0 seconds without an orientation
rejection or stalled process. Peak process Working Set was 26.9 MiB and the
decoded-volume SHA-256 was
`ceb1e998c616a73c743a53b64a1a6fd7e01be4103762dff872d844e54a7c3cb4`.
Synthetic top-left/bottom-left orientation, numeric filename ordering,
truncation rollback, empty input, plugin destruction, and reload tests also
passed. The DICOM importer additionally passed repeated cancellation and
transaction rollback tests.

## Integrated-GPU regression

A hidden `QGLWidget` using the same Desktop OpenGL compatibility contract was
forced to the Intel power-saving adapter on the build machine. It reported
Intel UHD Graphics 770 driver 32.0.101.6129, OpenGL 4.5 compatibility, GLSL
4.50, maximum 2D texture size 16384, maximum 3D texture size 2048, and 2048
array layers. Compatibility shaders, an RGBA16F framebuffer, and representative
3D/array texture allocations all passed.

## Known limits

- This is CPU compute plus integrated-GPU display, not a pure CPU software
  renderer. A working Intel/AMD Desktop OpenGL compatibility driver is required
  for the three 3D programs; Import itself does not create an OpenGL context.
- The supplied i7-13700H laptop has not yet been retested with this exact ZIP.
  The current regression used Intel UHD Graphics 770; AMD integrated graphics
  were not available on the build machine. Those target systems still require
  the short on-device acceptance test described above.
- Memory admission can reject a volume before allocation when physical-memory
  or Windows Commit headroom is unsafe. A safe refusal is intentional and is
  preferable to paging the entire machine until Windows stops responding.
- Some large segmentation and processing operations remain synchronous CPU
  tasks. They are protected against known oversized allocations but can still
  make the application temporarily unresponsive near the supported limit.
- RAW and NumPy `.npy` import use memory mapping plus chunked or one-slice
  buffers, so they no longer copy an entire volume into RAM. Their initial
  statistics pass is still synchronous and CPU-bound; very large inputs can
  temporarily show `Not responding` even while the scan is making progress.
- TXM passed synthetic compound-file corruption and lifecycle tests, but no
  vendor-produced TXM sample was available. Large compressed NIfTI `.nii.gz`
  volumes may be slow because random slice access can require repeated
  decompression.
- The executables are not code-signed. SmartScreen or endpoint-security policy
  may require an administrator-approved exception for this exact ZIP hash.
- Windows N editions require Microsoft's matching Media Feature Pack for the
  optional Qt multimedia backends. Import, volume rendering, painting, and
  mesh processing do not use those media backends.

## Troubleshooting

- Keep every DLL and plugin directory beside the executables as packaged.
- Keep the `assets`, `docs`, and `python` directories beside the executables.
- If no OpenGL renderer is available, update the Intel/AMD driver and retest
  outside Remote Desktop.
- Keep the Windows page file enabled and system-managed.
- On an import error, record the full dialog text, input format, dimensions,
  bit depth, physical-memory use, and Windows Commit use.
