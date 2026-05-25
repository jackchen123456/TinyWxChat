#include "MessageBuilder.h"

// ── 辅助 ──────────────────────────────────────────────

QString MessageBuilder::messageType(const QJsonObject& payload) {
    return payload.value("type").toString();
}

QJsonObject MessageBuilder::bodyFrom(const QJsonObject& payload) {
    return payload.value("body").toObject();
}

// ============================================================
//  Phase 1
// ============================================================

QJsonObject MessageBuilder::buildLogin(const QString& username, const QString& password) {
    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    QJsonObject frame;
    frame["type"] = MsgType::LOGIN;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

LoginResponse MessageBuilder::parseLoginResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    LoginResponse r;
    r.code     = b.value("code").toInt(-1);
    r.userId   = b.value("user_id").toInt();
    r.nickname = b.value("nickname").toString();
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    return r;
}

QJsonObject MessageBuilder::buildRegister(const QString& username, const QString& password,
                                           const QString& nickname) {
    QJsonObject body;
    body["username"] = username;
    body["password"] = password;
    body["nickname"] = nickname;
    QJsonObject frame;
    frame["type"] = MsgType::REGISTER;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

RegisterResponse MessageBuilder::parseRegisterResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    RegisterResponse r;
    r.code     = b.value("code").toInt(-1);
    r.userId   = b.value("user_id").toInt();
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    return r;
}

QJsonObject MessageBuilder::buildChatSend(int toUserId, const QString& content,
                                           int msgType, const QString& extra) {
    QJsonObject body;
    body["to_user_id"] = toUserId;
    body["content"]    = content;
    body["msg_type"]   = msgType;
    if (!extra.isEmpty()) body["extra"] = extra;
    QJsonObject frame;
    frame["type"] = MsgType::SEND;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

ChatSendResponse MessageBuilder::parseChatSendResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    ChatSendResponse r;
    r.code      = b.value("code").toInt(-1);
    r.msgId     = b.value("msg_id").toInteger();
    r.timestamp = b.value("timestamp").toInteger();
    r.errorMsg  = b.value("message").toString();
    r.ok        = (r.code == 0);
    return r;
}

QJsonObject MessageBuilder::buildChatHistory(int withUserId, int beforeMsgId, int limit) {
    QJsonObject body;
    body["with_user_id"]  = withUserId;
    body["before_msg_id"] = beforeMsgId;
    body["limit"]         = limit;
    QJsonObject frame;
    frame["type"] = MsgType::HISTORY;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

ChatHistoryResponse MessageBuilder::parseChatHistoryResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    ChatHistoryResponse r;
    r.code     = b.value("code").toInt(-1);
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    if (r.ok) {
        QJsonArray msgs = b.value("messages").toArray();
        for (const QJsonValue& v : msgs) {
            QJsonObject m = v.toObject();
            ChatHistoryItem item;
            item.msgId      = m.value("msg_id").toInteger();
            item.fromUserId = m.value("from_user_id").toInt();
            item.toUserId   = m.value("to_user_id").toInt(-1);
            item.content    = m.value("content").toString();
            item.msgType    = m.value("msg_type").toInt(1);
            item.timestamp  = m.value("timestamp").toInteger();
            item.extra      = m.value("extra").toString();
            r.messages.push_back(item);
        }
    }
    return r;
}

ChatRecv MessageBuilder::parseChatRecv(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    ChatRecv r;
    r.fromUserId   = b.value("from_user_id").toInt();
    r.fromNickname = b.value("from_nickname").toString();
    r.msgId        = b.value("msg_id").toInteger();
    r.content      = b.value("content").toString();
    r.msgType      = b.value("msg_type").toInt(1);
    r.timestamp    = b.value("timestamp").toInteger();
    r.extra        = b.value("extra").toString();
    return r;
}

// ============================================================
//  Phase 2：好友
// ============================================================

QJsonObject MessageBuilder::buildFriendRequest(int toUserId, const QString& message) {
    QJsonObject body;
    body["to_user_id"] = toUserId;
    if (!message.isEmpty()) body["message"] = message;
    QJsonObject frame;
    frame["type"] = MsgType::FRIEND_REQUEST;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

FriendRequestResponse MessageBuilder::parseFriendRequestResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    FriendRequestResponse r;
    r.code     = b.value("code").toInt(-1);
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    return r;
}

FriendNotify MessageBuilder::parseFriendNotify(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    FriendNotify n;
    n.requestId    = b.value("request_id").toInt();
    n.fromUserId   = b.value("from_user_id").toInt();
    n.fromNickname = b.value("from_nickname").toString();
    n.message      = b.value("message").toString();
    n.timestamp    = b.value("timestamp").toInteger();
    return n;
}

QJsonObject MessageBuilder::buildFriendHandle(int requestId, const QString& action) {
    QJsonObject body;
    body["request_id"] = requestId;
    body["action"]     = action;  // "accept" or "reject"
    QJsonObject frame;
    frame["type"] = MsgType::FRIEND_HANDLE;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

FriendHandleResponse MessageBuilder::parseFriendHandleResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    FriendHandleResponse r;
    r.code     = b.value("code").toInt(-1);
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    return r;
}

QJsonObject MessageBuilder::buildFriendList() {
    QJsonObject frame;
    frame["type"] = MsgType::FRIEND_LIST;
    frame["seq"]  = 0;
    frame["body"] = QJsonObject();
    return frame;
}

FriendListResponse MessageBuilder::parseFriendListResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    FriendListResponse r;
    r.code     = b.value("code").toInt(-1);
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    if (r.ok) {
        QJsonArray arr = b.value("friends").toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject f = v.toObject();
            FriendInfo fi;
            fi.userId   = f.value("user_id").toInt();
            fi.nickname = f.value("nickname").toString();
            r.friends.push_back(fi);
        }
    }
    return r;
}

QJsonObject MessageBuilder::buildFriendPending() {
    QJsonObject frame;
    frame["type"] = MsgType::FRIEND_PENDING;
    frame["seq"]  = 0;
    frame["body"] = QJsonObject();
    return frame;
}

FriendPendingResponse MessageBuilder::parseFriendPendingResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    FriendPendingResponse r;
    r.code     = b.value("code").toInt(-1);
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    if (r.ok) {
        QJsonArray arr = b.value("requests").toArray();
        for (const QJsonValue& v : arr) {
            QJsonObject req = v.toObject();
            PendingRequest pr;
            pr.requestId    = req.value("request_id").toInt();
            pr.fromUserId   = req.value("from_user_id").toInt();
            pr.fromNickname = req.value("from_nickname").toString();
            pr.message      = req.value("message").toString();
            pr.timestamp    = req.value("timestamp").toInteger();
            r.requests.push_back(pr);
        }
    }
    return r;
}

// ============================================================
//  Phase 3：群聊
// ============================================================

QJsonObject MessageBuilder::buildGroupCreate(const QString& name) {
    QJsonObject body;
    body["name"] = name;
    QJsonObject frame;
    frame["type"] = MsgType::GROUP_CREATE;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

GroupCreateResponse MessageBuilder::parseGroupCreateResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    GroupCreateResponse r;
    r.code     = b.value("code").toInt(-1);
    r.groupId  = b.value("group_id").toInt();
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    return r;
}

QJsonObject MessageBuilder::buildGroupJoin(int groupId) {
    QJsonObject body;
    body["group_id"] = groupId;
    QJsonObject frame;
    frame["type"] = MsgType::GROUP_JOIN;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

GroupJoinResponse MessageBuilder::parseGroupJoinResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    GroupJoinResponse r;
    r.code     = b.value("code").toInt(-1);
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    return r;
}

QJsonObject MessageBuilder::buildGroupSend(int groupId, const QString& content,
                                            int msgType, const QString& extra) {
    QJsonObject body;
    body["group_id"] = groupId;
    body["content"]  = content;
    body["msg_type"] = msgType;
    if (!extra.isEmpty()) body["extra"] = extra;
    QJsonObject frame;
    frame["type"] = MsgType::GROUP_SEND;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

GroupSendResponse MessageBuilder::parseGroupSendResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    GroupSendResponse r;
    r.code      = b.value("code").toInt(-1);
    r.msgSeq    = b.value("msg_seq").toInt();
    r.timestamp = b.value("timestamp").toInteger();
    r.errorMsg  = b.value("message").toString();
    r.ok        = (r.code == 0);
    return r;
}

GroupRecv MessageBuilder::parseGroupRecv(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    GroupRecv r;
    r.groupId      = b.value("group_id").toInt();
    r.fromUserId   = b.value("from_user_id").toInt();
    r.fromNickname = b.value("from_nickname").toString();
    r.msgSeq       = b.value("msg_seq").toInt();
    r.content      = b.value("content").toString();
    r.msgType      = b.value("msg_type").toInt(1);
    r.timestamp    = b.value("timestamp").toInteger();
    r.extra        = b.value("extra").toString();
    return r;
}

QJsonObject MessageBuilder::buildGroupHistory(int groupId, int beforeMsgSeq, int limit) {
    QJsonObject body;
    body["group_id"]       = groupId;
    body["before_msg_seq"] = beforeMsgSeq;
    body["limit"]          = limit;
    QJsonObject frame;
    frame["type"] = MsgType::GROUP_HISTORY;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

GroupHistoryResponse MessageBuilder::parseGroupHistoryResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    GroupHistoryResponse r;
    r.code     = b.value("code").toInt(-1);
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    if (r.ok) {
        QJsonArray msgs = b.value("messages").toArray();
        for (const QJsonValue& v : msgs) {
            QJsonObject m = v.toObject();
            GroupHistoryItem item;
            item.msgSeq       = m.value("msg_seq").toInt();
            item.groupId      = m.value("group_id").toInt();
            item.fromUserId   = m.value("from_user_id").toInt();
            item.fromNickname = m.value("from_nickname").toString();
            item.content      = m.value("content").toString();
            item.msgType      = m.value("msg_type").toInt(1);
            item.timestamp    = m.value("timestamp").toInteger();
            item.extra        = m.value("extra").toString();
            r.messages.push_back(item);
        }
    }
    return r;
}

QJsonObject MessageBuilder::buildGroupApply(int groupId, const QString& message) {
    QJsonObject body;
    body["group_id"] = groupId;
    if (!message.isEmpty()) body["message"] = message;
    QJsonObject frame;
    frame["type"] = MsgType::GROUP_APPLY;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

GroupApplyResponse MessageBuilder::parseGroupApplyResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    GroupApplyResponse r;
    r.code     = b.value("code").toInt(-1);
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    return r;
}

GroupApplyNotify MessageBuilder::parseGroupApplyNotify(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    GroupApplyNotify n;
    n.requestId    = b.value("request_id").toInt();
    n.groupId      = b.value("group_id").toInt();
    n.groupName    = b.value("group_name").toString();
    n.fromUserId   = b.value("from_user_id").toInt();
    n.fromNickname = b.value("from_nickname").toString();
    n.message      = b.value("message").toString();
    n.timestamp    = b.value("timestamp").toInteger();
    return n;
}

QJsonObject MessageBuilder::buildGroupApplyHandle(int requestId, const QString& action) {
    QJsonObject body;
    body["request_id"] = requestId;
    body["action"]     = action;  // "accept" or "reject"
    QJsonObject frame;
    frame["type"] = MsgType::GROUP_APPLY_HANDLE;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

GroupApplyHandleResponse MessageBuilder::parseGroupApplyHandleResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    GroupApplyHandleResponse r;
    r.code     = b.value("code").toInt(-1);
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    return r;
}

QJsonObject MessageBuilder::buildGroupList() {
    QJsonObject body;
    QJsonObject frame;
    frame["type"] = MsgType::GROUP_LIST;
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

GroupListResponse MessageBuilder::parseGroupListResponse(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    GroupListResponse r;
    r.code = b.value("code").toInt(-1);
    r.ok   = (r.code == 0);
    QJsonArray arr = b.value("groups").toArray();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        GroupListItem g;
        g.groupId   = obj.value("group_id").toInt();
        g.groupName = obj.value("group_name").toString();
        r.groups.push_back(g);
    }
    return r;
}

// ============================================================
//  通用
// ============================================================

ErrorNotification MessageBuilder::parseError(const QJsonObject& payload) {
    QJsonObject b = bodyFrom(payload);
    ErrorNotification e;
    e.code    = b.value("code").toInt(-1);
    e.message = b.value("message").toString();
    return e;
}
