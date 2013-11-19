/****************************************************************************
** Meta object code from reading C++ file 'dcolorwheel.h'
**
** Created: Mon Nov 18 14:44:23 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../dcolorwheel.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'dcolorwheel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DColorWheel[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       3,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      12,   27,   27,   27, 0x05,

 // slots: signature, parameters, type, tag, flags
      28,   27,   27,   27, 0x0a,
      41,   27,   27,   27, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_DColorWheel[] = {
    "DColorWheel\0colorChanged()\0\0moreShades()\0"
    "lessShades()\0"
};

void DColorWheel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DColorWheel *_t = static_cast<DColorWheel *>(_o);
        switch (_id) {
        case 0: _t->colorChanged(); break;
        case 1: _t->moreShades(); break;
        case 2: _t->lessShades(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData DColorWheel::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DColorWheel::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_DColorWheel,
      qt_meta_data_DColorWheel, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DColorWheel::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DColorWheel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DColorWheel::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DColorWheel))
        return static_cast<void*>(const_cast< DColorWheel*>(this));
    return QWidget::qt_metacast(_clname);
}

int DColorWheel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void DColorWheel::colorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
