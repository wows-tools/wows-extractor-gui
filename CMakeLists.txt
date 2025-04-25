cmake_minimum_required(VERSION 3.16)
project(wows-depack-ui VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Set build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(ZLIB REQUIRED)

# Find required system libraries
find_library(PCRE2_LIBRARY NAMES pcre2-8)

# Add executable
add_executable(wows-depack-ui
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/mainwindow.ui
)

target_include_directories(wows-depack-ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wows-depack-ui PRIVATE
    Qt6::Core
    Qt6::Widgets
    ${CMAKE_CURRENT_SOURCE_DIR}/libwows-depack.a
    ${PCRE2_LIBRARY}
    ZLIB::ZLIB
    dl
    pthread
)

# Install rules
install(TARGETS wows-depack-ui
    RUNTIME DESTINATION bin
) 