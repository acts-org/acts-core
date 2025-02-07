# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsFatras)

add_unittest(FatrasBarcode BarcodeTests.cpp)
add_unittest(FatrasHit HitTests.cpp)
add_unittest(FatrasParticle ParticleTests.cpp)
