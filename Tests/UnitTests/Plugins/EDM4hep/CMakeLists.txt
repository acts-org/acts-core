# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_unittest(ConvertTrackEDM4hep ConvertTrackEDM4hepTest.cpp)

target_link_libraries(ActsUnitTestConvertTrackEDM4hep PUBLIC ActsPluginEDM4hep)
