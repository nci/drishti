/****************************************************************************
** Meta object code from reading C++ file 'propertyeditor.h'
**
** Created: Mon Nov 18 14:44:24 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../propertyeditor.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'propertyeditor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_PropertyEditor[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      15,   36,   36,   36, 0x08,
      37,   36,   36,   36, 0x08,
      56,   36,   36,   36, 0x08,
      78,   36,   36,   36, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_PropertyEditor[] = {
    "PropertyEditor\0changeColor(QString)\0"
    "\0resetProperty(int)\0helpItemSelected(int)\0"
    "hotkeymouseClicked(bool)\0"
};

void PropertyEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        PropertyEditor *_t = static_cast<PropertyEditor *>(_o);
        switch (_id) {
        case 0: _t->changeColor((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 1: _t->resetProperty((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 2: _t->helpItemSelected((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->hotkeymouseClicked((*reinterpret_cast< bool(*)>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData PropertyEditor::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject PropertyEditor::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_PropertyEditor,
      qt_meta_data_PropertyEditor, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &PropertyEditor::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *PropertyEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *PropertyEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_PropertyEditor))
        return static_cast<void*>(const_cast< PropertyEditor*>(this));
    return QDialog::qt_metacast(_clname);
}

int PropertyEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
