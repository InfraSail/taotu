/**
 * @file sync_client_main.cc
 * @author Sigma711 (sigma711 at foxmail dot com)
 * @brief Main entrance of the sync client of the time service.
 * @date 2023-08-24
 *
 * @copyright Copyright (c) 2023 Sigma711
 *
 */

#include <stdio.h>

#include <memory>

#include "../../../src/logger.h"
#include "../../../src/net_address.h"
#include "../../../src/rpc_channel.h"
#include "time_service.pb.h"

int main() {
  taotu::START_LOG("time_service_sync_client_log.txt");
  {
    taotu::RpcSyncChannel rpc_sync_channel(
        taotu::NetAddress{"127.0.0.1", 4567});
    timeservice::TimeService::Stub stub(&rpc_sync_channel);
    timeservice::TimeRequest request;
    request.set_client_id("1234");
    timeservice::TimeResponse response;
    stub.GetTime(nullptr, &request, &response, nullptr);
    ::printf("TimeService RPC Server time: %s\n",
             response.current_time().c_str());
  }
  taotu::END_LOG();
  return 0;
}
