SET(SIMPLE_DISCARD_SOURCE
  main.cc
  discard.cc
)

ADD_EXECUTABLE(simple_discard ${SIMPLE_DISCARD_SOURCE} "${PROJECT_BINARY_DIR}/output/bin")
TARGET_LINK_LIBRARIES(simple_discard PUBLIC taotu-static)
