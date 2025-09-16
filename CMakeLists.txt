cmake_minimum_required(VERSION 3.27)

project(Rttometer LANGUAGES CXX VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${PROJECT_BINARY_DIR}
)

file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE APP_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/app/*.cpp")


add_library(rttometer_lib STATIC
    ${LIB_SOURCES}
)

add_executable(rttometer
    ${APP_SOURCES}
)
target_link_libraries(rttometer PRIVATE rttometer_lib)
