#include <gtest/gtest.h>

#include <atomic>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "../src/event_manager.h"
#include "../src/logger.h"

namespace {

std::string ReadFile(const std::string& path) {
  std::ifstream in(path);
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

size_t CountSubstring(const std::string& text, const std::string& sub) {
  size_t count = 0;
  size_t pos = 0;
  while (true) {
    pos = text.find(sub, pos);
    if (pos == std::string::npos) {
      break;
    }
    ++count;
    pos += sub.size();
  }
  return count;
}

}  // namespace

TEST(LoggerUnit, WritesExpectedPrefix) {
  const std::string log_path = "logger_unit_test.log";
  taotu::START_LOG(log_path.c_str());
  taotu::LOG_INFO("unit test %d", 1);
  taotu::END_LOG();

  const std::string content = ReadFile(log_path);
  ASSERT_NE(content.find("Log(Info): unit test 1"), std::string::npos);

  std::remove(log_path.c_str());
}

TEST(LoggerRegression, MultiThreadedLoggingDoesNotLoseLogs) {
  const std::string log_path = "logger_regression_test.log";
  taotu::START_LOG(log_path.c_str());

  constexpr int kThreads = 8;
  constexpr int kPerThread = 2000;
  std::atomic<bool> start{false};
  std::vector<std::thread> workers;
  workers.reserve(kThreads);

  for (int t = 0; t < kThreads; ++t) {
    workers.emplace_back([&]() {
      while (!start.load(std::memory_order_acquire)) {
      }
      for (int i = 0; i < kPerThread; ++i) {
        taotu::LOG_INFO("regression %d", i);
      }
    });
  }

  start.store(true, std::memory_order_release);
  for (auto& th : workers) {
    th.join();
  }

  taotu::END_LOG();

  const std::string content = ReadFile(log_path);
  const size_t found = CountSubstring(content, "Log(Info): regression");
  ASSERT_EQ(found, static_cast<size_t>(kThreads * kPerThread));

  std::remove(log_path.c_str());
}

TEST(LoggerIntegration, LogsFromEventManagerThread) {
  const std::string log_path = "logger_integration_test.log";
  taotu::START_LOG(log_path.c_str());

  taotu::EventManager event_manager;
  std::atomic<int> done{0};
  constexpr int kTasks = 1000;

  event_manager.Loop();
  for (int i = 0; i < kTasks; ++i) {
    event_manager.RunSoon([&event_manager, &done, i]() {
      taotu::LOG_INFO("integration %d", i);
      if (done.fetch_add(1) + 1 == kTasks) {
        event_manager.Quit();
      }
    });
  }
  event_manager.Join();

  taotu::END_LOG();

  const std::string content = ReadFile(log_path);
  ASSERT_NE(content.find("Log(Info): integration"), std::string::npos);

  std::remove(log_path.c_str());
}

TEST(LoggerRegression, ConcurrentStartIsSafe) {
  const std::string log_path = "log.txt";
  std::remove(log_path.c_str());

  constexpr int kThreads = 8;
  constexpr int kPerThread = 200;
  std::atomic<bool> start{false};
  std::vector<std::thread> workers;
  workers.reserve(kThreads);

  for (int t = 0; t < kThreads; ++t) {
    workers.emplace_back([&]() {
      while (!start.load(std::memory_order_acquire)) {
      }
      for (int i = 0; i < kPerThread; ++i) {
        taotu::LOG_INFO("init %d", i);
      }
    });
  }

  start.store(true, std::memory_order_release);
  for (auto& th : workers) {
    th.join();
  }

  taotu::END_LOG();

  const std::string content = ReadFile(log_path);
  const size_t found = CountSubstring(content, "Log(Info): init");
  ASSERT_EQ(found, static_cast<size_t>(kThreads * kPerThread));

  std::remove(log_path.c_str());
}
