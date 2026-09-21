cmake_minimum_required(VERSION 3.30)
add_compile_options(-std=c++20)

project(halfred LANGUAGES CXX)
find_package(Boost REQUIRED COMPONENTS program_options unit_test_framework)

add_executable(halfred src/app.cpp src/halfred.cpp)
target_include_directories(halfred PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(halfred PRIVATE Boost::program_options)

add_executable(test_board test/test_board.cpp src/halfred.cpp)
target_include_directories(test_board PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(test_board PRIVATE Boost::unit_test_framework)

add_executable(test_game test/test_game.cpp src/halfred.cpp)
target_include_directories(test_game PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(test_game PRIVATE Boost::unit_test_framework)

add_executable(test_other test/test_other.cpp src/halfred.cpp)
target_include_directories(test_other PRIVATE "${PROJECT_SOURCE_DIR}/include")
target_link_libraries(test_other PRIVATE Boost::unit_test_framework)
