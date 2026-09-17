cmake_minimum_required(VERSION 3.30)
add_compile_options(-std=c++20)

project(halfred LANGUAGES CXX)
find_package(Boost REQUIRED COMPONENTS program_options unit_test_framework)

add_executable(halfred src/app.cpp src/halfred.cpp)
target_include_directories(halfred PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(halfred PRIVATE Boost::program_options)

add_executable(test_halfred test/test_halfred.cpp src/halfred.cpp)
target_include_directories(test_halfred PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(test_halfred PRIVATE Boost::program_options)
target_link_libraries(test_halfred PRIVATE Boost::unit_test_framework)
