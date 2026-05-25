# 开发环境搭建（Linux / macOS）

> 目标：先把“能编译 + 能调试 + 能跑起来”的工具链搭好，后面 Hermes 才能直接自动化落地。

## 1. 必备工具

- 编译器：GCC 11+ 或 Clang 14+（macOS 用 Apple Clang）
- 构建：CMake 3.20+
- 构建器：Ninja（推荐）
- Qt：Qt 6（含 Widgets/Network 模块）
- 调试：gdb 或 lldb
- 依赖（服务端）：SQLite、nlohmann_json
  - Linux 额外需要 `libcrypt`（密码哈希）
  - macOS 额外需要 OpenSSL（密码哈希，详见 §3.3）
  - 当前服务端使用 POSIX 原生 socket，**无需 asio**。早期文档/CMake 引用过 asio，已移除。

## 2. Ubuntu / Debian（推荐命令）

> 你当前使用 Ubuntu：推荐按本节安装。其他发行版请看文末”其他发行版”。

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
  libcrypt-dev
```

可选（更舒服）：
```bash
sudo apt install -y qtcreator
```

## 3. macOS（Homebrew）

> 适用于 macOS（Apple Silicon / Intel），使用 Homebrew 安装依赖。

### 3.1 前提

- Xcode Command Line Tools：`xcode-select --install`
- Homebrew：`/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"`

### 3.2 安装依赖

```bash
brew update
brew install \
  cmake \
  ninja \
  pkg-config \
  qt \
  sqlite \
  nlohmann-json \
  openssl@3
```

> macOS 自带 Apple Clang（Xcode），无需额外安装编译器。
> 调试器使用 lldb（随 Xcode 自带），而非 gdb。
> sqlite 安装后为 keg-only，需设置 PKG_CONFIG_PATH：
> ```bash
> export PKG_CONFIG_PATH="/opt/homebrew/opt/sqlite/lib/pkgconfig"
> ```
> 建议将上述 export 写入 `~/.zshrc`。

### 3.3 平台兼容性：crypt.h 问题

服务端 `database.cpp` 使用了 Linux 特有的 `<crypt.h>` 和 `crypt_r()`。
macOS 没有这个头文件，需要使用条件编译：

**wx-server/CMakeLists.txt**：条件链接 OpenSSL（macOS）或 crypt（Linux）：
```cmake
# ── Platform-specific crypto ──────────────────────────────
if(APPLE)
    find_package(OpenSSL REQUIRED)
    target_link_libraries(tinywechat-server PRIVATE OpenSSL::Crypto)
    target_include_directories(tinywechat-server PRIVATE ${OPENSSL_INCLUDE_DIR})
else()
    target_link_libraries(tinywechat-server PRIVATE crypt)
endif()
```

**wx-server/src/database.cpp**：条件编译密码哈希函数：
```cpp
#ifdef __APPLE__
#include <openssl/evp.h>
#else
#include <crypt.h>
#endif

// bcryptHash() 中：
#ifdef __APPLE__
    // macOS: PBKDF2-HMAC-SHA256
    PKCS5_PBKDF2_HMAC(...);
#else
    // Linux: crypt_r with bcrypt
    crypt_r(...);
#endif
```

> ⚠️ 注意：macOS（PBKDF2）和 Linux（bcrypt）生成的密码哈希格式不同，
> 数据库不跨平台兼容。同一份 db 文件不能在两个平台间迁移用户数据。

## 4. 环境自检

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

## 5. 推荐的项目构建方式（后续会用到）

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

## 6. 其他发行版（思路）

- Arch：用 `pacman` 安装 qt6-base、qt6-tools、cmake、ninja、sqlite、nlohmann-json
- Fedora：用 `dnf` 安装 qt6-qtbase-devel、qt6-qttools-devel、cmake、ninja-build、sqlite-devel、nlohmann-json-devel、libxcrypt-devel

> 如果你告诉我具体发行版，我可以把这一节补成可直接复制的命令。
