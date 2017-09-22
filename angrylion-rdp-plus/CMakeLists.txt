cmake_minimum_required(VERSION 2.8)

project(angrylion-plus)

set(CMAKE_CXX_STANDARD 14)
set(COMPILE_DEFINITIONS -O3 -Wall -Wextra -Winline)

find_package(Git REQUIRED)
find_package(PythonInterp 3.2 REQUIRED)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

# RDP core library
set(PATH_CORE "core")
set(PATH_VERSION_IN "${PATH_CORE}/version.h.in")
set(PATH_VERSION_OUT "${PATH_CORE}/version.h")

add_custom_command(
    OUTPUT ${PATH_VERSION_OUT}
    COMMAND
        ${PYTHON_EXECUTABLE}
        "${CMAKE_SOURCE_DIR}/make_version.py"
        "${CMAKE_SOURCE_DIR}/${PATH_VERSION_IN}"
        "${CMAKE_SOURCE_DIR}/${PATH_VERSION_OUT}"
    COMMENT "Generate Git version"
)

file(GLOB SOURCES_CORE "${PATH_CORE}/*.c" "${PATH_CORE}/*.cpp")
add_library(core STATIC ${SOURCES_CORE} ${PATH_VERSION_OUT})

if ( NOT MINGW )
    set_target_properties(core PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif ( NOT MINGW )

include_directories(${CMAKE_SOURCE_DIR})

# OpenGL screen library
set(PATH_PLUGIN_COMMON "plugin-common")

file(GLOB SOURCES_PLUGIN_COMMON "${PATH_PLUGIN_COMMON}/*.c")
add_library(plugin-common STATIC ${SOURCES_PLUGIN_COMMON})

if ( NOT MINGW )
    set_target_properties(plugin-common PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif ( NOT MINGW )

# Mupen64Plus GFX plugin
set(NAME_PLUGIN_M64P "mupen64plus-video-${CMAKE_PROJECT_NAME}")
set(PATH_PLUGIN_M64P "plugin-mupen64plus")

file(GLOB SOURCES_PLUGIN_M64P "${PATH_PLUGIN_M64P}/*.c")
add_library(${NAME_PLUGIN_M64P} SHARED ${SOURCES_PLUGIN_M64P})

set_target_properties(${NAME_PLUGIN_M64P} PROPERTIES PREFIX "")

target_link_libraries(${NAME_PLUGIN_M64P} core plugin-common ${OPENGL_LIBRARIES})

# RDP retracer
set(NAME_RETRACER "${CMAKE_PROJECT_NAME}-retrace")
set(PATH_RETRACER "retrace")

file(GLOB SOURCES_RETRACE "${PATH_RETRACER}/*.c")
add_executable(${NAME_RETRACER} ${SOURCES_RETRACE})

target_link_libraries(${NAME_RETRACER} core pthread SDL2)
