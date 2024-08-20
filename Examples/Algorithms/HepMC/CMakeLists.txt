add_library(ActsExamplesHepMC3 SHARED src/HepMCProcessExtractor.cpp)
target_include_directories(
    ActsExamplesHepMC3
    SYSTEM
    PUBLIC ${HEPMC3_INCLUDE_DIR}
)
target_include_directories(
    ActsExamplesHepMC3
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesHepMC3
    PUBLIC ActsCore ActsExamplesFramework ActsExamplesIoHepMC3
)

install(TARGETS ActsExamplesHepMC3 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
