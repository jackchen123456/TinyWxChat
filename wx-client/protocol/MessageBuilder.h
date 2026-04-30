#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <cstdint>
#include <vector>

// ─── 消息结构体 ───────────────────────────────────────

struct LoginResponse {
    bool   ok = false;
    int    code = 0;
    int    userId = 0;
    QString nickname;
    QString errorMsg;
};

struct ChatSendResponse {
    bool   ok = false;
    int    code = 0;
    int    msgId = 0;
    int64_t timestamp = 0;
    QString errorMsg;
};

struct ChatRecv {
    int    fromUserId;
    QString fromNickname;
    int    msgId;
    QString content;
    int    msgType = 0;   // 1=文本
    int64_t timestamp;
};

struct ChatHistoryItem {
    int    msgId;
    int    fromUserId;
    int    toUserId;
    QString content;
    int    msgType = 1;
    int64_t timestamp;
};

struct ChatHistoryResponse {
    bool   ok = false;
    int    code = 0;
    std::vector<ChatHistoryItem> messages;
    QString errorMsg;
};

struct ErrorNotification {
    int    code = 0;
    QString message;
};

// ─── 消息构造与解析 ──────────────────────────────────

class MessageBuilder {
public:
    // ── 请求构造（返回 QJsonObject 可直接作为 Frame.payload）──

    static QJsonObject buildLogin(const QString& username, const QString& password);
    static QJsonObject buildChatSend(int toUserId, const QString& content);
    static QJsonObject buildChatHistory(int withUserId, int beforeMsgId = 0, int limit = 20);

    // ── 响应解析 ─────────────────────────────────────

    static LoginResponse parseLoginResponse(const QJsonObject& payload);
    static ChatSendResponse parseChatSendResponse(const QJsonObject& payload);
    static ChatHistoryResponse parseChatHistoryResponse(const QJsonObject& payload);

    // 解析服务端推送的单聊消息（chat.recv）
    static ChatRecv parseChatRecv(const QJsonObject& payload);

    // 解析通用错误通知
    static ErrorNotification parseError(const QJsonObject& payload);

private:
    // 从 payload 中提取 "type" 字符串
    static QString messageType(const QJsonObject& payload);
};
