/****************************************************************************
** Meta object code from reading C++ file 'transferfunctioncontainer.h'
**
** Created: Tue Dec 17 10:58:30 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "transferfunctioncontainer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'transferfunctioncontainer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_TransferFunctionContainer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      26,   37,   37,   37, 0x0a,
      38,   37,   37,   37, 0x0a,
      55,   37,   37,   37, 0x0a,
      83,   37,   37,   37, 0x0a,
     124,   37,   37,   37, 0x0a,
     138,   37,   37,   37, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_TransferFunctionContainer[] = {
    "TransferFunctionContainer\0switch1D()\0"
    "\0clearContainer()\0fromDomElement(QDomElement)\0"
    "fromSplineInformation(SplineInformation)\0"
    "addSplineTF()\0removeSplineTF(int)\0"
};

void TransferFunctionContainer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TransferFunctionContainer *_t = static_cast<TransferFunctionContainer *>(_o);
        switch (_id) {
        case 0: _t->switch1D(); break;
        case 1: _t->clearContainer(); break;
        case 2: _t->fromDomElement((*reinterpret_cast< QDomElement(*)>(_a[1]))); break;
        case 3: _t->fromSplineInformation((*reinterpret_cast< SplineInformation(*)>(_a[1]))); break;
        case 4: _t->addSplineTF(); break;
        case 5: _t->removeSplineTF((*reinterpret_cast< int(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData TransferFunctionContainer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject TransferFunctionContainer::staticMetaObject = {
    { &QObject::staticMetaObject, qt_meta_stringdata_TransferFunctionContainer,
      qt_meta_data_TransferFunctionContainer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &TransferFunctionContainer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *TransferFunctionContainer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *TransferFunctionContainer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_TransferFunctionContainer))
        return static_cast<void*>(const_cast< TransferFunctionContainer*>(this));
    return QObject::qt_metacast(_clname);
}

int TransferFunctionContainer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
