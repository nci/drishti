/****************************************************************************
** Meta object code from reading C++ file 'gradienteditor.h'
**
** Created: Tue Dec 17 10:58:20 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "gradienteditor.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gradienteditor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GradientEditor[] = {

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
      15,   33,   33,   33, 0x05,
      34,   33,   33,   33, 0x05,

       0        // eod
};

static const char qt_meta_stringdata_GradientEditor[] = {
    "GradientEditor\0gradientChanged()\0\0"
    "refreshGradient()\0"
};

void GradientEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        GradientEditor *_t = static_cast<GradientEditor *>(_o);
        switch (_id) {
        case 0: _t->gradientChanged(); break;
        case 1: _t->refreshGradient(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

const QMetaObjectExtraData GradientEditor::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject GradientEditor::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_GradientEditor,
      qt_meta_data_GradientEditor, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GradientEditor::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GradientEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GradientEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GradientEditor))
        return static_cast<void*>(const_cast< GradientEditor*>(this));
    return QWidget::qt_metacast(_clname);
}

int GradientEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
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
void GradientEditor::gradientChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}

// SIGNAL 1
void GradientEditor::refreshGradient()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
