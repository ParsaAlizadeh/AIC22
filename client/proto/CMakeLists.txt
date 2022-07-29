include(common.cmake)

# Proto file
set(proto_name hide_and_seek)
get_filename_component(my_proto "./v1/${proto_name}.proto" ABSOLUTE)
get_filename_component(my_proto_path "${my_proto}" PATH)

# Generated sources
set(my_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.cc")
set(my_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.pb.h")
set(my_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.cc")
set(my_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/${proto_name}.grpc.pb.h")
add_custom_command(
      OUTPUT "${my_proto_srcs}" "${my_proto_hdrs}" "${my_grpc_srcs}" "${my_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${my_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${my_proto}"
      DEPENDS "${my_proto}")

include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(GRPC_PROTO
  ${my_grpc_srcs}
  ${my_grpc_hdrs}
  ${my_proto_srcs}
  ${my_proto_hdrs})
target_link_libraries(GRPC_PROTO
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF})
