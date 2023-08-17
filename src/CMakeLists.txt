FIND_PACKAGE(Protobuf REQUIRED)

SET(PROTO_PATH "${CMAKE_SOURCE_DIR}/src/")
SET(TIME_PROTO "${PROTO_PATH}/rpc.proto")
SET(GENERATED_PROTOBUF_PATH "${CMAKE_SOURCE_DIR}/src/")

SET(RPC_PB_CPP_FILE "${GENERATED_PROTOBUF_PATH}/rpc.pb.cc")
SET(RPC_PB_H_FILE "${GENERATED_PROTOBUF_PATH}/rpc.pb.h")

ADD_CUSTOM_COMMAND(
  OUTPUT ${RPC_PB_H_FILE}
          ${RPC_PB_CPP_FILE}
  COMMAND protobuf::protoc
  ARGS --proto_path ${PROTO_PATH}
        --cpp_out ${GENERATED_PROTOBUF_PATH}
        ${TIME_PROTO}
  DEPENDS ${TIME_PROTO}
)

SET(TAOTU_SOURCE
  acceptor.cc
  timer.cc
  connecting.cc
  io_buffer.cc
  event_manager.cc
  eventer.cc
  time_point.cc
  server.cc
  socketer.cc
  poller.cc
  thread_pool.cc
  reactor_manager.cc
  connector.cc
  logger.cc
  client.cc
  net_address.cc
  balancer.cc
  ${RPC_PB_CPP_FILE}
  rpc_codec.cc
  rpc_channel.cc
  rpc_server.cc
)

FIND_PACKAGE(Threads REQUIRED)
ADD_LIBRARY(taotu-static STATIC ${TAOTU_SOURCE})
TARGET_LINK_LIBRARIES(taotu-static ${CMAKE_THREAD_LIBS_INIT})
SET_TARGET_PROPERTIES(taotu-static PROPERTIES OUTPUT_NAME "taotu")
SET_TARGET_PROPERTIES(taotu-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

ADD_LIBRARY(taotu-shared SHARED ${TAOTU_SOURCE})
TARGET_LINK_LIBRARIES(taotu-shared ${CMAKE_THREAD_LIBS_INIT})
SET_TARGET_PROPERTIES(taotu-shared PROPERTIES OUTPUT_NAME "taotu")
SET_TARGET_PROPERTIES(taotu-shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
