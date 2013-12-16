/****************************************************************************
** Meta object code from reading C++ file 'volumemask.h'
**
** Created: Tue Dec 17 10:58:39 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "volumemask.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'volumemask.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_VolumeMask[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      11,   32,   32,   32, 0x05,
      33,   32,   32,   32, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_VolumeMask[] = {
    "VolumeMask\0progressChanged(int)\0\0"
    "progressReset()\0"
};

void VolumeMask::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        VolumeMask *_t = static_cast<VolumeMask *>(_o);
        switch (_id) {
        case 0: _t->progressChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 1: _t->progressReset(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData VolumeMask::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject VolumeMask::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_VolumeMask,
      qt_meta_data_VolumeMask, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &VolumeMask::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *VolumeMask::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *VolumeMask::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_VolumeMask))
        return static_cast<void*>(const_cast< VolumeMask*>(this));
    return QObject::qt_metacast(_clname);
}

int VolumeMask::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void VolumeMask::progressChanged(int _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void VolumeMask::progressReset()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
