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
)

# ---------------------------
# Project sources / includes
# ---------------------------
# Emulator-specific sources
set(EMULATOR_SOURCES
  src/main_new.cpp
  src/desktop_display_sdl2.cpp
  src/hardware_shim.cpp
  src/oled_service.cpp
)

add_executable(${PROJECT_NAME}
  ${EMULATOR_SOURCES}
  ${POCKETMAGE_SOURCES_MANUAL}
)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${POCKETMAGE_INCLUDE}
)

# Definitions & compile options
target_compile_definitions(${PROJECT_NAME} PRIVATE DESKTOP_EMULATOR)
if (MSVC)
  target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX)
endif()
if (WIN32)
  # Prevent SDL from redefining main() to SDL_main on Windows
  target_compile_definitions(${PROJECT_NAME} PRIVATE SDL_MAIN_HANDLED)
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
# Post-build: copy assets & DLLs
# ---------------------------
# Copy data directory (if it exists)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:${PROJECT_NAME}>/data
  VERBATIM
)

# On Windows shared builds, copy SDL2/SDL2_ttf DLLs next to the .exe
if (WIN32)
  foreach(tgt IN ITEMS SDL2::SDL2 SDL2_ttf::SDL2_ttf)
    if (TARGET ${tgt})
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:${tgt}> $<TARGET_FILE_DIR:${PROJECT_NAME}>
      )
    endif()
  endforeach()
endif()
