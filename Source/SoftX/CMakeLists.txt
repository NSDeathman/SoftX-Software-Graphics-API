cmake_minimum_required(VERSION 3.15)
project(SoftX LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

add_library(SoftX STATIC)

set(SOFTX_SOURCES
    src/CPUDetector.cpp
    src/Device.cpp
    src/DeviceContext.cpp
    src/Texture.cpp
    src/DepthBuffer.cpp
    src/OcclusionQuery.cpp
)

set(SOFTX_PRIVATE_HEADERS
    src/CPUDetector.h
    src/RasterizerCommon.h
    src/DepthRasterizer.h
    src/Rasterizer.h
    src/TileGrid.h
    src/ThreadPool.h
    src/ThreadPoolManager.h
    src/ThreadUtils.h
    src/InternalTypes.h
)

set(SOFTX_PUBLIC_HEADERS
    include/DepthBuffer.h
    include/Device.h
    include/DeviceContext.h
    include/Exceptions.h
    include/LibInternal.h
    include/OcclusionQuery.h
    include/SoftX.h
    include/Texture.h
    include/ThirdPartyIncluding.h
    include/Types.h
)

target_sources(SoftX PRIVATE ${SOFTX_SOURCES} ${SOFTX_PRIVATE_HEADERS})
target_sources(SoftX PUBLIC ${SOFTX_PUBLIC_HEADERS})

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

source_group("Source Files\\Core"          			FILES 	src/Device.cpp
									src/DeviceContext.cpp
									src/OcclusionQuery.cpp)

source_group("Source Files\\Tools\\CPU Detection"         	FILES 	src/CPUDetector.h
									src/CPUDetector.cpp)

source_group("Source Files\\Tools\\Threading"         		FILES 	src/ThreadPool.h
									src/ThreadPoolManager.h
									src/ThreadUtils.h)

source_group("Source Files\\Tools\\Tile binning"     		FILES 	src/TileGrid.h)

source_group("Source Files\\Resources"     			FILES 	src/InternalTypes.h src/Texture.cpp src/DepthBuffer.cpp)

source_group("Source Files\\Rastization"     			FILES 	src/RasterizerCommon.h
									src/Rasterizer.h
									src/DepthRasterizer.h)

source_group("Header Files\\API"           			FILES 	include/SoftX.h
									include/Device.h
									include/DeviceContext.h
									include/OcclusionQuery.h
									include/Types.h
                                                 			include/DepthBuffer.h
									include/Exceptions.h
									include/Texture.h
                                                 			include/LibInternal.h
									include/ThirdPartyIncluding.h)
