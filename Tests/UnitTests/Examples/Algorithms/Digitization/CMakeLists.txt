# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsExamplesDigitization ActsExamplesIoJson)

add_unittest(ModuleClusters ModuleClustersTests.cpp)
