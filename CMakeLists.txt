cmake_minimum_required(VERSION 3.29)
project(Cargo_and_crans)

set(CMAKE_CXX_STANDARD 20)

find_package(nlohmann_json CONFIG REQUIRED)

add_executable(Cargo_and_crans
        main.cpp
        src/Settings.cpp)

target_link_libraries(Cargo_and_crans PRIVATE nlohmann_json::nlohmann_json)
