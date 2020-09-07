cmake_minimum_required(VERSION 3.10)
include(GNUInstallDirs)

project(execmgr-unit-test)

find_package(Protobuf REQUIRED)
if (NOT Protobuf_FOUND)
    message(FATAL_ERROR "Protobuf Not Found!")
else()
    message(STATUS "##Found Protobuf: ${PROTOBUF_INCLUDE_DIR} ${PROTOBUF_LIBRARIES}")
    include_directories(${PROTOBUF_INCLUDE_DIR})
    link_directories(${PROTOBUF_LIB_DIR})
endif (NOT Protobuf_FOUND)

find_package(GTest REQUIRED)
if (NOT GTest_FOUND)
    message(FATAL_ERROR "GTest Not Found")
else ()
    message(STATUS "##Found GTest: ${GTEST_INCLUDE_DIR} ${GTEST_LIBRARIES}")
    include_directories(${GTEST_INCLUDE_DIR})
endif (NOT GTest_FOUND)

include_directories(${CMAKE_BINARY_DIR}/config)
# link_directories(${CMAKE_BINARY_DIR}/config)
# link_directories(${CMAKE_BINARY_DIR}/src)

AUX_SOURCE_DIRECTORY(. DIR_SRCS)
add_executable(em-test ${DIR_SRCS})
target_link_libraries(em-test pthread ${Protobuf_LIBRARIES} task_proto execmgr ${GTEST_LIBRARIES})

install(TARGETS em-test DESTINATION ${CMAKE_INSTALL_LIBDIR})
