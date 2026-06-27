cmake_minimum_required(VERSION 3.12)

include_directories(
        ${CMAKE_CURRENT_LIST_DIR}/opus/include
)

find_package(shadowhook REQUIRED CONFIG)
add_subdirectory(opus)
add_subdirectory(multiplayer)

# --- APK Size Optimization ---
# This section optimizes the final APK size by stripping unnecessary
# debug symbols from the native libraries in the release build.

# For the 'opus' library, set optimization level to O3 for better performance.
target_compile_options(opus PRIVATE -O3)

# For the main 'multiplayer' library, apply different flags for debug and release builds.
target_compile_options(multiplayer PRIVATE
        $<$<CONFIG:DEBUG>:-g>  # Include debug symbols only for debug builds
        $<$<CONFIG:RELEASE>:-s> # Strip all symbols for release builds to reduce size
)