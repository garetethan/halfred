cmake_minimum_required(VERSION 3.10)
add_compile_options(-std=c++20)

project(scrabble_sleuth)
add_executable(scrabble_sleuth app/play_scrabble.cpp)
target_include_directories(scrabble_sleuth PUBLIC "${PROJECT_SOURCE_DIR}/include")
