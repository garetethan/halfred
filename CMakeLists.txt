cmake_minimum_required(VERSION 3.10)
project(scrabble_solver)
add_executable(scrabble_solver app/test_solver.cpp)
target_include_directories(scrabble_solver PUBLIC "${PROJECT_SOURCE_DIR}/include")
