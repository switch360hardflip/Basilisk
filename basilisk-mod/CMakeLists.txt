cmake_minimum_required(VERSION 3.23.2)
project(basilisk-mod)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(IS_DEBUG_BUILD CMAKE_BUILD_TYPE STREQUAL "Debug")

file(GLOB SOURCE_FILES "src/*.c")

file(GLOB TYPE_FILES "src/types/*.*")

file(GLOB VS_SHADER_FILES "shaders/vert/*.*")
file(GLOB FS_SHADER_FILES "shaders/frag/*.*")

file(GLOB_RECURSE GENERATE_FILES "generated/*.*")
file(GLOB_RECURSE HEADER_FILES "include/*.h")
file(GLOB PROJECT_FILES "./*" LIST_DIRECTORIES false)

add_library(${PROJECT_NAME} SHARED
    ${SOURCE_FILES}
    ${HEADER_FILES}
    ${GENERATE_FILES}

    ${VS_SHADER_FILES}
    ${FS_SHADER_FILES}

    ${TYPE_FILES}
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    LINKER_LANGUAGE CXX
)

if (DEFINED RENDERDOC_PATH)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    RENDERDOC_PATH="${RENDERDOC_PATH}"
)
endif()

source_group("Views" FILES ${VIEWS})
source_group("Source Files" FILES ${SOURCE_FILES})

source_group("Shaders/vert" FILES ${VS_SHADER_FILES})
source_group("Shaders/frag" FILES ${FS_SHADER_FILES})

source_group("Source Files/Types" FILES ${TYPE_FILES})
source_group("Project Files" FILES ${PROJECT_FILES})
source_group("Headers" FILES ${HEADER_FILES})
source_group("Generated Files" FILES ${GENERATE_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/basilisk-core/include
    ${CMAKE_SOURCE_DIR}/basilisk-core/generated
    #${CMAKE_SOURCE_DIR}/basilisk-steam/include
    ${CMAKE_SOURCE_DIR}/basilisk-gfx/include
    ${CMAKE_SOURCE_DIR}/basilisk-gfx/generated
    ${CMAKE_SOURCE_DIR}/basilisk-gfx/src
    ${CMAKE_SOURCE_DIR}/basilisk-mod/include
    ${CMAKE_SOURCE_DIR}/external
    #${CMAKE_SOURCE_DIR}/external/GFSDK_Aftermath_Lib
    ${CMAKE_SOURCE_DIR}/external/glslang
    ${CMAKE_SOURCE_DIR}/external/SPIRV-Cross
    external/
    include/
    generated/
    src/
    ${CMAKE_SOURCE_DIR}/include/objects/
    ${CMAKE_SOURCE_DIR}/external/freetype/include
)

if (MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /wd6255 /wd4047 /wd4090 /wd4024 /wd4133)
endif()

if (${IS_DEBUG_BUILD})
    target_link_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/external/
        ${CMAKE_SOURCE_DIR}/build/external/glslang/glslang/Debug
        ${CMAKE_SOURCE_DIR}/build/external/glslang/glslang
        ${CMAKE_SOURCE_DIR}/build/external/glslang/glslang/OSDependent/Unix
        ${CMAKE_SOURCE_DIR}/build/external/glslang/glslang/OSDependent/Windows/Debug
        ${CMAKE_SOURCE_DIR}/build/external/glslang/SPIRV/Debug
        ${CMAKE_SOURCE_DIR}/build/external/glslang/SPIRV
        ${CMAKE_SOURCE_DIR}/build/external/SPIRV-Cross/Debug
        ${CMAKE_SOURCE_DIR}/build/external/SPIRV-Cross
        ${CMAKE_SOURCE_DIR}/external/freetype/objs
        ${CMAKE_SOURCE_DIR}/external/freetype/objs/.libs
    )
else()
target_link_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/external/
    ${CMAKE_SOURCE_DIR}/build/external/glslang/glslang/Release
    ${CMAKE_SOURCE_DIR}/build/external/glslang/glslang
    ${CMAKE_SOURCE_DIR}/build/external/glslang/glslang/OSDependent/Unix
    ${CMAKE_SOURCE_DIR}/build/external/glslang/glslang/OSDependent/Windows/Release
    ${CMAKE_SOURCE_DIR}/build/external/glslang/SPIRV/Release
    ${CMAKE_SOURCE_DIR}/build/external/glslang/SPIRV
    ${CMAKE_SOURCE_DIR}/build/external/SPIRV-Cross/Release
    ${CMAKE_SOURCE_DIR}/build/external/SPIRV-Cross
    ${CMAKE_SOURCE_DIR}/external/freetype/objs
    ${CMAKE_SOURCE_DIR}/external/freetype/objs/.libs
)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    basilisk-core
    basilisk-gfx
    #basilisk-steam
    #steam_api64
    #GFSDK_Aftermath_Lib.x64
    freetype
)

if (WIN32)
target_link_libraries(${PROJECT_NAME} PRIVATE
    vulkan-1
    dwmapi
)
else()
target_link_libraries(${PROJECT_NAME} PRIVATE
    vulkan
)
endif()

if (WIN32 AND ${IS_DEBUG_BUILD})
    target_link_libraries(${PROJECT_NAME} PRIVATE
        GenericCodeGend
        glslangd
        glslang-default-resource-limitsd
        MachineIndependentd
        SPIRVd
        spirv-cross-cd
        spirv-cross-cored
        spirv-cross-glsld
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
        OSDependent
        GenericCodeGen
        glslang
        glslang-default-resource-limits
        MachineIndependent
        SPIRV
        spirv-cross-c
        spirv-cross-core
        spirv-cross-glsl
    )
endif()
