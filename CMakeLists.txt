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


# 参考 https://github.com/hezhihua/horse-raft 引入 RocksDB 依赖
add_custom_target(thirdparty)
include(ExternalProject)
 
 #gflags 库
set(GFLAG_ROOT          ${CMAKE_BINARY_DIR}/thirdparty/gflags-2.2.2)
set(GFLAG_LIB_DIR       ${GFLAG_ROOT}/lib)
set(GFLAG_INCLUDE_DIR   ${GFLAG_ROOT}/include)
 
set(GFLAG_URL           https://github.com/gflags/gflags/archive/v2.2.2.tar.gz)
#set(GFLAG_URL           https://github.com.cnpmjs.org/gflags/gflags/archive/v2.2.2.tar.gz)
set(GFLAG_CONFIGURE     cd ${GFLAG_ROOT}/src/gflags-2.2.2 && cmake  .)
set(GFLAG_MAKE          cd ${GFLAG_ROOT}/src/gflags-2.2.2 && make)
set(GFLAG_INSTALL       cd ${GFLAG_ROOT}/src/gflags-2.2.2 && make install)

ExternalProject_Add(gflags-2.2.2
        URL                   ${GFLAG_URL}
        DOWNLOAD_NAME         gflag-2.2.2.tar.gz
        PREFIX                ${GFLAG_ROOT}
        CONFIGURE_COMMAND     ${GFLAG_CONFIGURE}
        BUILD_COMMAND         ${GFLAG_MAKE}
        INSTALL_COMMAND       ${GFLAG_INSTALL}
)

add_dependencies(thirdparty gflags-2.2.2)


 #rocksdb 库
set(ROCKSDB_ROOT          ${CMAKE_BINARY_DIR}/thirdparty/rocksdb)
set(ROCKSDB_LIB_DIR       ${ROCKSDB_ROOT}/lib)
set(ROCKSDB_INCLUDE_DIR   ${ROCKSDB_ROOT}/include)
 
set(ROCKSDB_URL           https://github.com/facebook/rocksdb/archive/v6.14.5.tar.gz)
#set(ROCKSDB_URL           https://github.com.cnpmjs.org/facebook/rocksdb/archive/v6.14.5.tar.gz)
#set(ROCKSDB_CONFIGURE     cd ${ROCKSDB_ROOT}/src/rocksdb-6.14.5 && cmake -D CMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR}/third-party/rocksdb -DWITH_SNAPPY=ON .)
set(ROCKSDB_CONFIGURE     cd ${ROCKSDB_ROOT}/src/rocksdb-6.14.5 )
set(ROCKSDB_MAKE          cd ${ROCKSDB_ROOT}/src/rocksdb-6.14.5 && make static_lib)
set(ROCKSDB_INSTALL       cd ${ROCKSDB_ROOT}/src/rocksdb-6.14.5 && make PREFIX=${CMAKE_SOURCE_DIR}/third-party/rocksdb  install)

ExternalProject_Add(rocksdb-6.14.5
        URL                   ${ROCKSDB_URL}
        DOWNLOAD_NAME         rocksdb-6.14.5.tar.gz
        PREFIX                ${ROCKSDB_ROOT}
        CONFIGURE_COMMAND     ${ROCKSDB_CONFIGURE}
        BUILD_COMMAND         ${ROCKSDB_MAKE}
        INSTALL_COMMAND       ${ROCKSDB_INSTALL}
        BUILD_ALWAYS          1
)

add_dependencies(thirdparty rocksdb-6.14.5)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third-party/rocksdb/include)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/third-party/rocksdb/lib)
add_executable(rocksdb_test src/rocksdb_test.cpp)
target_link_libraries(rocksdb_test rocksdb ${CMAKE_DL_LIBS})  # https://stackoverflow.com/a/55386760

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

add_executable(raft_consensus_server src/raft_consensus_server.cpp)
target_link_libraries(raft_consensus_server 
    raft_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})
add_executable(raft_consensus_client src/raft_consensus_client.cpp)
target_link_libraries(raft_consensus_client 
    raft_grpc_proto
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF})