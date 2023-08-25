/**
 * @file server_main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Main entrance of the server of the time service.
 * @date 2023-08-24
 *
 * @copyright Copyright (c) 2023 Sigma711
 *
 */

#include <vector>

#include "../../../src/event_manager.h"
#include "../../../src/logger.h"
#include "../../../src/net_address.h"
#include "../../../src/rpc_server.h"
#include "time_service.pb.h"

class TimeServiceImpl : public timeservice::TimeService {
 public:
  void GetTime(::google::protobuf::RpcController* controller,
               const timeservice::TimeRequest* request,
               timeservice::TimeResponse* response,
               ::google::protobuf::Closure* done) {
    std::time_t now =
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    ::printf("Received request from time service RPC Client: %s\n",
             request->client_id().c_str());
    std::string current_time = std::ctime(&now);
    current_time[current_time.size() - 1] = '\0';
    current_time.resize(current_time.size() - 1);
    response->set_current_time(current_time);
    response->current_time();
  }
};

int main() {
  taotu::START_LOG("time_service_server_log.txt");
  std::vector<taotu::EventManager*> event_managers(4, nullptr);
  for (auto& event_manager : event_managers) {
    event_manager = new taotu::EventManager{};
  }
  taotu::RpcServer rpc_server(&event_managers, taotu::NetAddress{4567});
  TimeServiceImpl time_service_impl;
  rpc_server.RegisterService(&time_service_impl);
  rpc_server.Start();
  for (auto& event_manager : event_managers) {
    delete event_manager;
  }
  taotu::END_LOG();
  return 0;
}
