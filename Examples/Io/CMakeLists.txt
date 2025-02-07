# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_subdirectory(Csv)
add_subdirectory_if(EDM4hep ACTS_BUILD_EXAMPLES_EDM4HEP)
add_subdirectory_if(HepMC3 ACTS_BUILD_EXAMPLES_HEPMC3)
add_subdirectory(Json)
add_subdirectory(Obj)
add_subdirectory(Root)
add_subdirectory_if(Svg ACTS_BUILD_PLUGIN_ACTSVG)
