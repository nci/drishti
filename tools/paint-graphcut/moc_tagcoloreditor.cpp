/****************************************************************************
** Meta object code from reading C++ file 'tagcoloreditor.h'
**
** Created: Tue Dec 17 10:58:35 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "tagcoloreditor.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tagcoloreditor.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_TagColorEditor[] = {

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
      15,   33,   33,   33, 0x05,

 // slots: signature, parameters, type, tag, flags
      34,   33,   33,   33, 0x0a,
      46,   33,   33,   33, 0x0a,
      63,   84,   33,   33, 0x0a,
      86,   33,   33,   33, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_TagColorEditor[] = {
    "TagColorEditor\0tagColorChanged()\0\0"
    "setColors()\0newColorSet(int)\0"
    "cellClicked(int,int)\0,\0newTagsClicked()\0"
};

void TagColorEditor::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        TagColorEditor *_t = static_cast<TagColorEditor *>(_o);
        switch (_id) {
        case 0: _t->tagColorChanged(); break;
        case 1: _t->setColors(); break;
        case 2: _t->newColorSet((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 3: _t->cellClicked((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->newTagsClicked(); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData TagColorEditor::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject TagColorEditor::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_TagColorEditor,
      qt_meta_data_TagColorEditor, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &TagColorEditor::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *TagColorEditor::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *TagColorEditor::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_TagColorEditor))
        return static_cast<void*>(const_cast< TagColorEditor*>(this));
    return QWidget::qt_metacast(_clname);
}

int TagColorEditor::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
void TagColorEditor::tagColorChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, 0);
}
QT_END_MOC_NAMESPACE
