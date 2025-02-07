# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsPluginActSVG)
add_unittest(GridSvgConverter GridSvgConverterTests.cpp)
add_unittest(DetectorSvgConverter DetectorSvgConverterTests.cpp)
add_unittest(DetectorVolumeSvgConverter DetectorVolumeSvgConverterTests.cpp)
add_unittest(IndexedSurfacesSvgConverter IndexedSurfacesSvgConverterTests.cpp)
add_unittest(PortalSvgConverter PortalSvgConverterTests.cpp)
add_unittest(SurfaceSvgConverter SurfaceSvgConverterTests.cpp)
add_unittest(LayerSvgConverter LayerSvgConverterTests.cpp)
add_unittest(TrackingGeometrySvgConverter TrackingGeometrySvgConverterTests.cpp)
