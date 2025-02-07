# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

target_sources(
    ActsCore
    PRIVATE
        NavigationStream.cpp
        TryAllNavigationPolicy.cpp
        SurfaceArrayNavigationPolicy.cpp
)
