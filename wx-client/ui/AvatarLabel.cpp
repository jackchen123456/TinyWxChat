#include "AvatarLabel.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>

const QStringList& AvatarLabel::defaultColors()
{
    static QStringList colors = {
        "#e74c3c", "#e67e22", "#f1c40f", "#2ecc71", "#1abc9c",
        "#3498db", "#9b59b6", "#34495e", "#e84393", "#00cec9"
    };
    return colors;
}

AvatarLabel::AvatarLabel(QWidget* parent)
    : QLabel(parent)
{
    setFixedSize(defaultSize());
    updatePixmap();
}

void AvatarLabel::setNickname(const QString& nickname)
{
    if (nickname.isEmpty()) {
        m_text = "?";
        m_color = QColor("#bdc3c7");
    } else {
        // 取第一个字符作为头像文字
        m_text = nickname.left(1).toUpper();
        // 颜色由昵称 hash 决定（同一昵称总是同一颜色）
        uint hash = qHash(nickname);
        m_color = QColor(defaultColors().at(hash % defaultColors().size()));
    }
    updatePixmap();
}

void AvatarLabel::setAvatarColor(const QColor& color)
{
    m_color = color;
    updatePixmap();
}

void AvatarLabel::setAvatarText(const QString& text)
{
    m_text = text;
    updatePixmap();
}

void AvatarLabel::updatePixmap()
{
    int size = defaultSize().width();
    QPixmap pix(size, size);
    pix.fill(Qt::transparent);

    QPainter painter(&pix);
    painter.setRenderHint(QPainter::Antialiasing);

    // 圆形背景
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    painter.fillRect(0, 0, size, size, m_color);

    // 白色文字
    painter.setPen(Qt::white);
    QFont font;
    font.setPixelSize(size / 2);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(QRect(0, 0, size, size), Qt::AlignCenter, m_text);

    setPixmap(pix);
}
