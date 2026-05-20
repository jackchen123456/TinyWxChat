#pragma once

#include <QWidget>
#include <QPushButton>
#include <QStringList>

// 内置 Unicode 表情选择器（弹出面板）
class EmojiPicker : public QWidget
{
    Q_OBJECT
public:
    explicit EmojiPicker(QWidget* parent = nullptr);

signals:
    void emojiSelected(const QString& emoji);

private:
    static QStringList emojiList();
};
