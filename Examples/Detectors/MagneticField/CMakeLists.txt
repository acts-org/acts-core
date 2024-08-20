add_library(
    ActsExamplesMagneticField
    SHARED
    src/FieldMapRootIo.cpp
    src/FieldMapTextIo.cpp
    src/ScalableBFieldService.cpp
)
target_include_directories(
    ActsExamplesMagneticField
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesMagneticField
    # the ROOT libraries should be private, but if we do that then the linker
    # fails with some missing ROOT symbols.
    PUBLIC ActsCore ActsExamplesFramework ROOT::Core ROOT::Tree
    PRIVATE std::filesystem
)

install(
    TARGETS ActsExamplesMagneticField
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
