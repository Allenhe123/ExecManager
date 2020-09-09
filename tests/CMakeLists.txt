cmake_minimum_required(VERSION 3.10)
include(GNUInstallDirs)

project(execmgr-unit-test)

include_directories(${CMAKE_BINARY_DIR}/config)

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
add_executable(em-test ${DIR_SRCS})
target_link_libraries(em-test pthread execmgr)

