# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

# helpers shared between all tests
add_subdirectory(CommonHelpers)

add_subdirectory_if(Benchmarks ACTS_BUILD_BENCHMARKS)
add_subdirectory_if(IntegrationTests ACTS_BUILD_INTEGRATIONTESTS)
add_subdirectory_if(UnitTests ACTS_BUILD_UNITTESTS)
