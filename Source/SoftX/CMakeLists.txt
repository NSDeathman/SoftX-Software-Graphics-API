cmake_minimum_required(VERSION 3.15)
project(SoftX LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

add_library(SoftX STATIC)

file(GLOB_RECURSE SOFTX_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

file(GLOB_RECURSE SOFTX_PUBLIC_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)

target_sources(SoftX PRIVATE ${SOFTX_SOURCES})
target_sources(SoftX PUBLIC  ${SOFTX_PUBLIC_HEADERS})

target_include_directories(SoftX
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../AfterMath/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(SoftX
    PRIVATE
        SOFTX_IMPLEMENTATION
        _LIB
        $<$<CONFIG:Debug>:_DEBUG>
        $<$<CONFIG:Release>:NDEBUG>
        $<$<CXX_COMPILER_ID:MSVC>:$<$<EQUAL:${CMAKE_SIZEOF_VOID_P},4>:WIN32>>
)

if(MSVC)
    add_compile_options(/W4 /sdl)
    add_compile_options(/fp:fast)
    target_compile_options(SoftX PRIVATE
        $<$<CONFIG:Release>:/GL /FR>
    )
endif()

set_target_properties(SoftX PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
    UNITY_BUILD ON
)

set_target_properties(SoftX PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/Source/SoftX/lib/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>"
)

source_group("Source"  FILES ${SOFTX_SOURCES})
source_group("Include" FILES ${SOFTX_PUBLIC_HEADERS})