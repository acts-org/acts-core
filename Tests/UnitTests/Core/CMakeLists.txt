# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_unittest(Version VersionTests.cpp)

add_subdirectory(Clusterization)
add_subdirectory(Definitions)
add_subdirectory(Detector)
add_subdirectory(EventData)
add_subdirectory(Geometry)
add_subdirectory(MagneticField)
add_subdirectory(Material)
add_subdirectory(Navigation)
add_subdirectory(Propagator)
add_subdirectory(Seeding)
add_subdirectory(SpacePointFormation)
add_subdirectory(Surfaces)
add_subdirectory(TrackFinding)
add_subdirectory(TrackFitting)
add_subdirectory(Utilities)
add_subdirectory(Vertexing)
add_subdirectory(Visualization)
add_subdirectory(AmbiguityResolution)
