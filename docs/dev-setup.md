# 开发环境搭建（Linux）

> 目标：先把“能编译 + 能调试 + 能跑起来”的工具链搭好，后面 Hermes 才能直接自动化落地。

## 1. 必备工具

- 编译器：GCC 11+ 或 Clang 14+
- 构建：CMake 3.20+
- 构建器：Ninja（推荐）
- Qt：Qt 6（含 Widgets/Network 模块）
- 调试：gdb 或 lldb
- 依赖（服务端）：SQLite、JSON、网络库（建议 Asio）

## 2. Ubuntu / Debian（推荐命令）

> 你当前使用 Ubuntu：推荐按本节安装。其他发行版请看文末“其他发行版”。

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  cmake ninja-build pkg-config \
  gdb \
  qt6-base-dev qt6-base-dev-tools \
  qt6-tools-dev qt6-tools-dev-tools \
  libsqlite3-dev \
  nlohmann-json3-dev \
  libasio-dev
```

可选（更舒服）：
```bash
sudo apt install -y qtcreator
```

## 3. 环境自检

```bash
cmake --version
ninja --version
g++ --version

# Qt 版本（任意一个能输出即可）
qtpaths6 --qt-version 2>/dev/null || qmake6 -v
```

Qt 是否可被 CMake 找到（后续工程里会用）：
- 关键是 `Qt6Config.cmake` 能被找到；如果找不到，通常是 Qt6 dev 包没装全。

SQLite/JSON/Asio 是否安装：
```bash
pkg-config --modversion sqlite3 || true
```

Qt 是否可被 pkg-config 识别（可选）：
```bash
pkg-config --modversion Qt6Core 2>/dev/null || true
```

## 4. 推荐的项目构建方式（后续会用到）

- 统一使用 out-of-source build：
```bash
cmake -S . -B build -G Ninja
cmake --build build
```

### 常见问题：CMake 源目录不匹配（Cache 指向了别的源码目录）

如果你看到类似报错：

> CMake Error: The source ".../TinyWeChat/CMakeLists.txt" does not match the source ".../TinyWeChat/wx-client/CMakeLists.txt" used to generate cache.

意思是：你正在复用同一个 `build/` 目录，但这个目录里的 `CMakeCache.txt` 之前是用**另一个源码目录**（比如 `wx-client/`）配置出来的。CMake 不允许在同一个 build 目录里切换源码目录。

最简单修复（推荐）：清理 build 目录后重新配置：
```bash
rm -rf build
cmake -S . -B build -G Ninja
cmake --build build
```

或者只清理缓存文件：
```bash
rm -f build/CMakeCache.txt
rm -rf build/CMakeFiles
cmake -S . -B build -G Ninja
```

### 可选：分别构建客户端/服务端（避免互相影响）

如果你更想把两端完全分开构建（新手也更不容易混）：
```bash
cmake -S wx-client -B build/wx-client -G Ninja
cmake --build build/wx-client

cmake -S wx-server -B build/wx-server -G Ninja
cmake --build build/wx-server
```

## 5. 其他发行版（思路）

- Arch：用 `pacman` 安装 qt6-base、qt6-tools、cmake、ninja、sqlite、asio、nlohmann-json
- Fedora：用 `dnf` 安装 qt6-qtbase-devel、qt6-qttools-devel、cmake、ninja-build、sqlite-devel、asio-devel、nlohmann-json-devel

> 如果你告诉我具体发行版，我可以把这一节补成可直接复制的命令。
