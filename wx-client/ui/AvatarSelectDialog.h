#pragma once

#include <QDialog>
#include <QColor>
#include <QString>

// 头像选择对话框：从内置颜色集中选择一种
class AvatarSelectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AvatarSelectDialog(const QString& nickname, QWidget* parent = nullptr);

    QColor selectedColor() const { return m_selectedColor; }

private:
    QColor m_selectedColor;
    QString m_nickname;
};
