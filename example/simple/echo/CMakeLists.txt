SET(SIMPLE_ECHO_SOURCE
  main.cc
  echo.cc
)

ADD_EXECUTABLE(simple_echo ${SIMPLE_ECHO_SOURCE} "${PROJECT_BINARY_DIR}/output/bin")
TARGET_LINK_LIBRARIES(simple_echo PUBLIC taotu-static)
