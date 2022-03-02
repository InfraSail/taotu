SET(SIMPLE_DISCARD_SOURCE
  main.cc
  discard.cc
)

ADD_EXECUTABLE(simple_discard ${SIMPLE_DISCARD_SOURCE})
TARGET_LINK_LIBRARIES(simple_discard PUBLIC taotu-static)
