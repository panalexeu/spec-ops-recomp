# specops - ReXGlue Recompiled Project
#
# SDK boilerplate lives in generated/rexglue.cmake.

cmake_minimum_required(VERSION 3.25)
project(specops LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use the pinned, patched SDK submodule unless REXSDK_DIR is given (see tools/apply_patches.sh).
# Pass -DREXSDK_DIR= (empty) plus CMAKE_PREFIX_PATH to use an installed SDK package instead.
set(REXSDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/rexglue-sdk" CACHE PATH "Path to rexglue-sdk source tree")

include(generated/rexglue.cmake)

# Sources
set(SPECOPS_SOURCES
    src/main.cpp
)

if(WIN32)
    add_executable(specops WIN32 ${SPECOPS_SOURCES})
else()
    add_executable(specops ${SPECOPS_SOURCES})
endif()

rexglue_setup_target(specops)

# Copy the runtime configuration next to the executable (the runtime reads <exe dir>/specops.toml).
add_custom_command(TARGET specops POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/config/specops.toml
            $<TARGET_FILE_DIR:specops>/specops.toml
    COMMENT "Copying config/specops.toml next to the executable")

# The exe's RUNPATH is $ORIGIN: copy the SDK's shared libraries (runtime, GPU plugin) next to it.
# Needed for both the in-tree SDK (builds into thirdparty/rexglue-sdk/out/) and an installed package.
add_custom_command(TARGET specops POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:rex::runtime> $<TARGET_FILE:rex::gpu-xenos>
            $<TARGET_FILE_DIR:specops>
    COMMENT "Copying ReXGlue SDK shared libraries next to the executable")
