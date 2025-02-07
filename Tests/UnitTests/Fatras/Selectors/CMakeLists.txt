# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsFatras)

add_unittest(FatrasKinematicCasts KinematicCastsTests.cpp)
add_unittest(FatrasParticleSelectors ParticleSelectorsTests.cpp)
add_unittest(FatrasSelectorHelpers SelectorHelpersTests.cpp)
