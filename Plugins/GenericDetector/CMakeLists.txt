add_library(
  ActsPluginGenericDetector SHARED
  src/BuildGenericDetector.cpp
  src/GenericDetector.cpp
  src/GenericDetectorElement.cpp)
target_include_directories(
  ActsPluginGenericDetector
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(
  ActsPluginGenericDetector
  PUBLIC ActsCore)

install(
  TARGETS ActsPluginGenericDetector
  EXPORT ActsPluginGenericDetectorTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(
  DIRECTORY include/Acts
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
