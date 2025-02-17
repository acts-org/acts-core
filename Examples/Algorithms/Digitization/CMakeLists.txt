add_library(
    ActsExamplesDigitization
    SHARED
    src/DigitizationAlgorithm.cpp
    src/DigitizationConfig.cpp
    src/DigitizationCoordinatesConverter.cpp
    src/MeasurementCreation.cpp
    src/DigitizationConfigurator.cpp
    src/ModuleClusters.cpp
)
target_include_directories(
    ActsExamplesDigitization
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesDigitization
    PUBLIC ActsCore ActsExamplesFramework
)

acts_compile_headers(ActsExamplesDigitization GLOB "include/**/*.hpp")

install(
    TARGETS ActsExamplesDigitization
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
