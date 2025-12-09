cmake_minimum_required(VERSION 3.26)
project(Cargo_and_cranes)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(Cargo_and_cranes
    src/main.cpp
    src/Settings.cpp
    src/GUI.cpp
    src/Schedule.cpp
    src/Port.cpp
    src/Crane.cpp
    src/EventLog.cpp
    src/Event.cpp
    src/Deviations.cpp
    src/Statistics.cpp
)

# Поиск nlohmann_json ДО использования
find_package(nlohmann_json 3.11.2 REQUIRED)
find_package(Qt6 COMPONENTS Widgets REQUIRED)

# Универсальная настройка SFML для разных платформ
if(WIN32)
    # Настройки для Windows
    set(SFML_ROOT "C:/SFML-3.0.2")
    set(SFML_DIR "${SFML_ROOT}/lib/cmake/SFML")
    set(SFML_COMPONENTS Graphics System Window)
    find_package(SFML COMPONENTS ${SFML_COMPONENTS} REQUIRED)
    
    # Копирование DLL только для Windows
    add_custom_command(TARGET Cargo_and_cranes POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_ROOT}/bin/sfml-graphics-3.dll"
        $<TARGET_FILE_DIR:Cargo_and_cranes>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_ROOT}/bin/sfml-window-3.dll"
        $<TARGET_FILE_DIR:Cargo_and_cranes>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SFML_ROOT}/bin/sfml-system-3.dll"
        $<TARGET_FILE_DIR:Cargo_and_cranes>
    )
    
elseif(APPLE)
    # Настройки для macOS
    if(NOT DEFINED SFML_DIR)
        if(EXISTS "/opt/homebrew/lib/cmake/SFML")
            set(SFML_DIR "/opt/homebrew/lib/cmake/SFML")
        elseif(EXISTS "/usr/local/lib/cmake/SFML")
            set(SFML_DIR "/usr/local/lib/cmake/SFML")
        endif()
    endif()
    set(SFML_COMPONENTS Graphics System Window)
    find_package(SFML COMPONENTS ${SFML_COMPONENTS} REQUIRED)
    
else()
    # Настройки для Linux и других систем
    set(SFML_COMPONENTS Graphics System Window)
    find_package(SFML COMPONENTS ${SFML_COMPONENTS} REQUIRED)
endif()

# Связывание библиотек
target_link_libraries(Cargo_and_cranes
    nlohmann_json::nlohmann_json
    SFML::Graphics
    SFML::Window
    SFML::System
)

add_executable(Cargo_menu
    src/Menu.cpp
)

target_link_libraries(Cargo_menu
    Qt6::Widgets
)

add_executable(CargoEditor
    src/editor/main.cpp
    src/editor/EditorWindow.cpp
    src/editor/SettingsTab.cpp
    src/editor/ScheduleTab.cpp
    src/editor/EditorFileUtils.cpp
    include/editor/EditorWindow.h
    include/editor/SettingsTab.h
    include/editor/ScheduleTab.h
    include/editor/EditorFileUtils.h
)

target_include_directories(CargoEditor PRIVATE include)

target_link_libraries(CargoEditor Qt6::Widgets)
