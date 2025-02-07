# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsFatras)

add_unittest(FatrasBetheBloch BetheBlochTests.cpp)
add_unittest(FatrasBetheHeitler BetheHeitlerTests.cpp)
add_unittest(FatrasScattering ScatteringTests.cpp)
add_unittest(FatrasPhotonConversion PhotonConversionTests.cpp)
