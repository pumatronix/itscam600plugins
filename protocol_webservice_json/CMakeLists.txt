################################################################################
# Plugin ITSCAM Protocol to WebService REST JSON Standalone CMake
# Author: Alexandre Krzyzanovski, Pumatronix
################################################################################
cmake_minimum_required(VERSION 3.8.2)

# set the project name and version
project(protocol_webservice_json VERSION 1.0)

# update version in header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol_webservice_json.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/src/protocol_webservice_json.h
)

# include helpers directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# sources associated with the binary
set(PROTOCOL_WEBSERVICE_JSON_SRC
    src/protocol_webservice_json.cpp
    src/http/handle_post.cpp
)

# helper libraries
set(LIB_LOG_SRC src/helpers/log.cpp)
set(LIB_FILE_SRC src/helpers/file_handler.cpp)
set(LIB_QUEUE_SRC src/helpers/blocking_queue.cpp)
set(LIB_CAMERA_SRC
    src/camera/channel.cpp
    src/camera/itscam/itscam300.cpp
    src/camera/itscam/recebedados.cpp
    src/camera/itscam/compat_pumatronix.cpp
    src/camera/jlib/jos/impl/jtime_impl_linux.cpp
    src/camera/jlib/jfs/filesystem.cpp
)

# show make commands before running it
set(CMAKE_VERBOSE_MAKEFILE on)

# specify the C++11 standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# adding compiler warnings
set(CMAKE_CXX_WARNINGS_FLAGS
    "-Wall \
    -Wextra \
    -Werror \
    -Wshadow \
    -Wunused \
    -pedantic-errors"
)
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_WARNINGS_FLAGS}")

# add thread support
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)

# executable
add_executable(protocol_webservice_json ${PROTOCOL_WEBSERVICE_JSON_SRC})

# libs
add_library(simple_log STATIC ${LIB_LOG_SRC})
add_library(file_handler STATIC ${LIB_FILE_SRC})
add_library(blocking_queue STATIC ${LIB_QUEUE_SRC})
add_library(camera_channel STATIC ${LIB_CAMERA_SRC})

target_link_libraries(
    protocol_webservice_json
    simple_log
    camera_channel
    file_handler
    blocking_queue
    ${CURL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    nlohmann_json::nlohmann_json
)
