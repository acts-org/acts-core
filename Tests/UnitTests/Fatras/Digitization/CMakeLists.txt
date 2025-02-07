# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsFatras)

add_unittest(FatrasChannelMerger ChannelMergerTests.cpp)
add_unittest(FatrasChannelizer ChannelizerTests.cpp)
add_unittest(FatrasPlanarSurfaceDrift PlanarSurfaceDriftTests.cpp)
add_unittest(FatrasPlanarSurfaceMask PlanarSurfaceMaskTests.cpp)
add_unittest(FatrasUncorrelatedHitSmearer UncorrelatedHitSmearerTests.cpp)
add_unittest(FatrasSegmentizer SegmentizerTests.cpp)
