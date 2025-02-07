# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_library(
    ActsExamplesGeant4
    SHARED
    src/Geant4Simulation.cpp
    src/MagneticFieldWrapper.cpp
    src/MaterialPhysicsList.cpp
    src/MaterialSteppingAction.cpp
    src/ParticleTrackingAction.cpp
    src/RegionCreator.cpp
    src/SensitiveSurfaceMapper.cpp
    src/SensitiveSteppingAction.cpp
    src/SimParticleTranslation.cpp
    src/ParticleKillAction.cpp
    src/PhysicsListFactory.cpp
    src/Geant4Manager.cpp
)

target_compile_definitions(ActsExamplesGeant4 PUBLIC ${Geant4_DEFINITIONS})
target_include_directories(
    ActsExamplesGeant4
    SYSTEM
    PUBLIC ${Geant4_INCLUDE_DIRS}
)
target_include_directories(
    ActsExamplesGeant4
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesGeant4
    PUBLIC
        ActsCore
        ActsExamplesFramework
        ActsExamplesDetectorsCommon
        Boost::headers
        ${Geant4_LIBRARIES}
)

install(TARGETS ActsExamplesGeant4 LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
