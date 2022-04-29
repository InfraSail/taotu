SET(PINGPONG_CLIENT_SOURCE
  client_main.cc
  pingpong_client.cc
)

SET(PINGPONG_SERVER_SOURCE
  server_main.cc
  pingpong_server.cc
)

ADD_EXECUTABLE(pingpong_client ${PINGPONG_CLIENT_SOURCE})
TARGET_LINK_LIBRARIES(pingpong_client PUBLIC taotu-static)

ADD_EXECUTABLE(pingpong_server ${PINGPONG_SERVER_SOURCE})
TARGET_LINK_LIBRARIES(pingpong_server PUBLIC taotu-static)
