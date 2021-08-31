set(_common_libraries
  ActsCore
  ActsExamplesFramework
  ActsExamplesPropagation
  ActsExamplesCommon
  ActsExamplesMagneticField
  ActsExamplesIoObj
  ActsExamplesIoJson
  ActsExamplesIoRoot
  Boost::program_options)

# Empty detector
add_executable(
  ActsExamplePropagationEmpty
  EmptyDetectorPropagationExample.cpp)
target_link_libraries(
  ActsExamplePropagationEmpty
  PRIVATE ${_common_libraries} ActsExamplesDetectorEmpty)

# Generic detector
add_executable(
  ActsExamplePropagationGeneric
  GenericPropagationExample.cpp)
target_link_libraries(
  ActsExamplePropagationGeneric
  PRIVATE ${_common_libraries} ActsExamplesDetectorGeneric)

# Generic detector with alignment
add_executable(
  ActsExamplePropagationAligned
  AlignedPropagationExample.cpp)
target_link_libraries(
  ActsExamplePropagationAligned
  PRIVATE ${_common_libraries} ActsExamplesDetectorContextual)


# TGeo detector
add_executable(
  ActsExamplePropagationTGeo
  TGeoPropagationExample.cpp)
target_link_libraries(
  ActsExamplePropagationTGeo
  PRIVATE ${_common_libraries} ActsExamplesDetectorTGeo)

install(
  TARGETS
    ActsExamplePropagationEmpty
    ActsExamplePropagationGeneric
    ActsExamplePropagationAligned
    ActsExamplePropagationTGeo
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_subdirectory_if(DD4hep ACTS_BUILD_EXAMPLES_DD4HEP)
