# This file is part of the Acts project.
#
# Copyright (C) 2022 CERN for the benefit of the Acts project
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# CMake include(s).
include(FetchContent)

# Tell the user what's happening.
message(STATUS "Building actsvg as part of the ACTS project")

FetchContent_Declare(actsvg ${ACTS_ACTSVG_SOURCE})

# Now set up its build.
FetchContent_MakeAvailable(actsvg)
