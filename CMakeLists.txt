cmake_minimum_required(VERSION 3.16)
project(wows-extractor-gui VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# ── MSVC global flags ────────────────────────────────────────────────
if(MSVC)
    # suppress POSIX-name deprecation warnings (open, close, read, …)
    add_compile_definitions(_CRT_NONSTDC_NO_WARNINGS _CRT_SECURE_NO_WARNINGS)
    # suppress "unknown pragma" for #pragma GCC diagnostic in vendored code
    add_compile_options(/wd4068)
endif()

# ── Qt ──────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Quick QuickWidgets Quick3D)

# ── System libraries ────────────────────────────────────────────────
find_package(ZLIB REQUIRED)
find_library(PCRE2_LIBRARY NAMES pcre2-8 REQUIRED)

find_library(MESHOPTIMIZER_LIBRARY NAMES meshoptimizer)
if(NOT MESHOPTIMIZER_LIBRARY)
    include(FetchContent)
    FetchContent_Declare(meshoptimizer_fc
        GIT_REPOSITORY https://github.com/zeux/meshoptimizer.git
        GIT_TAG        v0.22
        GIT_SHALLOW    TRUE
    )
    set(MESHOPTIMIZER_BUILD_DEMO  OFF CACHE BOOL "" FORCE)
    set(MESHOPTIMIZER_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(meshoptimizer_fc)
    set(MESHOPTIMIZER_LIBRARY meshoptimizer)
endif()

find_package(Python3 REQUIRED COMPONENTS Development)

# TinyGLTF: try system package first, then vcpkg CONFIG package
find_package(TinyGLTF QUIET)
if(NOT TinyGLTF_FOUND)
    find_package(TinyGLTF CONFIG QUIET)
endif()
if(NOT TinyGLTF_FOUND)
    message(FATAL_ERROR "TinyGLTF not found (install libtinygltf-dev or vcpkg tinygltf)")
endif()
# tinygltf is header-only on some platforms; compiled lib is optional
find_library(TINYGLTF_LIBRARY NAMES tinygltf)

# ── wows-depack submodule (build as static lib) ─────────────────────
set(WOWS_DEPACK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wows-depack)
file(GLOB WOWS_DEPACK_SOURCES ${WOWS_DEPACK_DIR}/lib/*.c)

add_library(wows-depack-static STATIC ${WOWS_DEPACK_SOURCES})
target_include_directories(wows-depack-static PUBLIC ${WOWS_DEPACK_DIR}/inc)
target_link_libraries(wows-depack-static PRIVATE ZLIB::ZLIB ${PCRE2_LIBRARY})

if(NOT MSVC)
    target_compile_definitions(wows-depack-static PRIVATE _GNU_SOURCE)
endif()

# ── wows-geometry (model exporter) submodule ────────────────────────
set(WOWS_ME_DIR ${CMAKE_CURRENT_SOURCE_DIR}/wows-model-exporter)
file(GLOB WOWS_ME_SOURCES ${WOWS_ME_DIR}/lib/*.c ${WOWS_ME_DIR}/lib/*.cpp)

add_library(wows-geometry-static STATIC ${WOWS_ME_SOURCES})
target_include_directories(wows-geometry-static PUBLIC
    ${WOWS_ME_DIR}/inc
    ${WOWS_ME_DIR}/deps
    ${WOWS_DEPACK_DIR}/inc
    ${Python3_INCLUDE_DIRS}
)
target_link_libraries(wows-geometry-static PUBLIC
    TinyGLTF::TinyGLTF
    ${MESHOPTIMIZER_LIBRARY}
    ${Python3_LIBRARIES}
    ZLIB::ZLIB
    wows-depack-static
    ${PCRE2_LIBRARY}
)
if(TINYGLTF_LIBRARY)
    target_link_libraries(wows-geometry-static PUBLIC ${TINYGLTF_LIBRARY})
endif()
if(NOT MSVC)
    target_link_libraries(wows-geometry-static PUBLIC m)
endif()

target_compile_options(wows-geometry-static PRIVATE -DBFD_VERSION='"0.1.0"')

# ── Windows / MSVC POSIX compatibility ──────────────────────────────
if(MSVC)
    # Shadow headers provide unistd.h, libgen.h, endian.h, linux/limits.h
    # They must appear before system include paths so the compiler finds
    # our shims when vendored C code does #include <unistd.h> etc.
    set(WIN32_COMPAT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/compat/win32)
    foreach(tgt wows-depack-static wows-geometry-static)
        target_include_directories(${tgt} BEFORE PUBLIC
            ${WIN32_COMPAT_DIR}
            ${WIN32_COMPAT_DIR}/linux  # for <linux/limits.h>
        )
    endforeach()

    # mman-win32 (vcpkg) provides sys/mman.h + mman.lib
    find_library(MMAN_LIBRARY NAMES mman)
    if(NOT MMAN_LIBRARY)
        message(FATAL_ERROR
            "mman-win32 not found. "
            "Add 'mman' to vcpkg.json or run: vcpkg install mman")
    endif()
    target_link_libraries(wows-depack-static  PUBLIC ${MMAN_LIBRARY})
    target_link_libraries(wows-geometry-static PUBLIC ${MMAN_LIBRARY})
    # dirent (vcpkg) is header-only; no library to link, just needs the include path
endif()

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

add_executable(wows-extractor ${UI_SOURCES})

target_include_directories(wows-extractor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${WOWS_DEPACK_DIR}/inc
    ${WOWS_ME_DIR}/inc
    ${Python3_INCLUDE_DIRS}
)

target_link_libraries(wows-extractor PRIVATE
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
)
# dl / pthread: Linux/macOS only
if(UNIX AND NOT WIN32)
    target_link_libraries(wows-extractor PRIVATE dl)
endif()
if(UNIX)
    target_link_libraries(wows-extractor PRIVATE pthread)
endif()

install(TARGETS wows-extractor RUNTIME DESTINATION bin)
