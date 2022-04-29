SET(MESSAGE_CODEC_SOURCE
  message_codec.cc
)

SET(CHAT_CLIENT_SOURCE
  client_main.cc
  chat_client.cc
  ${MESSAGE_CODEC_SOURCE}
)

SET(CHAT_SERVER_SOURCE
  server_main.cc
  chat_server.cc
  ${MESSAGE_CODEC_SOURCE}
)

ADD_EXECUTABLE(chat_client ${CHAT_CLIENT_SOURCE})
TARGET_LINK_LIBRARIES(chat_client PUBLIC taotu-static)

ADD_EXECUTABLE(chat_server ${CHAT_SERVER_SOURCE})
TARGET_LINK_LIBRARIES(chat_server PUBLIC taotu-static)
