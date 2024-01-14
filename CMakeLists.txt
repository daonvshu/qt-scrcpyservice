cmake_minimum_required(VERSION 3.12)

project(qt-scrcpy-service-project)

if (WIN32)
    add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
    add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

add_subdirectory(src)

option(QT_SCRCPY_SERVICE_BUILD_TEST "enable qt-scrcpy-service test projects" OFF)

if (QT_SCRCPY_SERVICE_BUILD_TEST)
    add_subdirectory(test)
endif ()