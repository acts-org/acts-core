# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

target_sources(
    ActsCore
    PRIVATE
        CombinatorialKalmanFilterError.cpp
        MeasurementSelector.cpp
        GbtsConnector.cpp
        RoiDescriptor.cpp
        AmbiguityTrackClustering.cpp
)
