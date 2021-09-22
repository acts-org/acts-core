add_library(
  ActsExamplesDetectorTGeo SHARED
  src/TGeoDetector.cpp
  src/TGeoITkModuleSplitter.cpp)
  
target_include_directories(
  ActsExamplesDetectorTGeo
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(
  ActsExamplesDetectorTGeo
  PUBLIC
    ActsCore ActsPluginIdentification ActsPluginDigitization ActsPluginTGeo ActsPluginJson
    ActsExamplesFramework ActsExamplesDetectorsCommon
    ActsExamplesDetectorGeneric)

install(
  TARGETS ActsExamplesDetectorTGeo
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
