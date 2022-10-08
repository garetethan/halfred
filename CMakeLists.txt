cmake_minimum_required(VERSION 3.10)
add_compile_options(-std=c++20)

project(halfred LANGUAGES CXX)
find_package(Boost REQUIRED COMPONENTS program_options)
add_executable(halfred app/app.cpp)
target_include_directories(halfred PUBLIC "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(halfred PUBLIC Boost::program_options)
