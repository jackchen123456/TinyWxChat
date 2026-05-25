#pragma once

#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <cstdint>
#include <vector>

#include "protocol/common.h"   // MsgType / ErrCode / FrameType（共享自服务端）

// ─── 通用结构体 ───────────────────────────────────────

struct ErrorNotification {
    int     code = 0;
    QString message;
};

// ─── Phase 1：登录/单聊 ───────────────────────────────

struct LoginResponse {
    bool    ok = false;
    int     code = 0;
    int     userId = 0;
    QString nickname;
    QString errorMsg;
};

struct RegisterResponse {
    bool ok = false;
    int  code = 0;
    int  userId = 0;
    QString errorMsg;
};

struct ChatSendResponse {
    bool    ok = false;
    int     code = 0;
    int64_t msgId = 0;
    int64_t timestamp = 0;
    QString errorMsg;
};

struct ChatRecv {
    int     fromUserId = 0;
    QString fromNickname;
    int64_t msgId = 0;
    QString content;
    int     msgType = 0;   // 1=文本, 2=表情, 3=图片
    int64_t timestamp = 0;
    QString extra;         // 扩展字段（表情ID/图片路径）
};

struct ChatHistoryItem {
    int64_t msgId = 0;
    int     fromUserId = 0;
    int     toUserId = 0;
    QString content;
    int     msgType = 1;
    int64_t timestamp = 0;
    QString extra;
};

struct ChatHistoryResponse {
    bool ok = false;
    int  code = 0;
    std::vector<ChatHistoryItem> messages;
    QString errorMsg;
};

// ─── Phase 2：好友系统 ────────────────────────────────

struct FriendRequestResponse {
    bool ok = false;
    int  code = 0;
    QString errorMsg;
};

struct FriendNotify {
    int     requestId = 0;
    int     fromUserId = 0;
    QString fromNickname;
    QString message;
    int64_t timestamp = 0;
};

struct FriendHandleResponse {
    bool ok = false;
    int  code = 0;
    QString errorMsg;
};

struct FriendInfo {
    int     userId = 0;
    QString nickname;
};

struct FriendListResponse {
    bool ok = false;
    int  code = 0;
    std::vector<FriendInfo> friends;
    QString errorMsg;
};

struct PendingRequest {
    int     requestId = 0;
    int     fromUserId = 0;
    QString fromNickname;
    QString message;
    int64_t timestamp = 0;
};

struct FriendPendingResponse {
    bool ok = false;
    int  code = 0;
    std::vector<PendingRequest> requests;
    QString errorMsg;
};

// ─── Phase 3：群聊 ────────────────────────────────────

struct GroupCreateResponse {
    bool ok = false;
    int  code = 0;
    int  groupId = 0;
    QString errorMsg;
};

struct GroupJoinResponse {
    bool ok = false;
    int  code = 0;
    QString errorMsg;
};

struct GroupSendResponse {
    bool    ok = false;
    int     code = 0;
    int     msgSeq = 0;
    int64_t timestamp = 0;
    QString errorMsg;
};

struct GroupRecv {
    int     groupId = 0;
    int     fromUserId = 0;
    QString fromNickname;
    int     msgSeq = 0;
    QString content;
    int     msgType = 1;
    int64_t timestamp = 0;
    QString extra;
};

struct GroupHistoryItem {
    int     msgSeq = 0;
    int     groupId = 0;
    int     fromUserId = 0;
    QString fromNickname;
    QString content;
    int     msgType = 1;
    int64_t timestamp = 0;
    QString extra;
};

struct GroupHistoryResponse {
    bool ok = false;
    int  code = 0;
    std::vector<GroupHistoryItem> messages;
    QString errorMsg;
};

struct GroupApplyResponse {
    bool ok = false;
    int  code = 0;
    QString errorMsg;
};

struct GroupApplyNotify {
    int     requestId = 0;
    int     groupId = 0;
    QString groupName;
    int     fromUserId = 0;
    QString fromNickname;
    QString message;
    int64_t timestamp = 0;
};

struct GroupApplyHandleResponse {
    bool ok = false;
    int  code = 0;
    QString errorMsg;
};

struct GroupListItem {
    int     groupId = 0;
    QString groupName;
};

struct GroupListResponse {
    bool ok = false;
    int  code = 0;
    std::vector<GroupListItem> groups;
};

// ─── 消息构造与解析 ──────────────────────────────────

class MessageBuilder {
public:
    // ── Phase 1：登录/注册/单聊 ───────────────────────

    static QJsonObject buildLogin(const QString& username, const QString& password);
    static LoginResponse parseLoginResponse(const QJsonObject& payload);

    static QJsonObject buildRegister(const QString& username, const QString& password,
                                     const QString& nickname);
    static RegisterResponse parseRegisterResponse(const QJsonObject& payload);

    static QJsonObject buildChatSend(int toUserId, const QString& content,
                                     int msgType = 1, const QString& extra = QString());
    static ChatSendResponse parseChatSendResponse(const QJsonObject& payload);

    static QJsonObject buildChatHistory(int withUserId, int beforeMsgId = 0, int limit = 20);
    static ChatHistoryResponse parseChatHistoryResponse(const QJsonObject& payload);

    static ChatRecv parseChatRecv(const QJsonObject& payload);

    // ── Phase 2：好友 ─────────────────────────────────

    static QJsonObject buildFriendRequest(int toUserId, const QString& message = QString());
    static FriendRequestResponse parseFriendRequestResponse(const QJsonObject& payload);

    static FriendNotify parseFriendNotify(const QJsonObject& payload);

    static QJsonObject buildFriendHandle(int requestId, const QString& action);
    static FriendHandleResponse parseFriendHandleResponse(const QJsonObject& payload);

    static QJsonObject buildFriendList();
    static FriendListResponse parseFriendListResponse(const QJsonObject& payload);

    static QJsonObject buildFriendPending();
    static FriendPendingResponse parseFriendPendingResponse(const QJsonObject& payload);

    // ── Phase 3：群聊 ─────────────────────────────────

    static QJsonObject buildGroupCreate(const QString& name);
    static GroupCreateResponse parseGroupCreateResponse(const QJsonObject& payload);

    static QJsonObject buildGroupJoin(int groupId);
    static GroupJoinResponse parseGroupJoinResponse(const QJsonObject& payload);

    static QJsonObject buildGroupSend(int groupId, const QString& content,
                                      int msgType = 1, const QString& extra = QString());
    static GroupSendResponse parseGroupSendResponse(const QJsonObject& payload);

    static GroupRecv parseGroupRecv(const QJsonObject& payload);

    static QJsonObject buildGroupHistory(int groupId, int beforeMsgSeq = 0, int limit = 20);
    static GroupHistoryResponse parseGroupHistoryResponse(const QJsonObject& payload);

    static QJsonObject buildGroupApply(int groupId, const QString& message = QString());
    static GroupApplyResponse parseGroupApplyResponse(const QJsonObject& payload);

    static GroupApplyNotify parseGroupApplyNotify(const QJsonObject& payload);

    static QJsonObject buildGroupApplyHandle(int requestId, const QString& action);
    static GroupApplyHandleResponse parseGroupApplyHandleResponse(const QJsonObject& payload);

    static QJsonObject buildGroupList();
    static GroupListResponse parseGroupListResponse(const QJsonObject& payload);

    // ── 通用 ──────────────────────────────────────────

    static ErrorNotification parseError(const QJsonObject& payload);

private:
    static QString messageType(const QJsonObject& payload);
    static QJsonObject bodyFrom(const QJsonObject& payload);
};
