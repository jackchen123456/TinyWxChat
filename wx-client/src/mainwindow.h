#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QTextEdit>
#include <QDateTime>
#include <QTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QStatusBar>

class NetworkClient;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    // Login
    void onLogin();
    void onLoginResult(int code, int userId, const QString& nickname);
    // Register
    void onRegister();
    void goToRegister();
    void goToLogin();
    // Chat
    void onSendChat();
    void onMessageReceived(int fromUid, const QString& fromNick,
                           int msgId, const QString& content, int timestamp);
    void onGroupMessageReceived(int groupId, int fromUid, const QString& fromNick,
                                int msgSeq, const QString& content, int timestamp);
    // Contacts
    void onFriendRequest();
    void onAcceptFriend();
    void onRefreshFriendList();
    void refreshPendingRequests();
    // Groups
    void onCreateGroup();
    void onJoinGroup();
    void onSendGroupChat();
    void onOpenGroupChat();
    void onRefreshGroupList();
    // Navigation
    void openChatWith(int uid, const QString& nick);
    void goToContacts();
    void goToGroups();

private:
    void setupLoginPage();
    void setupRegisterPage();
    void setupContactPage();
    void setupGroupPage();
    void setupChatPage();
    void connectSignals();
    void requestHistory(int withUid);
    void requestGroupHistory(int groupId);

    NetworkClient*   client_;
    QStackedWidget*  stack_;

    // ── Login page ──────────────────────────────────────
    QWidget*   loginPage_;
    QLineEdit* loginUser_;
    QLineEdit* loginPass_;
    QPushButton* loginBtn_;
    QPushButton* toRegisterBtn_;

    // ── Register page ───────────────────────────────────
    QWidget*   registerPage_;
    QLineEdit* regUser_;
    QLineEdit* regPass_;
    QLineEdit* regNick_;
    QPushButton* regBtn_;
    QPushButton* toLoginBtn_;

    // ── Contact page ────────────────────────────────────
    QWidget*    contactPage_;
    QListWidget* friendList_;
    QListWidget* pendingList_;
    QLineEdit*  addFriendUid_;
    QLineEdit*  addFriendMsg_;
    QPushButton* addFriendBtn_;
    QPushButton* toGroupsBtn_;

    // ── Group page ──────────────────────────────────────
    QWidget*    groupPage_;
    QListWidget* groupList_;
    QLineEdit*  groupNameEdit_;
    QPushButton* createGroupBtn_;
    QLineEdit*  joinGroupId_;
    QPushButton* joinGroupBtn_;
    QPushButton* openGroupBtn_;
    QPushButton* toContactsBtn_;

    // ── Chat page ───────────────────────────────────────
    QWidget*    chatPage_;
    QTextEdit*  chatHistory_;
    QLineEdit*  chatInput_;
    QPushButton* chatSendBtn_;
    QPushButton* chatBackBtn_;
    QLabel*     chatTitle_;
    int         chatTargetUid_ = 0;
    int         chatGroupId_   = 0;

    // ── State ───────────────────────────────────────────
    int      myUserId_ = 0;
    QString  myNickname_;
};
