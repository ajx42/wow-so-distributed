project(wow_rpc)

find_package(Threads REQUIRED)
add_subdirectory(../grpc ${CMAKE_CURRENT_BINARY_DIR}/grpc EXCLUDE_FROM_ALL)
message(STATUS "Using gRPC via add_subdirectory.")

set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)

if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()
    set(_GRPC_GRPCPP grpc++)
if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
endif()

# Proto file
get_filename_component(fs_proto "fs.proto" ABSOLUTE)
get_filename_component(fs_proto_path "${fs_proto}" PATH)

# Generated sources
set(fs_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/fs.pb.cc")
set(fs_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/fs.pb.h")
set(fs_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/fs.grpc.pb.cc")
set(fs_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/fs.grpc.pb.h")
add_custom_command(
      OUTPUT "${fs_proto_srcs}" "${fs_proto_hdrs}" "${fs_grpc_srcs}" "${fs_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${fs_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${fs_proto}"
      DEPENDS "${fs_proto}")

add_library(fs_grpc_proto
  ${fs_grpc_srcs}
  ${fs_grpc_hdrs}
  ${fs_proto_srcs}
  ${fs_proto_hdrs})
target_link_libraries(fs_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})

add_executable(client client.cpp)
target_link_libraries(client fs_grpc_proto) 

add_executable(server server.cpp)
target_link_libraries(server fs_grpc_proto)

install(TARGETS client server  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

