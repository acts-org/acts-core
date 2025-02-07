# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

set(unittest_extra_libraries ActsPluginPodio)
add_unittest(PodioTrackContainer PodioTrackContainerTest.cpp)
add_unittest(PodioTrackStateContainer PodioTrackStateContainerTest.cpp)
