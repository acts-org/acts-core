# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_library(
    ActsExamplesDetectorGeant4
    SHARED
    src/Geant4Detector.cpp
    src/GdmlDetectorConstruction.cpp
    src/GdmlDetector.cpp
)

target_include_directories(
    ActsExamplesDetectorGeant4
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    ActsExamplesDetectorGeant4
    PUBLIC
        ActsCore
        ActsExamplesFramework
        ActsExamplesGeant4
        ActsPluginGeant4
        ActsExamplesDetectorsCommon
)

install(
    TARGETS ActsExamplesDetectorGeant4
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
