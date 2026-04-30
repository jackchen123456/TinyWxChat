#include "MessageBuilder.h"

// ── 构造请求 ──────────────────────────────────────────

QJsonObject MessageBuilder::buildLogin(const QString& username, const QString& password)
{
    QJsonObject body;
    body["username"] = username;
    body["password"] = password;

    QJsonObject frame;
    frame["type"] = QStringLiteral("auth.login");
    frame["seq"]  = 0;  // 调用方负责填充
    frame["body"] = body;
    return frame;
}

QJsonObject MessageBuilder::buildChatSend(int toUserId, const QString& content)
{
    QJsonObject body;
    body["to_user_id"] = toUserId;
    body["content"]    = content;
    body["msg_type"]   = 1;  // 文本

    QJsonObject frame;
    frame["type"] = QStringLiteral("chat.send");
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

QJsonObject MessageBuilder::buildChatHistory(int withUserId, int beforeMsgId, int limit)
{
    QJsonObject body;
    body["with_user_id"]  = withUserId;
    body["before_msg_id"] = beforeMsgId;
    body["limit"]         = limit;

    QJsonObject frame;
    frame["type"] = QStringLiteral("chat.history");
    frame["seq"]  = 0;
    frame["body"] = body;
    return frame;
}

// ── 解析响应 ──────────────────────────────────────────
// 服务端响应格式：{ "type": "...", "seq": N, "body": { ... } }
// 本层从 payload["body"] 中提取实际数据

LoginResponse MessageBuilder::parseLoginResponse(const QJsonObject& payload)
{
    QJsonObject b = payload.value("body").toObject();
    LoginResponse r;
    r.code     = b.value("code").toInt(-1);
    r.userId   = b.value("user_id").toInt();
    r.nickname = b.value("nickname").toString();
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);
    return r;
}

ChatSendResponse MessageBuilder::parseChatSendResponse(const QJsonObject& payload)
{
    QJsonObject b = payload.value("body").toObject();
    ChatSendResponse r;
    r.code      = b.value("code").toInt(-1);
    r.msgId     = b.value("msg_id").toInt();
    r.timestamp = static_cast<int64_t>(b.value("timestamp").toDouble());
    r.errorMsg  = b.value("message").toString();
    r.ok        = (r.code == 0);
    return r;
}

ChatHistoryResponse MessageBuilder::parseChatHistoryResponse(const QJsonObject& payload)
{
    QJsonObject b = payload.value("body").toObject();
    ChatHistoryResponse r;
    r.code     = b.value("code").toInt(-1);
    r.errorMsg = b.value("message").toString();
    r.ok       = (r.code == 0);

    if (r.ok) {
        QJsonArray msgs = b.value("messages").toArray();
        for (const QJsonValue& v : msgs) {
            QJsonObject m = v.toObject();
            ChatHistoryItem item;
            item.msgId      = m.value("msg_id").toInt();
            item.fromUserId = m.value("from_user_id").toInt();
            item.toUserId   = m.value("to_user_id").toInt(-1);
            item.content    = m.value("content").toString();
            item.msgType    = m.value("msg_type").toInt(1);
            item.timestamp  = static_cast<int64_t>(m.value("timestamp").toDouble());
            r.messages.push_back(item);
        }
    }
    return r;
}

// chat.recv 是 NOTIFICATION 帧。服务端推送格式可能是：
//   { "type": "chat.recv", "body": { ... } }
// 或直接 { "from_user_id": 1, ... }（取决于实现）
// 这里先尝试 body 包装，失败则回退到根级别
ChatRecv MessageBuilder::parseChatRecv(const QJsonObject& payload)
{
    QJsonObject b = payload.value("body").toObject();
    if (b.isEmpty()) b = payload;  // 回退：无 body 包装时直接读根级别

    ChatRecv r;
    r.fromUserId   = b.value("from_user_id").toInt();
    r.fromNickname = b.value("from_nickname").toString();
    r.msgId        = b.value("msg_id").toInt();
    r.content      = b.value("content").toString();
    r.msgType      = b.value("msg_type").toInt(1);
    r.timestamp    = static_cast<int64_t>(b.value("timestamp").toDouble());
    return r;
}

// error 通知（NOTIFICATION 帧）。也可能是 body 包装
ErrorNotification MessageBuilder::parseError(const QJsonObject& payload)
{
    QJsonObject b = payload.value("body").toObject();
    if (b.isEmpty()) b = payload;

    ErrorNotification e;
    e.code    = b.value("code").toInt(-1);
    e.message = b.value("message").toString();
    return e;
}

// ── 辅助 ──────────────────────────────────────────────

QString MessageBuilder::messageType(const QJsonObject& payload)
{
    return payload.value("type").toString();
}
