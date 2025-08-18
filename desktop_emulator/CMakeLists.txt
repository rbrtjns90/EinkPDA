cmake_minimum_required(VERSION 3.16)
project(PocketMage_Desktop_Emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2 and SDL2_ttf
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Add Homebrew paths for macOS
if(APPLE)
    execute_process(COMMAND brew --prefix OUTPUT_VARIABLE HOMEBREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    list(APPEND CMAKE_PREFIX_PATH "${HOMEBREW_PREFIX}")
    include_directories("${HOMEBREW_PREFIX}/include")
    link_directories("${HOMEBREW_PREFIX}/lib")
endif()

# PocketMage source paths
set(POCKETMAGE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../Code/PocketMage_V3)
set(POCKETMAGE_SRC ${POCKETMAGE_ROOT}/src)
set(POCKETMAGE_INCLUDE ${POCKETMAGE_ROOT}/include)

list(FILTER POCKETMAGE_SOURCES EXCLUDE REGEX ".*OLEDFunc\\.cpp$")
# PocketMage source files (actual app code) - sysFunc.cpp excluded due to updateKeypress conflict
set(POCKETMAGE_SOURCES_MANUAL
    ${POCKETMAGE_SRC}/globals.cpp
    ${POCKETMAGE_SRC}/assets.cpp
    ${POCKETMAGE_SRC}/PocketMageV3.cpp
    # ${POCKETMAGE_SRC}/USB.cpp  # Excluded - requires ESP32-specific headers
    ${POCKETMAGE_SRC}/TXT.cpp
    ${POCKETMAGE_SRC}/FILEWIZ.cpp
    ${POCKETMAGE_SRC}/TASKS.cpp
    ${POCKETMAGE_SRC}/CALENDAR.cpp
    ${POCKETMAGE_SRC}/SETTINGS.cpp
    ${POCKETMAGE_SRC}/HOME.cpp
    ${POCKETMAGE_SRC}/JOURNAL.cpp
    ${POCKETMAGE_SRC}/LEXICON.cpp
    ${POCKETMAGE_SRC}/POKEDEX.cpp
    ${POCKETMAGE_SRC}/BT.cpp
)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${POCKETMAGE_INCLUDE})
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_TTF_INCLUDE_DIRS})

# Emulator-specific source files
set(EMULATOR_SOURCES
    src/main_new.cpp
    src/desktop_display_sdl2.cpp
    src/hardware_shim.cpp
)

# Create executable
add_executable(${PROJECT_NAME} 
    ${EMULATOR_SOURCES}
    ${POCKETMAGE_SOURCES_MANUAL}
)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    SDL2
    SDL2_ttf
)

# Compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE 
    ${SDL2_CFLAGS_OTHER}
    ${SDL2_TTF_CFLAGS_OTHER}
    -DDESKTOP_EMULATOR
)

# Compiler-specific options
if(MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Copy data directory to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${PROJECT_NAME}>/data
)
