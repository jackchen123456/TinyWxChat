#include "GroupCreateDialog.h"
#include "../protocol/MessageBuilder.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QDebug>

GroupCreateDialog::GroupCreateDialog(WeChatSocket* socket, QWidget* parent)
    : QDialog(parent)
    , m_socket(socket)
{
    setWindowTitle("创建群聊");
    setMinimumWidth(320);
    auto* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("群名称："));
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("输入群聊名称");
    layout->addWidget(m_nameEdit);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #e74c3c;");
    layout->addWidget(m_statusLabel);

    auto* btnLayout = new QHBoxLayout();
    auto* cancelBtn = new QPushButton("取消");
    auto* createBtn = new QPushButton("创建");
    createBtn->setStyleSheet("background: #07c160; color: white;");
    btnLayout->addWidget(cancelBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(createBtn);
    layout->addLayout(btnLayout);

    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(createBtn, &QPushButton::clicked, this, &GroupCreateDialog::onCreateClicked);

    // 等待响应
    connect(m_socket, &WeChatSocket::frameReceived, this, [this](const Frame& frame) {
        QString type = frame.payload.value("type").toString();
        if (type == "group.create_res") {
            GroupCreateResponse r = MessageBuilder::parseGroupCreateResponse(frame.payload);
            if (r.ok) {
                m_groupId = r.groupId;
                m_name    = m_nameEdit->text().trimmed();
                m_statusLabel->setStyleSheet("color: #27ae60;");
                m_statusLabel->setText(QString("群聊创建成功！ID: %1").arg(r.groupId));
                QTimer::singleShot(600, this, &QDialog::accept);
            } else {
                m_statusLabel->setText(QString("创建失败：%1").arg(r.errorMsg));
            }
        }
    });
}

void GroupCreateDialog::onCreateClicked()
{
    QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        m_statusLabel->setText("请输入群名称");
        return;
    }
    QJsonObject msg = MessageBuilder::buildGroupCreate(name);
    msg["seq"] = static_cast<int>(m_socket->nextSeq());
    m_socket->sendFrame(FrameType::REQUEST, msg);
    m_statusLabel->setText("正在创建...");
}
