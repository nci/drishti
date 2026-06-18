#include "pybridge.h"


PaintVolMask* PaintVolMask::global_paint_vol_mask = 0;
py::module PaintVolMask::global_pyModule;

PYBIND11_EMBEDDED_MODULE(paintmod, m) {
    m.doc() = "Drishti Paint bridge for Python integration";

    py::class_<PaintVolMask>(m, "PaintVolMask")
        .def(py::init())

        .def_readwrite("depth", &PaintVolMask::depth)
        .def_readwrite("width", &PaintVolMask::width)
        .def_readwrite("height", &PaintVolMask::height)

        .def("get_volume_view", &PaintVolMask::get_volume_view)
        .def("get_mask_view", &PaintVolMask::get_mask_view)
        .def("get_lut_view", &PaintVolMask::get_lut_view)

        .def("update_slice_view", &PaintVolMask::update_slice_view)
        .def("update_3d_view", &PaintVolMask::update_3d_view);
      }


py::array_t<uint8_t>
PaintVolMask::get_volume_view() 
{
    int64_t size = static_cast<int64_t>(depth) * width * height;
    return py::array_t<uint8_t>({size}, 
                               {sizeof(uint8_t)},
                               volume, 
                               py::cast(nullptr));
}

py::array_t<uint8_t> 
PaintVolMask::get_lut_view() 
{
    int64_t size = 256*4; // Assuming LUT has 256 entries with RGBA values
    return py::array_t<uint8_t>({size}, 
                               {sizeof(uint8_t)},
                               lut, 
                               py::cast(nullptr));
}

py::array_t<uint16_t> 
PaintVolMask::get_mask_view() 
{
    int64_t size = static_cast<int64_t>(depth) * width * height;
    return py::array_t<uint16_t>({size}, 
                               {sizeof(uint16_t)},
                               mask, 
                               py::cast(nullptr));
}

void 
PaintVolMask::update_slice_view()
{
    emit reloadSlices();
}

void 
PaintVolMask::update_3d_view()
{
    emit viewerUpdate();
}