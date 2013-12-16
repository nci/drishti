/****************************************************************************
** Meta object code from reading C++ file 'drishtipaint.h'
**
** Created: Tue Dec 17 10:58:12 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "drishtipaint.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'drishtipaint.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_DrishtiPaint[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      34,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      13,   30,   30,   30, 0x08,
      31,   30,   30,   30, 0x08,
      57,   30,   30,   30, 0x08,
      82,   30,   30,   30, 0x08,
     108,   30,   30,   30, 0x08,
     134,   30,   30,   30, 0x08,
     166,   30,   30,   30, 0x08,
     184,   30,   30,   30, 0x08,
     202,   30,   30,   30, 0x08,
     220,   30,   30,   30, 0x08,
     245,   30,   30,   30, 0x08,
     274,   30,   30,   30, 0x08,
     302,   30,   30,   30, 0x08,
     333,   30,   30,   30, 0x08,
     361,   30,   30,   30, 0x08,
     383,  430,   30,   30, 0x08,
     432,  464,   30,   30, 0x08,
     467,   30,   30,   30, 0x08,
     485,   30,   30,   30, 0x08,
     499,   30,   30,   30, 0x08,
     517,  464,   30,   30, 0x08,
     542,  430,   30,   30, 0x08,
     564,  430,   30,   30, 0x08,
     586,  430,   30,   30, 0x08,
     608,  430,   30,   30, 0x08,
     630,  430,   30,   30, 0x08,
     652,  430,   30,   30, 0x08,
     674,  726,   30,   30, 0x08,
     734,  773,   30,   30, 0x08,
     779,   30,   30,   30, 0x08,
     788,  773,   30,   30, 0x08,
     820,   30,   30,   30, 0x08,
     828,  773,   30,   30, 0x08,
     859,  464,   30,   30, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_DrishtiPaint[] = {
    "DrishtiPaint\0openRecentFile()\0\0"
    "on_actionHelp_triggered()\0"
    "on_saveImage_triggered()\0"
    "on_actionLoad_triggered()\0"
    "on_actionExit_triggered()\0"
    "on_actionExtractTag_triggered()\0"
    "on_butZ_clicked()\0on_butY_clicked()\0"
    "on_butX_clicked()\0on_tag_valueChanged(int)\0"
    "on_boxSize_valueChanged(int)\0"
    "on_lambda_valueChanged(int)\0"
    "on_preverode_valueChanged(int)\0"
    "on_smooth_valueChanged(int)\0"
    "on_copyprev_clicked()\0"
    "changeTransferFunctionDisplay(int,QList<bool>)\0"
    ",\0checkStateChanged(int,int,bool)\0,,\0"
    "updateComposite()\0getSlice(int)\0"
    "getMaskSlice(int)\0getRawValue(int,int,int)\0"
    "tagDSlice(int,QImage)\0tagWSlice(int,QImage)\0"
    "tagHSlice(int,QImage)\0tagDSlice(int,uchar*)\0"
    "tagWSlice(int,uchar*)\0tagHSlice(int,uchar*)\0"
    "fillVolume(int,int,int,int,int,int,QList<int>,bool)\0"
    ",,,,,,,\0tagAllVisible(int,int,int,int,int,int)\0"
    ",,,,,\0dilate()\0dilate(int,int,int,int,int,int)\0"
    "erode()\0erode(int,int,int,int,int,int)\0"
    "applyMaskOperation(int,int,int)\0"
};

void DrishtiPaint::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        DrishtiPaint *_t = static_cast<DrishtiPaint *>(_o);
        switch (_id) {
        case 0: _t->openRecentFile(); break;
        case 1: _t->on_actionHelp_triggered(); break;
        case 2: _t->on_saveImage_triggered(); break;
        case 3: _t->on_actionLoad_triggered(); break;
        case 4: _t->on_actionExit_triggered(); break;
        case 5: _t->on_actionExtractTag_triggered(); break;
        case 6: _t->on_butZ_clicked(); break;
        case 7: _t->on_butY_clicked(); break;
        case 8: _t->on_butX_clicked(); break;
        case 9: _t->on_tag_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 10: _t->on_boxSize_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 11: _t->on_lambda_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 12: _t->on_preverode_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 13: _t->on_smooth_valueChanged((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 14: _t->on_copyprev_clicked(); break;
        case 15: _t->changeTransferFunctionDisplay((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QList<bool>(*)>(_a[2]))); break;
        case 16: _t->checkStateChanged((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< bool(*)>(_a[3]))); break;
        case 17: _t->updateComposite(); break;
        case 18: _t->getSlice((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 19: _t->getMaskSlice((*reinterpret_cast< int(*)>(_a[1]))); break;
        case 20: _t->getRawValue((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        case 21: _t->tagDSlice((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QImage(*)>(_a[2]))); break;
        case 22: _t->tagWSlice((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QImage(*)>(_a[2]))); break;
        case 23: _t->tagHSlice((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< QImage(*)>(_a[2]))); break;
        case 24: _t->tagDSlice((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< uchar*(*)>(_a[2]))); break;
        case 25: _t->tagWSlice((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< uchar*(*)>(_a[2]))); break;
        case 26: _t->tagHSlice((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< uchar*(*)>(_a[2]))); break;
        case 27: _t->fillVolume((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5])),(*reinterpret_cast< int(*)>(_a[6])),(*reinterpret_cast< QList<int>(*)>(_a[7])),(*reinterpret_cast< bool(*)>(_a[8]))); break;
        case 28: _t->tagAllVisible((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5])),(*reinterpret_cast< int(*)>(_a[6]))); break;
        case 29: _t->dilate(); break;
        case 30: _t->dilate((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5])),(*reinterpret_cast< int(*)>(_a[6]))); break;
        case 31: _t->erode(); break;
        case 32: _t->erode((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3])),(*reinterpret_cast< int(*)>(_a[4])),(*reinterpret_cast< int(*)>(_a[5])),(*reinterpret_cast< int(*)>(_a[6]))); break;
        case 33: _t->applyMaskOperation((*reinterpret_cast< int(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2])),(*reinterpret_cast< int(*)>(_a[3]))); break;
        default: ;
        }
    }
}

const QMetaObjectExtraData DrishtiPaint::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject DrishtiPaint::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_DrishtiPaint,
      qt_meta_data_DrishtiPaint, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &DrishtiPaint::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *DrishtiPaint::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *DrishtiPaint::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_DrishtiPaint))
        return static_cast<void*>(const_cast< DrishtiPaint*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int DrishtiPaint::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 34)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 34;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
