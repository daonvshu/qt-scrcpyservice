cmake_minimum_required(VERSION 3.12)

project(scrcpy-service)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

find_package(Qt5 COMPONENTS Core Gui Widgets Network Multimedia REQUIRED)

if (NOT DEFINED FFMPEG_DIR)
    message(FATAL_ERROR "FFMPEG_DIR not defined!")
else ()
    message("ffmpeg dir: ${FFMPEG_DIR}")
endif ()
include_directories(${FFMPEG_DIR}/include)
link_directories(${FFMPEG_DIR}/lib)

if (NOT DEFINED SCRCPY_VERSION)
    message(FATAL_ERROR "SCRCPY_VERSION not defined!")
else ()
    add_compile_definitions(SCRCPY_VERSION="${SCRCPY_VERSION}")
endif ()

add_library(${PROJECT_NAME}
    adbcommandrunner.h
    adbcommandrunner.cpp
    videodecoder.h
    videodecoder.cpp
    scrcpyserver.h
    scrcpyserver.cpp
    bufferreceiver.h
    bufferreceiver.cpp
    controlmsg.h
    controlmsg.cpp

    render/videowidget.h
    render/videowidget.cpp
    render/qmediaopenglhelper_p.h
    render/qpaintervideosurface_p.h
    render/qpaintervideosurface.cpp
)

target_link_libraries(${PROJECT_NAME}
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::Network
    Qt5::Multimedia

    avcodec
    avformat
    avutil
    swscale
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/ ${FFMPEG_DIR}/include)
target_link_directories(${PROJECT_NAME} PUBLIC ${FFMPEG_DIR}/lib)