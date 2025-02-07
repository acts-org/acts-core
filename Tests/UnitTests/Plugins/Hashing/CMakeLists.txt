# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsPluginHashing)

add_unittest(AnnoyTest AnnoyTest.cpp)
add_unittest(HashingSeedingTest HashingSeedingTest.cpp)
