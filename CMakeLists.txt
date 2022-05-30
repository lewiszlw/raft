cmake_minimum_required(VERSION 3.15)
project(raft)

set(CMAKE_CXX_STANDARD 17)

# 参考 https://github.com/grpc/grpc/blob/master/examples/cpp/cmake/common.cmake 采用Fetch方式获取grpc依赖库
find_package(Threads REQUIRED)

include(FetchContent)
FetchContent_Declare(
  gRPC
  GIT_REPOSITORY https://github.com/grpc/grpc
  GIT_TAG        v1.28.0  # e.g v1.28.0
)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(gRPC)

# Since FetchContent uses add_subdirectory under the hood, we can use
# the grpc targets directly from this build.
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_GRPCPP grpc++)
if(CMAKE_CROSSCOMPILING)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
endif()

# 参考官方 helloworld 例子 https://github.com/grpc/grpc/blob/master/examples/cpp/helloworld/CMakeLists.txt
# Proto file
get_filename_component(raft_proto "proto/raft.proto" ABSOLUTE)
get_filename_component(raft_proto_path "${raft_proto}" PATH)

# Generated sources
set(raft_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/raft.pb.cc")
set(raft_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/raft.pb.h")
set(raft_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/raft.grpc.pb.cc")
set(raft_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/raft.grpc.pb.h")
add_custom_command(
      OUTPUT "${raft_proto_srcs}" "${raft_proto_hdrs}" "${raft_grpc_srcs}" "${raft_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${raft_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${raft_proto}"
      DEPENDS "${raft_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

# raft_grpc_proto
add_library(raft_grpc_proto
  ${raft_grpc_srcs}
  ${raft_grpc_hdrs}
  ${raft_proto_srcs}
  ${raft_proto_hdrs})
target_link_libraries(raft_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_executable(proto_test src/proto_test.cpp)
target_link_libraries(proto_test 
    raft_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})