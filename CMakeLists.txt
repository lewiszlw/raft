cmake_minimum_required(VERSION 3.15)
project(raft)

set(CMAKE_CXX_STANDARD 17)

# find the protobuf compiler and libraries
find_package(Protobuf REQUIRED)

# check if protobuf was found
if(PROTOBUF_FOUND)
    message ("protobuf found")
else()
    message (FATAL_ERROR "Cannot find Protobuf")
endif()

# Generate the .h and .cxx files
PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS proto/raft.proto)

# Print path to generated files
message ("PROTO_SRCS = ${PROTO_SRCS}")
message ("PROTO_HDRS = ${PROTO_HDRS}")

message ("PROTOBUF_INCLUDE_DIRS = ${PROTOBUF_INCLUDE_DIRS}")
message ("PROTOBUF_LIBRARIES = ${PROTOBUF_LIBRARIES}")

add_executable(proto_test src/proto_test.cpp ${PROTO_SRCS} ${PROTO_HDRS})
target_include_directories(proto_test PUBLIC ${PROTOBUF_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(proto_test PUBLIC ${PROTOBUF_LIBRARIES})
