# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsPluginGeant4)

add_unittest(Geant4Converters Geant4ConvertersTests.cpp)
add_unittest(Geant4DetectorElement Geant4DetectorElementTests.cpp)
add_unittest(Geant4PhysicalVolumeSelectors Geant4PhysicalVolumeSelectorsTests.cpp)
add_unittest(Geant4DetectorSurfaceFactory Geant4DetectorSurfaceFactoryTests.cpp)
add_unittest(Geant4SurfaceProvider Geant4SurfaceProviderTests.cpp)
