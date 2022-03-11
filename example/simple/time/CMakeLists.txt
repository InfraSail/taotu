SET(SIMPLE_TIME_SOURCE
  main.cc
  time.cc
)

ADD_EXECUTABLE(simple_time ${SIMPLE_TIME_SOURCE} "${PROJECT_BINARY_DIR}/output/bin")
TARGET_LINK_LIBRARIES(simple_time PUBLIC taotu-static)
