/****************************************************************************
** Meta object code from reading C++ file 'splineeditor.h'
**
** Created: Tue Dec 17 10:58:25 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "splineeditor.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'splineeditor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_SplineEditor[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: signature, parameters, type, tag, flags
      13,   30,   30,   30, 0x05,
      31,   30,   30,   30, 0x05,
      47,   30,   30,   30, 0x05,
      75,   30,   30,   30, 0x05,

 // slots: signature, parameters, type, tag, flags
      91,   30,   30,   30, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_SplineEditor[] = {
    "SplineEditor\0refreshDisplay()\0\0"
    "splineChanged()\0selectEvent(QGradientStops)\0"
    "deselectEvent()\0setGradientStops(QGradientStops)\0"
};

void SplineEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        SplineEditor *_t = static_cast<SplineEditor *>(_o);
        switch (_id) {
        case 0: _t->refreshDisplay(); break;
        case 1: _t->splineChanged(); break;
        case 2: _t->selectEvent((*reinterpret_cast< QGradientStops(*)>(_a[1]))); break;
        case 3: _t->deselectEvent(); break;
        case 4: _t->setGradientStops((*reinterpret_cast< QGradientStops(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData SplineEditor::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject SplineEditor::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_SplineEditor,
      qt_meta_data_SplineEditor, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &SplineEditor::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *SplineEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *SplineEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SplineEditor))
        return static_cast<void*>(const_cast< SplineEditor*>(this));
    return QWidget::qt_metacast(_clname);
}

int SplineEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    }
    return _id;
}

// SIGNAL 0
void SplineEditor::refreshDisplay()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void SplineEditor::splineChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}

// SIGNAL 2
void SplineEditor::selectEvent(QGradientStops _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void SplineEditor::deselectEvent()
{
    QMetaObject::activate(this, &staticMetaObject, 3, 0);
}
QT_END_MOC_NAMESPACE
