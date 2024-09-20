cmake_minimum_required(VERSION 3.10)

# 项目信息
project(VideoCaptureExample)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置调试模式
set(CMAKE_BUILD_TYPE Debug)

# 查找 FFmpeg 库的头文件和库路径
find_package(PkgConfig REQUIRED)

# 查找必要的 FFmpeg 库
pkg_check_modules(AVFORMAT libavformat REQUIRED)
pkg_check_modules(AVDEVICE libavdevice REQUIRED)
pkg_check_modules(AVCODEC libavcodec REQUIRED)
pkg_check_modules(AVUTIL libavutil REQUIRED)
pkg_check_modules(SWSCALE libswscale REQUIRED)

# 将头文件路径添加到 include 目录
include_directories(${AVFORMAT_INCLUDE_DIRS})
include_directories(${AVDEVICE_INCLUDE_DIRS})
include_directories(${AVCODEC_INCLUDE_DIRS})
include_directories(${AVUTIL_INCLUDE_DIRS})
include_directories(${SWSCALE_INCLUDE_DIRS})

# 生成可执行文件
add_executable(VideoCapture main.c)

# 链接 FFmpeg 库
target_link_libraries(VideoCapture
    ${AVFORMAT_LIBRARIES}
    ${AVDEVICE_LIBRARIES}
    ${AVCODEC_LIBRARIES}
    ${AVUTIL_LIBRARIES}
    ${SWSCALE_LIBRARIES}
)

# 设置编译标志（如有必要）
target_compile_options(VideoCapture PRIVATE -Wall -Wextra)

# 设置输出路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
