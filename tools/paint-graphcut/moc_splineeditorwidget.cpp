/****************************************************************************
** Meta object code from reading C++ file 'splineeditorwidget.h'
**
** Created: Tue Dec 17 10:58:27 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "splineeditorwidget.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'splineeditorwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SplineEditorWidget[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      19,   51,   51,   51, 0x05,
      52,   51,   51,   51, 0x05,
      80,   51,   51,   51, 0x05,
      96,   51,   51,   51, 0x05,

 // slots: signature, parameters, type, tag, flags
     114,   51,   51,   51, 0x0a,
     147,   51,   51,   51, 0x0a,
     172,   51,   51,   51, 0x0a,
     205,   51,   51,   51, 0x0a,
     226,   51,   51,   51, 0x0a,
     246,   51,   51,   51, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_SplineEditorWidget[] = {
    "SplineEditorWidget\0transferFunctionChanged(QImage)\0"
    "\0selectEvent(QGradientStops)\0"
    "deselectEvent()\0switchHistogram()\0"
    "setGradientStops(QGradientStops)\0"
    "updateTransferFunction()\0"
    "selectSpineEvent(QGradientStops)\0"
    "deselectSpineEvent()\0hist1DClicked(bool)\0"
    "hist2DClicked(bool)\0"
};

void SplineEditorWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SplineEditorWidget *_t = static_cast<SplineEditorWidget *>(_o);
        switch (_id) {
        case 0: _t->transferFunctionChanged((*reinterpret_cast< QImage(*)>(_a[1]))); break;
        case 1: _t->selectEvent((*reinterpret_cast< QGradientStops(*)>(_a[1]))); break;
        case 2: _t->deselectEvent(); break;
        case 3: _t->switchHistogram(); break;
        case 4: _t->setGradientStops((*reinterpret_cast< QGradientStops(*)>(_a[1]))); break;
        case 5: _t->updateTransferFunction(); break;
        case 6: _t->selectSpineEvent((*reinterpret_cast< QGradientStops(*)>(_a[1]))); break;
        case 7: _t->deselectSpineEvent(); break;
        case 8: _t->hist1DClicked((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 9: _t->hist2DClicked((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SplineEditorWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SplineEditorWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_SplineEditorWidget,
      qt_meta_data_SplineEditorWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SplineEditorWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SplineEditorWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SplineEditorWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SplineEditorWidget))
        return static_cast<void*>(const_cast< SplineEditorWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int SplineEditorWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void SplineEditorWidget::transferFunctionChanged(QImage _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void SplineEditorWidget::selectEvent(QGradientStops _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void SplineEditorWidget::deselectEvent()
{
    QMetaObject::activate(this, &staticMetaObject, 2, 0);
}

// SIGNAL 3
void SplineEditorWidget::switchHistogram()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}
QT_END_MOC_NAMESPACE
