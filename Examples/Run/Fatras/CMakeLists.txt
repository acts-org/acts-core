# Common library that implements the core functionality
add_subdirectory(Common)

# Generic detector
add_executable(
  ActsExampleFatrasGeneric
  GenericFatrasExample.cpp)
target_link_libraries(
  ActsExampleFatrasGeneric
  PRIVATE ActsExamplesFatrasCommon ActsExamplesDetectorGeneric ActsExamplesIoJson)

# Generic detector with IOV based alignment
add_executable(
  ActsExampleFatrasAligned
  AlignedFatrasExample.cpp)
target_link_libraries(
  ActsExampleFatrasAligned
  PRIVATE ActsExamplesFatrasCommon ActsExamplesDetectorContextual ActsExamplesIoJson)

# Generic detector with Payload based alignment
add_executable(
  ActsExampleFatrasPayload
  PayloadFatrasExample.cpp)
target_link_libraries(
  ActsExampleFatrasPayload
  PRIVATE ActsExamplesFatrasCommon ActsExamplesDetectorContextual ActsExamplesIoJson)

# TGeo detector
add_executable(
  ActsExampleFatrasTGeo
  TGeoFatrasExample.cpp)
target_link_libraries(
  ActsExampleFatrasTGeo
  PRIVATE ActsExamplesFatrasCommon ActsExamplesDetectorTGeo ActsExamplesIoJson)

# Telescope detector
add_executable(
  ActsExampleFatrasTelescope
  TelescopeFatrasExample.cpp)
target_link_libraries(
  ActsExampleFatrasTelescope
  PRIVATE ActsExamplesFatrasCommon ActsExamplesDetectorTelescope ActsExamplesIoJson)

install(
  TARGETS
    ActsExampleFatrasGeneric
    ActsExampleFatrasAligned
    ActsExampleFatrasPayload
    ActsExampleFatrasTGeo
    ActsExampleFatrasTelescope
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory_if(DD4hep ACTS_BUILD_EXAMPLES_DD4HEP)
