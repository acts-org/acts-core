# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_library(ActsExamplesDetectorsCommon SHARED src/Detector.cpp)
target_include_directories(
    ActsExamplesDetectorsCommon
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesDetectorsCommon
    PUBLIC ActsCore ActsExamplesFramework
)

install(
    TARGETS ActsExamplesDetectorsCommon
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
