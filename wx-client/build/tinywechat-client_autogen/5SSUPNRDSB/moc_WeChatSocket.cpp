/****************************************************************************
** Meta object code from reading C++ file 'WeChatSocket.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.4.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../network/WeChatSocket.h"
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'WeChatSocket.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.4.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
namespace {
struct qt_meta_stringdata_WeChatSocket_t {
    uint offsetsAndSizes[26];
    char stringdata0[13];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[6];
    char stringdata4[6];
    char stringdata5[17];
    char stringdata6[7];
    char stringdata7[12];
    char stringdata8[12];
    char stringdata9[20];
    char stringdata10[14];
    char stringdata11[29];
    char stringdata12[6];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(sizeof(qt_meta_stringdata_WeChatSocket_t::offsetsAndSizes) + ofs), len 
Q_CONSTINIT static const qt_meta_stringdata_WeChatSocket_t qt_meta_stringdata_WeChatSocket = {
    {
        QT_MOC_LITERAL(0, 12),  // "WeChatSocket"
        QT_MOC_LITERAL(13, 13),  // "frameReceived"
        QT_MOC_LITERAL(27, 0),  // ""
        QT_MOC_LITERAL(28, 5),  // "Frame"
        QT_MOC_LITERAL(34, 5),  // "frame"
        QT_MOC_LITERAL(40, 16),  // "connectionFailed"
        QT_MOC_LITERAL(57, 6),  // "reason"
        QT_MOC_LITERAL(64, 11),  // "onReadyRead"
        QT_MOC_LITERAL(76, 11),  // "onHeartbeat"
        QT_MOC_LITERAL(88, 19),  // "onConnectionTimeout"
        QT_MOC_LITERAL(108, 13),  // "onSocketError"
        QT_MOC_LITERAL(122, 28),  // "QAbstractSocket::SocketError"
        QT_MOC_LITERAL(151, 5)   // "error"
    },
    "WeChatSocket",
    "frameReceived",
    "",
    "Frame",
    "frame",
    "connectionFailed",
    "reason",
    "onReadyRead",
    "onHeartbeat",
    "onConnectionTimeout",
    "onSocketError",
    "QAbstractSocket::SocketError",
    "error"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_WeChatSocket[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   50,    2, 0x06,    1 /* Public */,
       5,    1,   53,    2, 0x06,    3 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       7,    0,   56,    2, 0x08,    5 /* Private */,
       8,    0,   57,    2, 0x08,    6 /* Private */,
       9,    0,   58,    2, 0x08,    7 /* Private */,
      10,    1,   59,    2, 0x08,    8 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 11,   12,

       0        // eod
};

Q_CONSTINIT const QMetaObject WeChatSocket::staticMetaObject = { {
    QMetaObject::SuperData::link<QTcpSocket::staticMetaObject>(),
    qt_meta_stringdata_WeChatSocket.offsetsAndSizes,
    qt_meta_data_WeChatSocket,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_stringdata_WeChatSocket_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<WeChatSocket, std::true_type>,
        // method 'frameReceived'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const Frame &, std::false_type>,
        // method 'connectionFailed'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onHeartbeat'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onConnectionTimeout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSocketError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QAbstractSocket::SocketError, std::false_type>
    >,
    nullptr
} };

void WeChatSocket::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<WeChatSocket *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->frameReceived((*reinterpret_cast< std::add_pointer_t<Frame>>(_a[1]))); break;
        case 1: _t->connectionFailed((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->onReadyRead(); break;
        case 3: _t->onHeartbeat(); break;
        case 4: _t->onConnectionTimeout(); break;
        case 5: _t->onSocketError((*reinterpret_cast< std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (WeChatSocket::*)(const Frame & );
            if (_t _q_method = &WeChatSocket::frameReceived; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (WeChatSocket::*)(const QString & );
            if (_t _q_method = &WeChatSocket::connectionFailed; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 1;
                return;
            }
        }
    }
}

const QMetaObject *WeChatSocket::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *WeChatSocket::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_WeChatSocket.stringdata0))
        return static_cast<void*>(this);
    return QTcpSocket::qt_metacast(_clname);
}

int WeChatSocket::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QTcpSocket::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void WeChatSocket::frameReceived(const Frame & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void WeChatSocket::connectionFailed(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
