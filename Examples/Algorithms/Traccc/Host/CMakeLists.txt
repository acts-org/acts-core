add_library(
  ActsExamplesTracccHost SHARED
  src/TracccChainAlgorithm.cpp
)

target_include_directories(
  ActsExamplesTracccHost
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
  ActsExamplesTracccHost
  PUBLIC
    ActsExamplesTracccCommon
)

install(
  TARGETS ActsExamplesTracccHost
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
