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
    uint offsetsAndSizes[34];
    char stringdata0[13];
    char stringdata1[14];
    char stringdata2[1];
    char stringdata3[6];
    char stringdata4[6];
    char stringdata5[17];
    char stringdata6[7];
    char stringdata7[13];
    char stringdata8[8];
    char stringdata9[12];
    char stringdata10[12];
    char stringdata11[12];
    char stringdata12[20];
    char stringdata13[14];
    char stringdata14[29];
    char stringdata15[6];
    char stringdata16[17];
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
        QT_MOC_LITERAL(64, 12),  // "reconnecting"
        QT_MOC_LITERAL(77, 7),  // "attempt"
        QT_MOC_LITERAL(85, 11),  // "maxAttempts"
        QT_MOC_LITERAL(97, 11),  // "onReadyRead"
        QT_MOC_LITERAL(109, 11),  // "onHeartbeat"
        QT_MOC_LITERAL(121, 19),  // "onConnectionTimeout"
        QT_MOC_LITERAL(141, 13),  // "onSocketError"
        QT_MOC_LITERAL(155, 28),  // "QAbstractSocket::SocketError"
        QT_MOC_LITERAL(184, 5),  // "error"
        QT_MOC_LITERAL(190, 16)   // "attemptReconnect"
    },
    "WeChatSocket",
    "frameReceived",
    "",
    "Frame",
    "frame",
    "connectionFailed",
    "reason",
    "reconnecting",
    "attempt",
    "maxAttempts",
    "onReadyRead",
    "onHeartbeat",
    "onConnectionTimeout",
    "onSocketError",
    "QAbstractSocket::SocketError",
    "error",
    "attemptReconnect"
};
#undef QT_MOC_LITERAL
} // unnamed namespace

Q_CONSTINIT static const uint qt_meta_data_WeChatSocket[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   62,    2, 0x06,    1 /* Public */,
       5,    1,   65,    2, 0x06,    3 /* Public */,
       7,    2,   68,    2, 0x06,    5 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
      10,    0,   73,    2, 0x08,    8 /* Private */,
      11,    0,   74,    2, 0x08,    9 /* Private */,
      12,    0,   75,    2, 0x08,   10 /* Private */,
      13,    1,   76,    2, 0x08,   11 /* Private */,
      16,    0,   79,    2, 0x08,   13 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QString,    6,
    QMetaType::Void, QMetaType::Int, QMetaType::Int,    8,    9,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 14,   15,
    QMetaType::Void,

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
        // method 'reconnecting'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        QtPrivate::TypeAndForceComplete<int, std::false_type>,
        // method 'onReadyRead'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onHeartbeat'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onConnectionTimeout'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSocketError'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<QAbstractSocket::SocketError, std::false_type>,
        // method 'attemptReconnect'
        QtPrivate::TypeAndForceComplete<void, std::false_type>
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
        case 2: _t->reconnecting((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2]))); break;
        case 3: _t->onReadyRead(); break;
        case 4: _t->onHeartbeat(); break;
        case 5: _t->onConnectionTimeout(); break;
        case 6: _t->onSocketError((*reinterpret_cast< std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        case 7: _t->attemptReconnect(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 6:
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
        {
            using _t = void (WeChatSocket::*)(int , int );
            if (_t _q_method = &WeChatSocket::reconnecting; *reinterpret_cast<_t *>(_a[1]) == _q_method) {
                *result = 2;
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
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
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

// SIGNAL 2
void WeChatSocket::reconnecting(int _t1, int _t2)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))), const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t2))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
