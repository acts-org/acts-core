add_library(
    ActsExamplesPrinters
    SHARED
    ActsExamples/Printers/ParticlesPrinter.cpp
    ActsExamples/Printers/TrackParametersPrinter.cpp
)
target_include_directories(
    ActsExamplesPrinters
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
target_link_libraries(
    ActsExamplesPrinters
    PUBLIC ActsCore ActsFatras ActsExamplesFramework
)

install(
    TARGETS ActsExamplesPrinters
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
