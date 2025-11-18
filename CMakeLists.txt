cmake_minimum_required(VERSION 3.26)
project(Cargo_and_crans)

set(CMAKE_CXX_STANDARD 20)

add_executable(Cargo_and_crans
        src/main.cpp
        src/Settings.cpp
        src/GUI.cpp
        src/Schedule.cpp
        src/Port.cpp
        src/Crane.cpp
        src/EventLog.cpp
        src/Event.cpp
        )

if(APPLE AND NOT DEFINED SFML_DIR)
    if(EXISTS "/opt/homebrew/lib/cmake/SFML")
        set(SFML_DIR "/opt/homebrew/lib/cmake/SFML")
    elseif(EXISTS "/usr/local/lib/cmake/SFML")
        set(SFML_DIR "/usr/local/lib/cmake/SFML")
    endif()
endif()

find_package(nlohmann_json 3.11.2 REQUIRED)
set(SFML_COMPONENTS Graphics)
find_package(SFML COMPONENTS ${SFML_COMPONENTS} REQUIRED)

target_link_libraries(Cargo_and_crans PRIVATE
        nlohmann_json::nlohmann_json
        SFML::Graphics)
