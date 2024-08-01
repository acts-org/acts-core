add_library(
  ActsPluginDetray SHARED
  src/DetrayConverter.cpp)

target_include_directories(
  ActsPluginDetray
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
    $<INSTALL_INTERFACE:include>)
    
target_link_libraries(
  ActsPluginDetray 
  PUBLIC 
    ActsCore
    detray::core
    detray::core_array
    detray::io
    detray::utils
    vecmem::core)

install(
  TARGETS ActsPluginDetray
  EXPORT ActsPluginDetrayTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  
install(
  DIRECTORY include/Acts
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
