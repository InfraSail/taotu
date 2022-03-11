SET(SIMPLE_TIME_SOURCE
  main.cc
  time.cc
)

ADD_EXECUTABLE(simple_time ${SIMPLE_TIME_SOURCE})
TARGET_LINK_LIBRARIES(simple_time PUBLIC taotu-static)
