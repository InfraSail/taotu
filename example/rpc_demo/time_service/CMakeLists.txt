FIND_PACKAGE(Protobuf CONFIG REQUIRED)

SET(PROTO_PATH "${CMAKE_SOURCE_DIR}/example/rpc_demo/time_service/")
SET(TIME_PROTO "${PROTO_PATH}/time_service.proto")
SET(GENERATED_PROTOBUF_PATH "${CMAKE_SOURCE_DIR}/example/rpc_demo/time_service/")

SET(TIME_SERVICE_PB_CPP_FILE "${GENERATED_PROTOBUF_PATH}/time_service.pb.cc")
SET(TIME_SERVICE_PB_H_FILE "${GENERATED_PROTOBUF_PATH}/time_service.pb.h")

ADD_CUSTOM_COMMAND(
  OUTPUT ${TIME_SERVICE_PB_H_FILE}
          ${TIME_SERVICE_PB_CPP_FILE}
  COMMAND protoc
  ARGS --proto_path ${PROTO_PATH}
        --cpp_out ${GENERATED_PROTOBUF_PATH}
        ${TIME_PROTO}
  DEPENDS ${TIME_PROTO}
)

SET(TIME_SERVICE_SYNC_CLIENT_SOURCE
  sync_client_main.cc
  ${TIME_SERVICE_PB_CPP_FILE}
)

SET(TIME_SERVICE_SERVER_SOURCE
  server_main.cc
  ${TIME_SERVICE_PB_CPP_FILE}
)

ADD_EXECUTABLE(time_service_sync_client ${TIME_SERVICE_SYNC_CLIENT_SOURCE})
TARGET_LINK_LIBRARIES(time_service_sync_client PUBLIC protobuf::libprotobuf)
TARGET_LINK_LIBRARIES(time_service_sync_client PUBLIC taotu-static)

ADD_EXECUTABLE(time_service_server ${TIME_SERVICE_SERVER_SOURCE})
TARGET_LINK_LIBRARIES(time_service_server PUBLIC protobuf::libprotobuf)
TARGET_LINK_LIBRARIES(time_service_server PUBLIC taotu-static)
