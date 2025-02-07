# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

target_sources(
    ActsCore
    PRIVATE
        PrintParameters.cpp
        TrackParameters.cpp
        TransformationHelpers.cpp
        CorrectedTransformationFreeToBound.cpp
        TrackStatePropMask.cpp
        VectorMultiTrajectory.cpp
        VectorTrackContainer.cpp
        TrackParameterHelpers.cpp
)
