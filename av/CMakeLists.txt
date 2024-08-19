cmake_minimum_required(VERSION 3.0)

project(app)

set(CMAKE_C_STANDARD 99)

# 添加 pthread 选项
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc 
                   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tsbuilder
                   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tsbuilder/include)

add_subdirectory(third_party)

file(GLOB_RECURSE SRC_LIST "src/*.c")

add_executable(${PROJECT_NAME} main.c ${SRC_LIST})

# link .a || .so
target_link_libraries(${PROJECT_NAME} tsbuilder_static)
# target_link_libraries(my_app cjson_shared)

target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

include(CTest)

add_test(NAME valgrind_test
         COMMAND valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 $<TARGET_FILE:${PROJECT_NAME}>)
