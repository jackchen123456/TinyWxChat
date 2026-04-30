#!/bin/bash
# TinyWeChat Phase 0 环境安装脚本
# 在 Ubuntu 24.04 上执行，安装 C++ 构建工具链 + Qt6 + SQLite

set -e

echo "=== TinyWeChat 环境安装 ==="

# 基础构建工具
sudo apt update
sudo apt install -y build-essential cmake pkg-config

# Qt6 Widgets 开发包（AGENTS.md 要求 Qt Widgets）
sudo apt install -y qt6-base-dev libqt6sql6-sqlite

# SQLite3 开发库
sudo apt install -y libsqlite3-dev

# bcrypt（C++ 密码哈希）
sudo apt install -y libbcrypt-dev

echo ""
echo "=== 安装完成 ==="
g++ --version | head -1
cmake --version | head -1
echo "Qt6: $(dpkg -s qt6-base-dev 2>/dev/null | grep Version | awk '{print $2}')"
echo "SQLite3: $(dpkg -s libsqlite3-dev 2>/dev/null | grep Version | awk '{print $2}')"
