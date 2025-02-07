# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsPluginExaTrkX ${TORCH_LIBRARIES})

add_unittest(ExaTrkXTensorConversion ExaTrkXTensorConversionTests.cpp)
add_unittest(ExaTrkXEdgeBuilding ExaTrkXEdgeBuildingTest.cpp)
add_unittest(ExaTrkXBoostTrackBuilding ExaTrkXBoostTrackBuildingTests.cpp)
add_unittest(ExaTrkXMetricHookTests ExaTrkXMetricHookTests.cpp)
if(ACTS_EXATRKX_ENABLE_CUDA)
    add_unittest(ConnectedComponentsCuda ConnectedComponentCudaTests.cu)
endif()
