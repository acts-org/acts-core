add_library(
  ActsExamplesTracccCuda SHARED
  src/TracccChainAlgorithm.cpp
)

target_include_directories(
  ActsExamplesTracccCuda
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
  ActsExamplesTracccCuda
  PUBLIC
    ActsExamplesTracccCommon
)

install(
  TARGETS ActsExamplesTracccCuda
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
