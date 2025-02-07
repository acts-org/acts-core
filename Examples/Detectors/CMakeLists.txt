# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_subdirectory(Common)
add_subdirectory(ContextualDetector)
add_subdirectory_if(DD4hepDetector ACTS_BUILD_EXAMPLES_DD4HEP)
add_subdirectory(GenericDetector)
add_subdirectory_if(Geant4Detector ACTS_BUILD_EXAMPLES_GEANT4)
add_subdirectory(MagneticField)
add_subdirectory(TGeoDetector)
add_subdirectory(ITkModuleSplitting)
add_subdirectory(TelescopeDetector)
add_subdirectory_if(MuonSpectrometerMockupDetector ACTS_BUILD_EXAMPLES_GEANT4)
add_subdirectory_if(GeoModelDetector ACTS_BUILD_PLUGIN_GEOMODEL)
