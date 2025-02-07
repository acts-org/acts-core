# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_library(
    ActsExamplesIoObj
    SHARED
    src/ObjTrackingGeometryWriter.cpp
    src/ObjSimHitWriter.cpp
    src/ObjPropagationStepsWriter.cpp
)

target_include_directories(
    ActsExamplesIoObj
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesIoObj
    PUBLIC ActsCore ActsExamplesFramework Threads::Threads
)

install(TARGETS ActsExamplesIoObj LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
