#pragma once

#include <QLabel>
#include <QColor>
#include <QString>
#include <QMouseEvent>

// 圆形头像标签：显示首字母 + 背景色，支持点击信号
class AvatarLabel : public QLabel
{
    Q_OBJECT
public:
    static const QStringList& defaultColors();

    explicit AvatarLabel(QWidget* parent = nullptr);

    void setNickname(const QString& nickname);
    void setAvatarColor(const QColor& color);
    void setAvatarText(const QString& text);

    static QSize defaultSize() { return QSize(36, 36); }

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) emit clicked();
        QLabel::mousePressEvent(event);
    }

private:
    void updatePixmap();
    QColor m_color = QColor("#3498db");
    QString m_text = "你";
};
