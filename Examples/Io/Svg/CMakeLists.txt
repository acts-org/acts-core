# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_library(ActsExamplesIoSvg SHARED src/SvgTrackingGeometryWriter.cpp)
target_include_directories(
    ActsExamplesIoSvg
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(
    ActsExamplesIoSvg
    PUBLIC ActsCore ActsExamplesFramework ActsPluginActSVG Threads::Threads
)

install(TARGETS ActsExamplesIoSvg LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
