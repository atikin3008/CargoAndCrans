cmake_minimum_required(VERSION 3.26)
project(Cargo_and_cranes)

set(CMAKE_CXX_STANDARD 20)

add_executable(Cargo_and_cranes
        src/main.cpp
        src/Settings.cpp
        # src/GUI.cpp
        src/Schedule.cpp
        src/Port.cpp
        src/Crane.cpp
        src/EventLog.cpp
        src/Event.cpp
        )

find_package(nlohmann_json 3.11.2 REQUIRED)

target_link_libraries(Cargo_and_cranes nlohmann_json::nlohmann_json)
