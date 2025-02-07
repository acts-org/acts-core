# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

target_sources(
    ActsCore
    PRIVATE
        AnnealingUtility.cpp
        AxisDefinitions.cpp
        Logger.cpp
        SpacePointUtility.cpp
        TrackHelpers.cpp
        Intersection.cpp
        IAxis.cpp
        GraphViz.cpp
        ProtoAxis.cpp
)
