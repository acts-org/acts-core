# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_library(
    ActsExamplesDetectorTelescope
    SHARED
    src/TelescopeDetector.cpp
    src/TelescopeDetectorElement.cpp
    src/BuildTelescopeDetector.cpp
)

target_include_directories(
    ActsExamplesDetectorTelescope
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

target_link_libraries(
    ActsExamplesDetectorTelescope
    PUBLIC ActsCore ActsExamplesFramework ActsExamplesDetectorsCommon
)

if(ACTS_BUILD_EXAMPLES_GEANT4)
    target_link_libraries(
        ActsExamplesDetectorTelescope
        PUBLIC ActsExamplesGeant4
    )

    target_sources(
        ActsExamplesDetectorTelescope
        PUBLIC
            src/TelescopeDetectorGeant4.cpp
            src/TelescopeG4DetectorConstruction.cpp
    )
else()
    target_sources(
        ActsExamplesDetectorTelescope
        PUBLIC src/TelescopeDetectorGeant4Stub.cpp
    )
endif()

install(
    TARGETS ActsExamplesDetectorTelescope
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
