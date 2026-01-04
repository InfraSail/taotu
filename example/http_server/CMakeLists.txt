SET(HTTP_SERVER_SOURCE
  main.cc
  http_parser.cc
  http_response.cc
  http_server.cc
)


FIND_PACKAGE(PkgConfig REQUIRED)
# On Ubuntu, the pkg-config package is named libllhttp
PKG_CHECK_MODULES(LLHTTP REQUIRED libllhttp)

ADD_EXECUTABLE(http_server ${HTTP_SERVER_SOURCE})
TARGET_INCLUDE_DIRECTORIES(http_server PUBLIC ${LLHTTP_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(http_server PUBLIC ${LLHTTP_LIBRARIES} taotu-static)
