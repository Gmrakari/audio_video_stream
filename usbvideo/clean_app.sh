#!/bin/bash

# 获取当前脚本的目录
SCRIPT_PATH=$(readlink -f "$0")
SCRIPT_DIR=$(dirname "$SCRIPT_PATH")

BUILD_DIR="$SCRIPT_DIR/build"
BIN_DIR="$SCRIPT_DIR/bin"
OUTPUT_DIR="$SCRIPT_DIR/output"

# 删除目录函数
delete_directory() {
    local dir=$1
    if [ -d "$dir" ]; then
        local dir_name=$(basename "$dir")
        echo "Deleting $dir_name directory..."
        rm -rf "$dir"
        if [ $? -eq 0 ]; then
            echo "$dir_name directory deleted successfully."
        else
            echo "Failed to delete $dir_name directory."
            exit 1
        fi
    else
        local dir_name=$(basename "$dir")
        echo "$dir_name directory does not exist."
    fi
}

# 删除多个目录函数
delete_directories() {
    delete_directory "$BUILD_DIR"
    echo ""
    delete_directory "$BIN_DIR"
    echo ""
    delete_directory "$OUTPUT_DIR"
}

# 调用删除多个目录函数
delete_directories
