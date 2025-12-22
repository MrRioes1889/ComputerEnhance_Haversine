cmake_minimum_required(VERSION 3.10)
set(APP_NAME haversine)
set(PROJ_DIR ${CMAKE_SOURCE_DIR}/${APP_NAME})
set(SOURCE_DIR ${PROJ_DIR}/sauce)

project(${APP_NAME} C)

set(CMAKE_C_STANDARD 17)

set(BUILD_TYPE_SUBDIR "")

if (NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_CONFIGURATION_TYPES Debug Release ODebug)

    if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
        set(CMAKE_C_FLAGS_DEBUG "-g -O0 -Wall -Werror -DDEBUG")
        set(CMAKE_C_FLAGS_ODEBUG "-g -O2 -Wall -Werror -DDEBUG -DODEBUG")
        set(CMAKE_C_FLAGS_RELEASE "-O3 -Wall -Werror -DNDEBUG")

        # Provide a default build type if none was specified
        if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
            set(CMAKE_BUILD_TYPE Debug)
        endif()
    else()
        set(CMAKE_C_FLAGS_ODEBUG "/O2 /Zi /DODEBUG /DDEBUG /Wall /W4")
        set(CMAKE_EXE_LINKER_FLAGS_ODEBUG "/DEBUG /OPT:REF")
        add_compile_options(/Wall /W4)
    endif()

else()
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(DEBUG)

        if ((CMAKE_C_COMPILER_ID STREQUAL "Clang") OR (CMAKE_C_COMPILER_ID STREQUAL "GNU"))
            add_compile_options(-g -gcodeview -Wall -Werror)
            add_link_options(-fuse-ld=lld -g -Wl,--pdb=)
        endif()
    elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_definitions(NDEBUG)

        if ((CMAKE_C_COMPILER_ID STREQUAL "Clang") OR (CMAKE_C_COMPILER_ID STREQUAL "GNU"))
            add_compile_options(-Wall -Werror)
        endif()
    elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
        add_compile_definitions(DEBUG ODEBUG)

        if ((CMAKE_C_COMPILER_ID STREQUAL "Clang") OR (CMAKE_C_COMPILER_ID STREQUAL "GNU"))
            add_compile_options(-g -gcodeview -Wall -Werror)
            add_link_options(-fuse-ld=lld -g -Wl,--pdb=)
        endif()
    endif()

    set(BUILD_TYPE_SUBDIR "/${CMAKE_BUILD_TYPE}")
endif()

# message("Using compiler: ${CMAKE_C_COMPILER_ID}")

file(GLOB_RECURSE SRC_FILES "${SOURCE_DIR}/*.c")
file(GLOB_RECURSE HEADER_FILES "${SOURCE_DIR}/*.h")

set(BIN_DIR "${CMAKE_SOURCE_DIR}/bin${BUILD_TYPE_SUBDIR}")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BIN_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BIN_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}")

add_executable(${APP_NAME} ${SRC_FILES} ${HEADER_FILES})

set_target_properties(
    ${APP_NAME} PROPERTIES
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS ON
    RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${BIN_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${BIN_DIR}"
    OUTPUT_NAME "${APP_NAME}"
    INCLUDE_DIRECTORIES "${SOURCE_DIR}"
    LINK_DIRECTORIES "${SOURCE_DIR}/asm"
    LINK_LIBRARIES "winmm"
)

# Make Visual Studio solution a tad less obnoxious
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${APP_NAME})
source_group(TREE "${PROJ_DIR}" FILES ${SRC_FILES})
source_group(TREE "${PROJ_DIR}" FILES ${HEADER_FILES})