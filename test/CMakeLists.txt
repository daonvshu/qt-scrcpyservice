cmake_minimum_required(VERSION 3.12)

project(scrcpy-test)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

add_executable(${PROJECT_NAME} WIN32
    main.cpp

    app.h
    app.cpp
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets

    scrcpy-service
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND (taskkill /f /t /im adb.exe) || echo "adb not found"
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/3rdparty/adb $<TARGET_FILE_DIR:${PROJECT_NAME}>/adb
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/3rdparty/scrcpy $<TARGET_FILE_DIR:${PROJECT_NAME}>/scrcpy
)