add_library(
    ActsFatrasGeant4
    SHARED
    src/Geant4Decay.cpp
    src/PDGtoG4Converter.cpp
    src/DummyDetectorConstruction.cpp
)
target_compile_definitions(ActsFatrasGeant4 PUBLIC ${Geant4_DEFINITIONS})
target_include_directories(
    ActsFatrasGeant4
    SYSTEM
    PUBLIC ${Geant4_INCLUDE_DIRS}
)
target_include_directories(
    ActsFatrasGeant4
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(
    ActsFatrasGeant4
    PUBLIC ActsCore ActsFatras ${Geant4_LIBRARIES}
)

install(
    TARGETS ActsFatrasGeant4
    EXPORT ActsFatrasGeant4Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ActsFatras DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
