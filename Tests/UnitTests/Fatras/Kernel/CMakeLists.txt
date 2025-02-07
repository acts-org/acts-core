# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsFatras)

add_unittest(FatrasContinuousProcess ContinuousProcessTests.cpp)
add_unittest(FatrasInteractionList InteractionListTests.cpp)
add_unittest(FatrasSimulationActor SimulationActorTests.cpp)
add_unittest(FatrasTupleIndexOf TupleIndexOfTests.cpp)
