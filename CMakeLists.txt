cmake_minimum_required(VERSION 3.26)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED ENV{CMAKE_BUILD_TYPE})
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type")
endif ()

project(kingdom_key VERSION 0.3.0 LANGUAGES CXX)

# Pin to the known-good Dusklight SDK/game-header commit used by v0.2.1.
# Override with -DDUSKLIGHT_DIR=/path/to/dusklight for a local checkout.
if (NOT DUSKLIGHT_VERSION)
    set(DUSKLIGHT_VERSION "40457c6adb381928e4b5fef6ed459ed291edd5e2")
endif ()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchDusklight.cmake")
add_subdirectory("${DUSKLIGHT_DIR}/sdk" dusklight-sdk EXCLUDE_FROM_ALL)

# Normalize metadata line endings so per-platform bundles byte-compare when
# GitHub Actions merges them into one universal .dusk.
configure_file(mod.json "${CMAKE_CURRENT_BINARY_DIR}/mod.json" @ONLY NEWLINE_STYLE LF)

add_mod(kingdom_key
    FEATURES game
    SOURCES src/mod.cpp
    MOD_JSON "${CMAKE_CURRENT_BINARY_DIR}/mod.json"
    RES_DIR res
    BUNDLE
)

if (MSVC)
    target_compile_options(kingdom_key PRIVATE /Zc:__cplusplus)
endif ()
