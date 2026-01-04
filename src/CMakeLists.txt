# Prefer an installation that provides ProtobufConfig.cmake
# If it’s not found, fall back to module mode (compatible with Ubuntu apt packages)
FIND_PACKAGE(Protobuf CONFIG QUIET)
IF(NOT Protobuf_FOUND)
  FIND_PACKAGE(Protobuf REQUIRED)
ENDIF()

FIND_PACKAGE(PkgConfig REQUIRED)
PKG_CHECK_MODULES(LIBURING REQUIRED liburing)
INCLUDE_DIRECTORIES(${LIBURING_INCLUDE_DIRS})

SET(PROTO_PATH "${CMAKE_SOURCE_DIR}/src/")
SET(TIME_PROTO "${PROTO_PATH}/rpc.proto")
SET(GENERATED_PROTOBUF_PATH "${CMAKE_SOURCE_DIR}/src/")

SET(RPC_PB_CPP_FILE "${GENERATED_PROTOBUF_PATH}/rpc.pb.cc")
SET(RPC_PB_H_FILE "${GENERATED_PROTOBUF_PATH}/rpc.pb.h")

ADD_CUSTOM_COMMAND(
  OUTPUT ${RPC_PB_H_FILE}
          ${RPC_PB_CPP_FILE}
  COMMAND protoc
  ARGS --proto_path ${PROTO_PATH}
        --cpp_out ${GENERATED_PROTOBUF_PATH}
        ${TIME_PROTO}
  DEPENDS ${TIME_PROTO}
)

SET_SOURCE_FILES_PROPERTIES(${RPC_PB_CPP_FILE} PROPERTIES SKIP_LINTING ON)

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

SET_SOURCE_FILES_PROPERTIES(
  rpc_codec.cc
  rpc_channel.cc
  rpc_server.cc
  PROPERTIES SKIP_LINTING ON)

FIND_PACKAGE(Threads REQUIRED)

FIND_PACKAGE(ZLIB REQUIRED)

ADD_LIBRARY(taotu-static STATIC ${TAOTU_SOURCE})
TARGET_LINK_LIBRARIES(taotu-static ${CMAKE_THREAD_LIBS_INIT})
TARGET_LINK_LIBRARIES(taotu-static PUBLIC protobuf::libprotobuf)
TARGET_LINK_LIBRARIES(taotu-static PUBLIC ZLIB::ZLIB)
TARGET_LINK_LIBRARIES(taotu-static PUBLIC ${LIBURING_LIBRARIES})
SET_TARGET_PROPERTIES(taotu-static PROPERTIES OUTPUT_NAME "taotu")
SET_TARGET_PROPERTIES(taotu-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)

# ADD_LIBRARY(taotu-shared SHARED ${TAOTU_SOURCE})
# TARGET_LINK_LIBRARIES(taotu-shared ${CMAKE_THREAD_LIBS_INIT})
# TARGET_LINK_LIBRARIES(taotu-shared PUBLIC protobuf::libprotobuf)
# TARGET_LINK_LIBRARIES(taotu-shared PUBLIC ZLIB::ZLIB)
# SET_TARGET_PROPERTIES(taotu-shared PROPERTIES OUTPUT_NAME "taotu")
# SET_TARGET_PROPERTIES(taotu-shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
