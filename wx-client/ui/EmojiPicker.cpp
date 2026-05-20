#include "EmojiPicker.h"
#include <QGridLayout>
#include <QDebug>

EmojiPicker::EmojiPicker(QWidget* parent)
    : QWidget(parent, Qt::Popup)
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("表情");
    setMinimumSize(300, 200);

    auto* grid = new QGridLayout(this);
    grid->setSpacing(2);
    grid->setContentsMargins(4, 4, 4, 4);

    QStringList emojis = emojiList();
    int cols = 8;
    for (int i = 0; i < emojis.size(); ++i) {
        auto* btn = new QPushButton(emojis[i]);
        btn->setFixedSize(34, 34);
        btn->setStyleSheet(
            "QPushButton { font-size: 20px; border: none; border-radius: 4px; background: transparent; }"
            "QPushButton:hover { background: #e0e0e0; }"
        );
        connect(btn, &QPushButton::clicked, this, [this, emoji = emojis[i]]() {
            emit emojiSelected(emoji);
            close();
        });
        grid->addWidget(btn, i / cols, i % cols);
    }
    setLayout(grid);
}

QStringList EmojiPicker::emojiList()
{
    return {
        // 笑脸
        "😀","😃","😄","😁","😅","😂","🤣","😊",
        "😇","🙂","😉","😌","😍","🥰","😘","😗",
        "😋","😛","😜","🤪","😝","🤑","🤗","🤭",
        // 手势
        "👍","👎","👏","🙌","🤝","💪","✌️","🤞",
        "👋","🤚","✋","👌","🤏","👆","👇","👉",
        // 爱心/符号
        "❤️","🧡","💛","💚","💙","💜","🖤","🤍",
        "💔","❣️","💕","💖","🔥","⭐","✨","💯",
        // 动物/物品
        "🐶","🐱","🐭","🐹","🐰","🦊","🐻","🐼",
        "☀️","🌙","⭐","🌈","☁️","❄️","🌸","🌺",
        "🎉","🎊","🎈","🎁","🏆","🥇","🎵","🎶",
        "☕","🍵","🍺","🍻","🍰","🎂","🍕","🍔",
        // 常用
        "✅","❌","⚠️","❓","❗","💤","💢","💬",
        "➕","➖","➗","✖️","©️","®️","™️","🆗",
    };
}
