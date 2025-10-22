cmake_minimum_required(VERSION 3.10)
set(APP_NAME haversine)
set(PROJ_DIR ${CMAKE_SOURCE_DIR}/${APP_NAME})
set(SOURCE_DIR ${PROJ_DIR}/sauce)

project(${APP_NAME} C)

set(CMAKE_C_STANDARD 17)

set(BUILD_TYPE_SUBDIR "")

if (NOT DEFINED CMAKE_BUILD_TYPE)
#   set(CMAKE_CONFIGURATION_TYPES Debug Release ODebug)

    if(NOT CMAKE_GENERATOR MATCHES "Visual Studio")
        set(CMAKE_C_FLAGS_DEBUG "-g -O0")
        set(CMAKE_C_FLAGS_ODEBUG "-g -O2 -DNDEBUG")
        set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

        # Provide a default build type if none was specified
        if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
            set(CMAKE_BUILD_TYPE Debug)
        endif()
    else()
        set(CMAKE_C_FLAGS_ODEBUG "/O2 /Zi /DODEBUG /DDEBUG")
    endif()

else()
    if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
        add_compile_options(-g -gcodeview)
        add_link_options(-fuse-ld=lld -g -Wl,--pdb=)
    elseif (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        add_compile_options(-g -gcodeview)
        add_link_options(-fuse-ld=lld -g -Wl,--pdb=)
    endif()

    set(BUILD_TYPE_SUBDIR "/${CMAKE_BUILD_TYPE}")
endif()

file(GLOB_RECURSE SRC_FILES "${SOURCE_DIR}/*.c")
file(GLOB_RECURSE HEADER_FILES "${SOURCE_DIR}/*.h")

set(BIN_DIR "${CMAKE_SOURCE_DIR}/bin${BUILD_TYPE_SUBDIR}")

# Set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${BIN_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BIN_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}")

# Create executable from all .c files
add_executable(${APP_NAME} ${SRC_FILES} ${HEADER_FILES})

# Make sure object files go into bin-int
set_target_properties(
    ${APP_NAME} PROPERTIES
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS ON
    RUNTIME_OUTPUT_DIRECTORY "${BIN_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${BIN_DIR}"
    LIBRARY_OUTPUT_DIRECTORY "${BIN_DIR}"
    OUTPUT_NAME "${APP_NAME}"
)

target_include_directories(
    ${APP_NAME}
    PRIVATE
        ${SOURCE_DIR}
)

# Make Visual Studio solution a tad less obnoxious
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${APP_NAME})
source_group(TREE "${PROJ_DIR}" FILES ${SRC_FILES})
source_group(TREE "${PROJ_DIR}" FILES ${HEADER_FILES})