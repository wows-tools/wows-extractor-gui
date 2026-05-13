cmake_minimum_required(VERSION 3.16)
project(wows-depack-ui VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ── Qt ──────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Quick QuickWidgets Quick3D)

# ── System libraries ────────────────────────────────────────────────
find_package(ZLIB REQUIRED)
find_library(PCRE2_LIBRARY NAMES pcre2-8 REQUIRED)
find_library(MESHOPTIMIZER_LIBRARY NAMES meshoptimizer REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Development)

find_package(TinyGLTF REQUIRED)
find_library(TINYGLTF_LIBRARY NAMES tinygltf REQUIRED)

# ── wows-depack submodule (build as static lib) ─────────────────────
set(WOWS_DEPACK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wows-depack)
file(GLOB WOWS_DEPACK_SOURCES
    ${WOWS_DEPACK_DIR}/lib/*.c
)
add_library(wows-depack-static STATIC ${WOWS_DEPACK_SOURCES})
target_include_directories(wows-depack-static PUBLIC
    ${WOWS_DEPACK_DIR}/inc
)
target_link_libraries(wows-depack-static PRIVATE
    ZLIB::ZLIB
    ${PCRE2_LIBRARY}
)
target_compile_definitions(wows-depack-static PRIVATE _GNU_SOURCE)

# ── wows-geometry (model exporter) submodule ────────────────────────
set(WOWS_ME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wows-model-exporter)
file(GLOB WOWS_ME_SOURCES
    ${WOWS_ME_DIR}/lib/*.c
    ${WOWS_ME_DIR}/lib/*.cpp
)
add_library(wows-geometry-static STATIC ${WOWS_ME_SOURCES})
target_include_directories(wows-geometry-static PUBLIC
    ${WOWS_ME_DIR}/inc
    ${Python3_INCLUDE_DIRS}
)
target_link_libraries(wows-geometry-static PUBLIC
    TinyGLTF::TinyGLTF
    ${MESHOPTIMIZER_LIBRARY}
    ${Python3_LIBRARIES}
    ZLIB::ZLIB
    m
)
target_compile_options(wows-geometry-static PRIVATE -DBFD_VERSION='"0.1.0"')

# ── UI executable ────────────────────────────────────────────────────
set(UI_SOURCES
    src/main.cpp
    src/mainwindow.h
    src/mainwindow.cpp
    src/archivewidget.h
    src/archivewidget.cpp
    src/shipviewerwidget.h
    src/shipviewerwidget.cpp
    src/resources.qrc
)

add_executable(wows-depack-ui ${UI_SOURCES})

target_include_directories(wows-depack-ui PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${WOWS_DEPACK_DIR}/inc
    ${WOWS_ME_DIR}/inc
    ${Python3_INCLUDE_DIRS}
)

target_link_libraries(wows-depack-ui PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickWidgets
    Qt6::Quick3D
    wows-depack-static
    wows-geometry-static
    ${PCRE2_LIBRARY}
    ZLIB::ZLIB
    ${Python3_LIBRARIES}
    dl
    pthread
)
# Both wows-depack and wows-model-exporter define internal helpers with the same
# symbol name (e.g. datatoh32). Allow the linker to pick either one.
target_link_options(wows-depack-ui PRIVATE -Wl,--allow-multiple-definition)

install(TARGETS wows-depack-ui RUNTIME DESTINATION bin)
