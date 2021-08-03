################################################################################
# Plugin Capture Notifier Standalone CMake
# Author: Eduardo Almeida, Pumatronix
################################################################################
cmake_minimum_required(VERSION 3.8.2)

# set the project name and version
project(capture_notifier VERSION 1.0)

# update version in header file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/capture_notifier.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/src/capture_notifier.h
)

# include helpers directory
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# sources associated with the binary
set(CAPTURE_NOTIFIER_SRC
    src/capture_notifier.cpp
    src/http/handle_post.cpp
    src/http/http_response.cpp
    src/config/config_parser.cpp
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
add_executable(capture_notifier ${CAPTURE_NOTIFIER_SRC})

# libs
add_library(simple_log STATIC ${LIB_LOG_SRC})
add_library(file_handler STATIC ${LIB_FILE_SRC})
add_library(blocking_queue STATIC ${LIB_QUEUE_SRC})
add_library(camera_channel STATIC ${LIB_CAMERA_SRC})

target_link_libraries(
    capture_notifier
    simple_log
    file_handler
    blocking_queue
    camera_channel
    ${CURL_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    nlohmann_json::nlohmann_json
)
