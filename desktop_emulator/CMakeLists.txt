cmake_minimum_required(VERSION 3.16)
project(PocketMage_Desktop_Emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------
# SDL2 / SDL2_ttf discovery
# ---------------------------
set(_sdl_config_ok FALSE)

# Prefer CMake CONFIG packages on Windows (vcpkg or official SDL2 devel zips)
if (WIN32)
  find_package(SDL2 CONFIG QUIET)       # provides SDL2::SDL2 and (usually) SDL2::SDL2main
  if (TARGET SDL2::SDL2)
    set(_sdl_config_ok TRUE)
    find_package(SDL2_ttf CONFIG REQUIRED)  # provides SDL2_ttf::SDL2_ttf
  endif()
endif()

# Fallback to pkg-config (macOS/Linux; also works for MSYS2/MinGW on Windows)
if (NOT _sdl_config_ok)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED sdl2)
  pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

  add_library(SDL2::SDL2 INTERFACE IMPORTED)
  target_include_directories(SDL2::SDL2 INTERFACE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(SDL2::SDL2 INTERFACE ${SDL2_LIBRARIES})

  add_library(SDL2_ttf::SDL2_ttf INTERFACE IMPORTED)
  target_include_directories(SDL2_ttf::SDL2_ttf INTERFACE ${SDL2_TTF_INCLUDE_DIRS})
  target_link_libraries(SDL2_ttf::SDL2_ttf INTERFACE ${SDL2_TTF_LIBRARIES})
endif()

# Homebrew helper for macOS 
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
    ${POCKETMAGE_SRC}/PERIODIC.cpp
    ${POCKETMAGE_SRC}/BT.cpp
    ${POCKETMAGE_SRC}/PokedexUI.cpp
    ${POCKETMAGE_SRC}/PocketMageGraphics.cpp
)

# ---------------------------
# Project sources / includes
# ---------------------------
# Base emulator sources (cross-platform)
set(EMULATOR_SOURCES
  src/main_new.cpp
  src/hardware_shim.cpp
  src/oled_service.cpp
  src/text_utils_desktop.cpp
)

# Platform-specific display backend
if(WIN32)
  message(STATUS "Building for Windows - using Windows-specific display backend")
  list(APPEND EMULATOR_SOURCES src/desktop_display_sdl2_windows.cpp)
elseif(UNIX AND NOT APPLE)
  message(STATUS "Building for Linux - using Linux-specific display backend")
  list(APPEND EMULATOR_SOURCES src/desktop_display_sdl2_linux.cpp)
elseif(APPLE)
  message(STATUS "Building for macOS - using Mac display backend")
  list(APPEND EMULATOR_SOURCES src/desktop_display_sdl2.cpp)
else()
  message(STATUS "Building for unknown platform - using cross-platform display backend")
  list(APPEND EMULATOR_SOURCES src/desktop_display_sdl2.cpp)
endif()

add_executable(${PROJECT_NAME}
  ${EMULATOR_SOURCES}
  ${POCKETMAGE_SOURCES_MANUAL}
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${POCKETMAGE_INCLUDE}
)

# ---------------------------
# Platform-specific definitions & compile options
# ---------------------------
target_compile_definitions(${PROJECT_NAME} PRIVATE DESKTOP_EMULATOR)

# Windows-specific settings
if (WIN32)
  message(STATUS "Applying Windows-specific build settings")
  target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED)
  if (MSVC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
    # Enable Windows-specific optimizations
    target_compile_options(${PROJECT_NAME} PRIVATE /W3)
  endif()
endif()

# Linux-specific settings
if (UNIX AND NOT APPLE)
  message(STATUS "Applying Linux-specific build settings")
  target_compile_definitions(${PROJECT_NAME} PRIVATE __LINUX__)
  # Enable Linux-specific optimizations
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O2)
endif()

# macOS-specific settings
if (APPLE)
  message(STATUS "Applying macOS-specific build settings")
  target_compile_definitions(${PROJECT_NAME} PRIVATE __APPLE__)
  # Enable macOS-specific optimizations
  target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O2)
  # macOS-specific frameworks if needed
  # target_link_libraries(${PROJECT_NAME} PRIVATE "-framework Cocoa")
endif()

# ---------------------------
# Link libraries
# ---------------------------
set(_sdl_targets SDL2::SDL2 SDL2_ttf::SDL2_ttf)
if (TARGET SDL2::SDL2main)
  # Link SDL2main if it's available (fixes WinMain/SDL_main mismatch on Windows)
  list(INSERT _sdl_targets 0 SDL2::SDL2main)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE ${_sdl_targets})

# ---------------------------
# Post-build: copy assets & platform-specific files
# ---------------------------
# Copy data directory (if it exists)
if(EXISTS ${CMAKE_SOURCE_DIR}/data)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${PROJECT_NAME}>/data
    COMMENT "Copying data directory"
    VERBATIM
  )
endif()

# Copy fonts directory (cross-platform)
if(EXISTS ${CMAKE_SOURCE_DIR}/fonts)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:${PROJECT_NAME}>/fonts
    COMMENT "Copying fonts directory"
    VERBATIM
  )
endif()

# Platform-specific post-build steps
if (WIN32)
  message(STATUS "Setting up Windows post-build steps")
  # Copy SDL2/SDL2_ttf DLLs next to the .exe on Windows
  foreach(tgt IN ITEMS SDL2::SDL2 SDL2_ttf::SDL2_ttf)
    if (TARGET ${tgt})
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${tgt}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying ${tgt} DLL"
      )
    endif()
  endforeach()
elseif(UNIX AND NOT APPLE)
  message(STATUS "Setting up Linux post-build steps")
  # Linux-specific post-build steps (if any)
elseif(APPLE)
  message(STATUS "Setting up macOS post-build steps")
  # macOS-specific post-build steps (if any)
endif()
