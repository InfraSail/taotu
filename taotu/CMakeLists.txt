SET(TAOTU_SOURCE
  acceptor.cc
  timer.cc
  connecting.cc
  io_buffer.cc
  event_manager.cc
  eventer.cc
  time_point.cc
  server.cc
  socketer.cc
  poller.cc
  thread_pool.cc
  reactor_manager.cc
  connector.cc
  logger.cc
  client.cc
  net_address.cc
  balancer.cc
)

FIND_PACKAGE(Threads REQUIRED)
ADD_LIBRARY(taotu-static STATIC ${TAOTU_SOURCE})
TARGET_LINK_LIBRARIES(taotu-static ${CMAKE_THREAD_LIBS_INIT})
SET_TARGET_PROPERTIES(taotu-static PROPERTIES OUTPUT_NAME "taotu")
SET_TARGET_PROPERTIES(taotu-static PROPERTIES CLEAN_DIRECT_OUTPUT 1)


TARGET_INCLUDE_DIRECTORIES(taotu-static PUBLIC
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/taotu"
)

ADD_LIBRARY(taotu-shared SHARED ${TAOTU_SOURCE})
TARGET_LINK_LIBRARIES(taotu-shared ${CMAKE_THREAD_LIBS_INIT})
SET_TARGET_PROPERTIES(taotu-shared PROPERTIES OUTPUT_NAME "taotu")
SET_TARGET_PROPERTIES(taotu-shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)


TARGET_INCLUDE_DIRECTORIES(taotu-shared PUBLIC
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_SOURCE_DIR}/taotu"
)
