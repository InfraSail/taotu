CMAKE_MINIMUM_REQUIRED(VERSION 3.14)
PROJECT(taotu CXX)

SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

OPTION(TAOTU_ENABLE_CLANG_FORMAT "Enable clang-format checks." OFF)
OPTION(TAOTU_ENABLE_CLANG_TIDY "Enable clang-tidy checks." OFF)

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

IF(TAOTU_ENABLE_CLANG_TIDY)
  FIND_PROGRAM(CLANG_TIDY_EXE NAMES clang-tidy)
  IF(NOT CLANG_TIDY_EXE)
    MESSAGE(FATAL_ERROR "clang-tidy not found.")
  ENDIF()
  SET(CMAKE_CXX_CLANG_TIDY
      "${CLANG_TIDY_EXE};--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy")
ENDIF()

IF(TAOTU_ENABLE_CLANG_FORMAT)
  FIND_PROGRAM(CLANG_FORMAT_EXE NAMES clang-format)
  IF(NOT CLANG_FORMAT_EXE)
    MESSAGE(FATAL_ERROR "clang-format not found.")
  ENDIF()
  FILE(GLOB_RECURSE TAOTU_CLANG_FORMAT_FILES CONFIGURE_DEPENDS
       "${CMAKE_SOURCE_DIR}/src/*.h"
       "${CMAKE_SOURCE_DIR}/src/*.hpp"
       "${CMAKE_SOURCE_DIR}/src/*.c"
       "${CMAKE_SOURCE_DIR}/src/*.cc"
       "${CMAKE_SOURCE_DIR}/src/*.cpp"
       "${CMAKE_SOURCE_DIR}/test/*.h"
       "${CMAKE_SOURCE_DIR}/test/*.hpp"
       "${CMAKE_SOURCE_DIR}/test/*.c"
       "${CMAKE_SOURCE_DIR}/test/*.cc"
       "${CMAKE_SOURCE_DIR}/test/*.cpp"
       "${CMAKE_SOURCE_DIR}/example/*.h"
       "${CMAKE_SOURCE_DIR}/example/*.hpp"
       "${CMAKE_SOURCE_DIR}/example/*.c"
       "${CMAKE_SOURCE_DIR}/example/*.cc"
       "${CMAKE_SOURCE_DIR}/example/*.cpp")
  ADD_CUSTOM_TARGET(
    clang-format
    COMMAND ${CLANG_FORMAT_EXE} --dry-run --Werror -style=file
            ${TAOTU_CLANG_FORMAT_FILES}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    VERBATIM)
ENDIF()

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
