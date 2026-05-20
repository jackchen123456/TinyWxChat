#include "AvatarSelectDialog.h"
#include "AvatarLabel.h"
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

AvatarSelectDialog::AvatarSelectDialog(const QString& nickname, QWidget* parent)
    : QDialog(parent)
    , m_selectedColor("#3498db")
    , m_nickname(nickname)
{
    setWindowTitle("选择头像");
    setFixedSize(300, 360);
    auto* layout = new QVBoxLayout(this);

    QLabel* title = new QLabel("选择你喜欢的头像颜色：");
    title->setStyleSheet("font-size: 14px; margin-bottom: 8px;");
    layout->addWidget(title);

    auto* grid = new QGridLayout();
    grid->setSpacing(10);

    const QStringList& colors = AvatarLabel::defaultColors();
    for (int i = 0; i < colors.size(); ++i) {
        QColor color(colors[i]);
        auto* avatar = new AvatarLabel();
        avatar->setAvatarColor(color);
        avatar->setAvatarText(nickname.isEmpty() ? "你" : nickname.left(1).toUpper());
        avatar->setFixedSize(48, 48);
        avatar->setCursor(Qt::PointingHandCursor);
        avatar->setStyleSheet("border: 2px solid transparent; border-radius: 24px;");

        connect(avatar, &AvatarLabel::clicked, this, [this, color, avatar]() {
            m_selectedColor = color;
            accept();
        });
        grid->addWidget(avatar, i / 5, i % 5);
    }
    layout->addLayout(grid);

    QLabel* hint = new QLabel("点击头像选择，再点确定");
    hint->setStyleSheet("color: #888; font-size: 12px; margin-top: 8px;");
    layout->addWidget(hint);
}
