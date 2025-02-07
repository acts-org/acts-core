# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_library(ActsExamplesFatras SHARED src/FatrasSimulation.cpp)
target_include_directories(
    ActsExamplesFatras
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesFatras
    PUBLIC ActsCore ActsFatras ActsExamplesFramework ActsExamplesMagneticField
)

install(TARGETS ActsExamplesFatras LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
