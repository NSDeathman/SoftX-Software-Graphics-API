cmake_minimum_required(VERSION 3.15)
project(OptickCore LANGUAGES CXX)

file(GLOB_RECURSE OPTICK_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

file(GLOB_RECURSE OPTICK_PUBLIC_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
)

add_library(OptickCore SHARED)

target_sources(OptickCore PRIVATE ${OPTICK_SOURCES})
target_sources(OptickCore PUBLIC  ${OPTICK_PUBLIC_HEADERS})

target_include_directories(OptickCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(OptickCore
    PRIVATE
        USE_OPTICK=1
        OPTICK_FIBERS=1
        OPTICK_LIB=1
        OPTICK_EXPORTS
        _CRT_SECURE_NO_WARNINGS
        MT_INSTRUMENTED_BUILD
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<OR:$<CONFIG:Release>,$<CONFIG:ReleaseWithProfiling>>:NDEBUG>
)

target_compile_features(OptickCore PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(OptickCore PRIVATE /W4)
endif()

set_target_properties(OptickCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY
        "${CMAKE_SOURCE_DIR}/Source/SoftX/lib/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>"
    RUNTIME_OUTPUT_DIRECTORY
        "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>"
    LIBRARY_OUTPUT_DIRECTORY
        "${CMAKE_SOURCE_DIR}/bin/$<CONFIG>"
    UNITY_BUILD ON
)

source_group("Source"  FILES ${OPTICK_SOURCES})
source_group("Include" FILES ${OPTICK_PUBLIC_HEADERS})
