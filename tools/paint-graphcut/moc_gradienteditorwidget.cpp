/****************************************************************************
** Meta object code from reading C++ file 'gradienteditorwidget.h'
**
** Created: Tue Dec 17 10:58:22 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "gradienteditorwidget.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gradienteditorwidget.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_GradientEditorWidget[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       1,       // signalCount

 // signals: signature, parameters, type, tag, flags
      21,   53,   53,   53, 0x05,

 // slots: signature, parameters, type, tag, flags
      54,   53,   53,   53, 0x0a,
      87,   53,   53,   53, 0x0a,
     128,   53,   53,   53, 0x0a,
     145,   53,   53,   53, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_GradientEditorWidget[] = {
    "GradientEditorWidget\0"
    "gradientChanged(QGradientStops)\0\0"
    "setColorGradient(QGradientStops)\0"
    "setGeneralLock(GradientEditor::LockType)\0"
    "setDrawBox(bool)\0updateColorGradient()\0"
};

void GradientEditorWidget::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        GradientEditorWidget *_t = static_cast<GradientEditorWidget *>(_o);
        switch (_id) {
        case 0: _t->gradientChanged((*reinterpret_cast< QGradientStops(*)>(_a[1]))); break;
        case 1: _t->setColorGradient((*reinterpret_cast< QGradientStops(*)>(_a[1]))); break;
        case 2: _t->setGeneralLock((*reinterpret_cast< GradientEditor::LockType(*)>(_a[1]))); break;
        case 3: _t->setDrawBox((*reinterpret_cast< bool(*)>(_a[1]))); break;
        case 4: _t->updateColorGradient(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData GradientEditorWidget::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject GradientEditorWidget::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_GradientEditorWidget,
      qt_meta_data_GradientEditorWidget, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &GradientEditorWidget::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *GradientEditorWidget::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *GradientEditorWidget::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_GradientEditorWidget))
        return static_cast<void*>(const_cast< GradientEditorWidget*>(this));
    return QWidget::qt_metacast(_clname);
}

int GradientEditorWidget::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void GradientEditorWidget::gradientChanged(QGradientStops _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}
QT_END_MOC_NAMESPACE
