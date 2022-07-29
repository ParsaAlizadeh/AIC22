add_library(Client client.h client.cpp)

target_link_libraries(Client GRPC_PROTO)
target_link_libraries(Client AI)