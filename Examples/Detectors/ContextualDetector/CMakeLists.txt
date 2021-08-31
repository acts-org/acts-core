add_library(
  ActsExamplesDetectorContextual SHARED
  src/InternallyAlignedDetector.cpp
  src/AlignmentDecorator.cpp
  src/PayloadDecorator.cpp
  src/PayloadDetector.cpp)
target_include_directories(
  ActsExamplesDetectorContextual
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(
  ActsExamplesDetectorContextual
  PUBLIC
    ActsCore ActsPluginIdentification ActsPluginDigitization
    ActsExamplesFramework ActsExamplesDetectorsCommon ActsExamplesDetectorGeneric)

install(
  TARGETS ActsExamplesDetectorContextual
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
