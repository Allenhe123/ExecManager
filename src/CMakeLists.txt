cmake_minimum_required(VERSION 3.10)
include(GNUInstallDirs)

project(ExecMgr)

add_compile_options("-O0")
set(CMAKE_CXX_STANDARD 14)
if (NOT CMAKE_BUILD_TYPE)
 set(CMAKE_BUILD_TYPE "Release")
endif()

find_package(Protobuf REQUIRED)
if (NOT Protobuf_FOUND)
    message(FATAL_ERROR "Protobuf Not Found!")
else()
    message(STATUS "##Found Protobuf: ${PROTOBUF_INCLUDE_DIR} ${PROTOBUF_LIBRARIES}")
    include_directories(${PROTOBUF_INCLUDE_DIR})
    link_directories(${PROTOBUF_LIB_DIR})
endif (NOT Protobuf_FOUND)

include_directories(${CMAKE_BINARY_DIR}/config)

AUX_SOURCE_DIRECTORY(. DIR_SRCS)

add_library(execmgr SHARED ${DIR_SRCS})
target_link_libraries(execmgr ${PROTOBUF_LIB_DIR} task_proto)

install(TARGETS execmgr LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
