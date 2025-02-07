# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(integrationtest_extra_libraries ActsFatras)

add_integrationtest(FatrasSimulation FatrasSimulationTests.cpp)
