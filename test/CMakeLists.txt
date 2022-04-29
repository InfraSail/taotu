ENABLE_TESTING()

INCLUDE(GoogleTest)

FIND_PACKAGE(Threads REQUIRED)
ADD_EXECUTABLE(time_unittest time_unittest.cc)
TARGET_LINK_LIBRARIES(time_unittest PUBLIC gtest gtest_main taotu-static)
GTEST_DISCOVER_TESTS(time_unittest TEST_LIST TimeTest)

ADD_EXECUTABLE(lock_unittest lock_unittest.cc)
TARGET_LINK_LIBRARIES(lock_unittest PUBLIC gtest gtest_main taotu-static)
GTEST_DISCOVER_TESTS(lock_unittest TEST_LIST LockTest)
