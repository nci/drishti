/****************************************************************************
** Meta object code from reading C++ file 'transferfunctionmanager.h'
**
** Created: Tue Dec 17 10:58:33 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "transferfunctionmanager.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'transferfunctionmanager.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_TransferFunctionManager[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: signature, parameters, type, tag, flags
      24,   71,   73,   73, 0x05,
      74,  106,   73,   73, 0x05,

 // slots: signature, parameters, type, tag, flags
     109,   73,   73,   73, 0x0a,
     124,   73,   73,   73, 0x0a,
     149,   73,   73,   73, 0x09,
     168,   71,   73,   73, 0x09,
     189,   71,   73,   73, 0x09,
     210,   73,   73,   73, 0x09,
     236,   73,   73,   73, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_TransferFunctionManager[] = {
    "TransferFunctionManager\0"
    "changeTransferFunctionDisplay(int,QList<bool>)\0"
    ",\0\0checkStateChanged(int,int,bool)\0"
    ",,\0clearManager()\0addNewTransferFunction()\0"
    "headerClicked(int)\0cellClicked(int,int)\0"
    "cellChanged(int,int)\0refreshTransferFunction()\0"
    "removeTransferFunction()\0"
};

void TransferFunctionManager::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TransferFunctionManager *_t = static_cast<TransferFunctionManager *>(_o);
        switch (_id) {
        case 0: _t->changeTransferFunctionDisplay((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QList<bool>(*)>(_a[2]))); break;
        case 1: _t->checkStateChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 2: _t->clearManager(); break;
        case 3: _t->addNewTransferFunction(); break;
        case 4: _t->headerClicked((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 5: _t->cellClicked((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 6: _t->cellChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 7: _t->refreshTransferFunction(); break;
        case 8: _t->removeTransferFunction(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData TransferFunctionManager::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject TransferFunctionManager::staticMetaObject = {
    { &QFrame::staticMetaObject, qt_meta_stringdata_TransferFunctionManager,
      qt_meta_data_TransferFunctionManager, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &TransferFunctionManager::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *TransferFunctionManager::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *TransferFunctionManager::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_TransferFunctionManager))
        return static_cast<void*>(const_cast< TransferFunctionManager*>(this));
    return QFrame::qt_metacast(_clname);
}

int TransferFunctionManager::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QFrame::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void TransferFunctionManager::changeTransferFunctionDisplay(int _t1, QList<bool> _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void TransferFunctionManager::checkStateChanged(int _t1, int _t2, bool _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_END_MOC_NAMESPACE
