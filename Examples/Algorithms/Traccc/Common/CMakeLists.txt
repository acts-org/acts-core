add_library(
  ActsExamplesTracccCommon SHARED
  src/TracccChainAlgorithmBase.cpp
  src/Conversion/CellMapConversion.cpp
  src/Conversion/DigitizationConversion.cpp
  src/Conversion/MeasurementConversion.cpp
  src/Debug/Debug.cpp
)

target_include_directories(
  ActsExamplesTracccCommon
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
  ActsExamplesTracccCommon
  PUBLIC
    ActsPluginTraccc
    ActsExamplesFramework
    ActsExamplesDigitization
)

install(
  TARGETS ActsExamplesTracccCommon
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
