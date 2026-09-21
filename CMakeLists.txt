cmake_minimum_required(VERSION 3.16)

project(flight_cast LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.17.0
)

FetchContent_Declare(
    sqlitecpp
    GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git
    GIT_TAG 3.3.3
)

FetchContent_Declare(
    inih
    GIT_REPOSITORY https://github.com/benhoyt/inih.git
    GIT_TAG r62
)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)

FetchContent_MakeAvailable(
    spdlog
    sqlitecpp
    inih
    stb
)

add_executable(
    flight_cast 
    src/main.cpp
    src/socket.cpp
    src/aircraft.cpp
    src/process.cpp
    src/display.cpp
)

add_library(INIReader STATIC
    ${inih_SOURCE_DIR}/ini.c
    ${inih_SOURCE_DIR}/cpp/INIReader.cpp
)

target_include_directories(INIReader PUBLIC
    ${inih_SOURCE_DIR}
    ${inih_SOURCE_DIR}/cpp
)

target_include_directories( flight_cast PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${inih_SOURCE_DIR}/cpp
        ${stb_SOURCE_DIR}
)

target_link_libraries(flight_cast PRIVATE
    spdlog::spdlog
    SQLiteCpp
    INIReader
)

set(LEDMATRIX_DIR ${CMAKE_CURRENT_SOURCE_DIR}/rpi-rgb-led-matrix)

if (APPLE)
    add_library(ledmatrix_graphics STATIC
        ${LEDMATRIX_DIR}/lib/bdf-font.cc
        ${LEDMATRIX_DIR}/lib/graphics.cc
    )

    target_include_directories(ledmatrix_graphics PUBLIC
        ${LEDMATRIX_DIR}/include
    )

    target_link_libraries(flight_cast PRIVATE
        ledmatrix_graphics
    )
else()
    add_subdirectory(${LEDMATRIX_DIR} ledmatrix-build)

    target_link_libraries(flight_cast PRIVATE
        rgbmatrix
    )
endif()
