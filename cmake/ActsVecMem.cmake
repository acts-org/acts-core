# This file is part of the Acts project.
#
# Copyright (C) 2021 CERN for the benefit of the Acts project
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
# Guard against multiple includes.
if ( TARGET "vecmem::core")
   return()
endif()

# Look for VecMem, quietly first time around. Since if it is not found (which is
# likely), it prints multiple lines of warnings.
find_package( vecmem QUIET )

# If it was found, then we're finished.
if( vecmem_FOUND )
   # Call find_package again, just to nicely print where it is picked up from.
   find_package( vecmem )
   return()
endif()

# CMake include(s).
cmake_minimum_required( VERSION 3.11 )
include( FetchContent )

# Tell the user what's happening.
message( STATUS "Building VecMem as part of the Acts project" )

# Declare where to get VecMem from.
FetchContent_Declare( VecMem
   GIT_REPOSITORY "https://github.com/acts-project/vecmem.git"
   GIT_TAG "9b1a7c3fa6efa63a69e09f3a736d56c9c82fdb45" )

# Prevent VecMem from building its tests. As it would interfere with how Acts
# builds/uses GoogleTest.
set( BUILD_TESTING FALSE )

# Get it into the current directory.
FetchContent_Populate( VecMem )
add_subdirectory( "${vecmem_SOURCE_DIR}" "${vecmem_BINARY_DIR}"
   EXCLUDE_FROM_ALL )
