SET(MESSAGE_CODEC
  message_codec.cc
)

SET(CHAT_CLIENT
  client_main.cc
  chat_client.cc
  ${MESSAGE_CODEC}
)

SET(CHAT_SERVER
  server_main.cc
  chat_server.cc
  ${MESSAGE_CODEC}
)

ADD_EXECUTABLE(chat_client ${CHAT_CLIENT})
TARGET_LINK_LIBRARIES(chat_client PUBLIC taotu-static)

ADD_EXECUTABLE(chat_server ${CHAT_SERVER})
TARGET_LINK_LIBRARIES(chat_server PUBLIC taotu-static)
