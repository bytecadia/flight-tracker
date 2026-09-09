cmake_minimum_required(VERSION 3.16)

project(flight_cast LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)

pkg_check_modules(INIREADER REQUIRED INIReader)

add_executable(
    flight_cast 
    src/main.cpp
    src/socket.cpp
)

target_include_directories(flight_cast PRIVATE
    include
    ${INIREADER_INCLUDE_DIRS}
)

target_link_directories(flight_cast PRIVATE
    ${INIREADER_LIBRARY_DIRS}
)

target_link_libraries(flight_cast PRIVATE
    ${INIREADER_LIBRARIES}
)
