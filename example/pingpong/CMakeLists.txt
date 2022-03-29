SET(PINGPONG_SERVER
  server_main.cc
  pingpong_server.cc
)

ADD_EXECUTABLE(pingpong_server ${PINGPONG_SERVER})
TARGET_LINK_LIBRARIES(pingpong_server PUBLIC taotu-static)
