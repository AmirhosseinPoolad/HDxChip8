cmake_minimum_required(VERSION 3.7)

project(HDxChip8)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(HDxChip8 src/main.c)
target_link_libraries(HDxChip8 ${SDL2_LIBRARIES})

target_compile_options(HDxChip8 PRIVATE -Werror -Wall -Wextra)