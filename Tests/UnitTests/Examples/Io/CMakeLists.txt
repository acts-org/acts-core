# SPDX-PackageName: "ACTS"
# SPDX-FileCopyrightText: 2016 CERN
# SPDX-License-Identifier: MPL-2.0

add_subdirectory_if(Json ACTS_BUILD_PLUGIN_JSON)
add_subdirectory(Root)
add_subdirectory(Csv)
