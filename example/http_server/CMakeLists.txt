SET(HTTP_SERVER_SOURCE
  main.cc
  http_parser.cc
  http_response.cc
  http_server.cc
)


ADD_EXECUTABLE(http_server ${HTTP_SERVER_SOURCE})
TARGET_LINK_LIBRARIES(http_server PUBLIC llhttp taotu-static)
