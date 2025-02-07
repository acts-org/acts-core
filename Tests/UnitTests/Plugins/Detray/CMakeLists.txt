# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsPluginDetray)
add_unittest(DetrayConverter DetrayConverterTests.cpp)
add_unittest(DetrayGeometryConverter DetrayGeometryConverterTests.cpp)
add_unittest(DetrayMaterialConverter DetrayMaterialConverterTests.cpp)
