cmake_minimum_required(VERSION 2.8.2)

project(googletest-download NONE)

include(ExternalProject)
ExternalProject_Add(googletest
  GIT_REPOSITORY    https://github.com/google/googletest.git
  GIT_TAG           master
  SOURCE_DIR        "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-src"
  BINARY_DIR        "/cygdrive/c/Users/m7mdg/Documents/Studies/Year 3/Semester b/67808 Operating Systems/ex4/cmake-build-debug/tests/googletest-build"
  CONFIGURE_COMMAND ""
  BUILD_COMMAND     ""
  INSTALL_COMMAND   ""
  TEST_COMMAND      ""
)
