#ifndef PYBRIDGE_H
#define PYBRIDGE_H

#include <pybind11/embed.h>
#include <pybind11/numpy.h>

#include <QObject>
#include <QMessageBox>


namespace py = pybind11;

class PaintVolMask : public QObject
{
    Q_OBJECT

public:
    static PaintVolMask *global_paint_vol_mask;    
    static py::module global_pyModule;

    PaintVolMask() : volume(nullptr), mask(nullptr), 
                     lut(nullptr), tag(nullptr),
                     depth(0), width(0), height(0),
                     scriptActive(false) {}

    py::array_t<uint8_t> get_volume_view();
    py::array_t<uint8_t> get_lut_view();
    py::array_t<uint8_t> get_tag_view();
    py::array_t<uint16_t> get_mask_view();

    void update_slice_view();
    void update_3d_view();

    bool processSlice(uchar*, ushort*, int, int, int);

    uint8_t *volume;
    uint16_t *mask;
    uint8_t *lut;
    uint8_t *tag;
    uint8_t *label_colors;
    int depth, width, height;

    py::dict pyDict;

    bool scriptActive;
    QString scriptName;

signals :
    void viewerUpdate();
    void reloadSlices();
    
};

#endif // PYBRIDGE_H