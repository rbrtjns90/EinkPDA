cmake_minimum_required(VERSION 3.16)
project(PocketMage_Desktop_Emulator)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Include directories
include_directories(${SDL2_INCLUDE_DIRS})
include_directories(${SDL2_TTF_INCLUDE_DIRS})
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/../Code/PocketMage_V3/include)

# Source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Link directories and libraries
target_link_directories(${PROJECT_NAME} PRIVATE ${SDL2_LIBRARY_DIRS} ${SDL2_TTF_LIBRARY_DIRS})
target_link_libraries(${PROJECT_NAME} 
    ${SDL2_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
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
