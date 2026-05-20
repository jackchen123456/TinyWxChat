cmake_minimum_required(VERSION 3.20)
project(tinywechat-client VERSION 0.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

add_executable(tinywechat-client
    main.cpp
    network/FrameCodec.cpp
    network/WeChatSocket.cpp
    protocol/MessageBuilder.cpp
    ui/LoginWidget.cpp
    ui/RegisterWidget.cpp
    ui/ChatWidget.cpp
    ui/ConversationListWidget.cpp
    ui/FriendListWidget.cpp
    ui/GroupListWidget.cpp
    ui/GroupCreateDialog.cpp
    ui/GroupChatWidget.cpp
    ui/EmojiPicker.cpp
    ui/AvatarLabel.cpp
    ui/AvatarSelectDialog.cpp
    ui/MainHubWidget.cpp
    ui/MainWindow.cpp
)

target_link_libraries(tinywechat-client PRIVATE
    Qt6::Widgets
    Qt6::Network
)
