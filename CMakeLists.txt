CMAKE_MINIMUM_REQUIRED(VERSION 3.14)
PROJECT(taotu CXX)

SET(CMAKE_CXX_STANDARD 17)

SET(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

SET(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fsanitize=address -fsanitize=leak -DTAOTU_DEBUG")

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  message(STATUS "Darwin detected, not using -fsanitize=leak")
else()
  # The above SET(CMAKE_CXX_FLAGS_DEBUG ...) already includes -fsanitize=leak,
  # so this conditional addition is now redundant if the intent is to always include it.
  # If the intent was to *only* include it conditionally, the above line should be adjusted.
  # For now, keeping the original conditional structure but noting the redundancy.
  # set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=leak")
endif()

ADD_DEFINITIONS(-Wno-format-security)

IF(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Debug"))
  MESSAGE(STATUS "CMAKE_CXX_FLAGS_DEBUG = " ${CMAKE_CXX_FLAGS_DEBUG})
ELSEIF(CMAKE_BUILD_TYPE AND (CMAKE_BUILD_TYPE STREQUAL "Release"))
  MESSAGE(STATUS "CMAKE_CXX_FLAGS_RELEASE = " ${CMAKE_CXX_FLAGS_RELEASE})
ENDIF()

FILE(COPY
  "${PROJECT_SOURCE_DIR}/src/"
  DESTINATION
  "${PROJECT_BINARY_DIR}/output/include/taotu/"
  FILES_MATCHING
  PATTERN "*.h"
)

SET(LIBRARY_OUTPUT_PATH  "${PROJECT_BINARY_DIR}/output/lib/")

ADD_SUBDIRECTORY(src)

SET(EXECUTABLE_OUTPUT_PATH  "${PROJECT_BINARY_DIR}/output/bin/")

ADD_SUBDIRECTORY(test)

ADD_SUBDIRECTORY(example)
