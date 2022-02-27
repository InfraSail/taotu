SET(SIMPLE_ECHO_SOURCE
  echo.cc
  main.cc
)

ADD_EXECUTABLE(simple_echo ${SIMPLE_ECHO_SOURCE})
TARGET_LINK_LIBRARIES(simple_echo PUBLIC taotu-static)
