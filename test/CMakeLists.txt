ENABLE_TESTING()

INCLUDE(GoogleTest)

ADD_EXECUTABLE(time_unittest time_unittest.cc)
TARGET_LINK_LIBRARIES(time_unittest PUBLIC gtest gtest_main taotu-static)

gtest_discover_tests(time_unittest TEST_LIST TimeTest)
