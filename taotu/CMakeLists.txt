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
balancer.cc)

ADD_LIBRARY(taotu-static STATIC ${TAOTU_SOURCE})
ADD_LIBRARY(taotu-shared SHARED ${TAOTU_SOURCE})
