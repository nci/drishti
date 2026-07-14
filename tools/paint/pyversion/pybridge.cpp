#include "pybridge.h"
#include <iostream>

PaintVolMask* PaintVolMask::global_paint_vol_mask = 0;
py::module PaintVolMask::global_pyModule;

PYBIND11_EMBEDDED_MODULE(paintmod, m) {
    m.doc() = "Drishti Paint bridge for Python integration";

    py::class_<PaintVolMask>(m, "PaintVolMask")
        .def(py::init())

        .def_readwrite("depth", &PaintVolMask::depth)
        .def_readwrite("width", &PaintVolMask::width)
        .def_readwrite("height", &PaintVolMask::height)
        .def_readwrite("script_args", &PaintVolMask::pyDict)

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

bool
PaintVolMask::processSlice(uchar *img, ushort *mask, 
                          int width, int height,
                          int tag)
{
    py::gil_scoped_acquire gil;

    if (py::hasattr(PaintVolMask::global_pyModule, "process_slice") == false)
    {
        std::cout << "** process_slice NOT FOUND\n";
        return false;
    }

    int64_t size = width * height;
    py::array_t<uint8_t> py_img = py::array_t<uint8_t>({size}, 
                                                        {sizeof(uint8_t)},
                                                        img, 
                                                        py::cast(nullptr));                             
    py::array_t<uint16_t> py_mask = py::array_t<uint16_t>({size}, 
                                                        {sizeof(uint16_t)},
                                                        mask, 
                                                        py::cast(nullptr));
    
    PaintVolMask::global_pyModule.attr("process_slice")(py_img, py_mask, 
                                                        width, height,
                                                        tag);
    
    // mask updated inside script
    return true;
}
