#!/bin/bash

# 获取当前脚本的目录
SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

BUILD_DIR="$SCRIPT_DIR/build"
BIN_DIR="$SCRIPT_DIR/bin"

# 删除 build 目录
if [ -d "$BUILD_DIR" ]; then
    echo "Deleting build directory..."
    rm -rf "$BUILD_DIR"
    if [ $? -eq 0 ]; then
        echo "Build directory deleted successfully."
    else
        echo "Failed to delete build directory."
        exit 1
    fi
else
    echo "Build directory does not exist."
fi

echo ""

# 删除 bin 目录
if [ -d "$BIN_DIR" ]; then
    echo "Deleting bin directory..."
    rm -rf "$BIN_DIR"
    if [ $? -eq 0 ]; then
        echo "Bin directory deleted successfully."
    else
        echo "Failed to delete bin directory."
        exit 1
    fi
else
    echo "Bin directory does not exist."
fi

