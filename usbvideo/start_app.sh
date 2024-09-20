#!/bin/bash

# 获取当前脚本的目录
# SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

APP_NAME="VideoCapture"

# 检测是否存在目录 build
BUILD_DIR="$SCRIPT_DIR/build"
if [ ! -d "$BUILD_DIR" ]; then
    mkdir "$BUILD_DIR"
fi

# 进入 build 目录
cd "$BUILD_DIR" || exit

# 使用 cmake 和 make 编译项目
cmake .. >/dev/null 2>&1
make --silent

echo ""

# 检查是否编译成功
if [ $? -eq 0 ]; then
    # 进入 ../bin/ 目录并执行 app

    BIN_DIR="$SCRIPT_DIR/bin"
    # cd "$BIN_DIR" || exit
    # ./${APP_NAME}

else
    echo "Build failed. Exiting."
    exit 1
fi

