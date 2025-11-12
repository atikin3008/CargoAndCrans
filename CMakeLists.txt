cmake_minimum_required(VERSION 3.26)
project(Cargo_and_crans)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.2
)
FetchContent_MakeAvailable(json)

add_executable(Cargo_and_crans src/Schedule.cpp src/main.cpp)

target_link_libraries(Cargo_and_crans nlohmann_json::nlohmann_json)