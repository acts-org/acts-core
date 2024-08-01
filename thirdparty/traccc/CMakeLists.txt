# This file is part of the Acts project.
#
# Copyright (C) 2021 CERN for the benefit of the Acts project
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# CMake include(s).
cmake_minimum_required(VERSION 3.14)
include(FetchContent)

# Tell the user what's happening.
message( STATUS "Building tracc as part of the Acts project" )

set( TRACCC_VERSION "${_acts_traccc_version}")

# Declare where to get traccc from.
FetchContent_Declare( traccc ${ACTS_TRACCC_SOURCE} )

set( TRACCC_SETUP_VECMEM OFF CACHE BOOL
   "Do not set up Actsvg as part of Traccc" )
set( TRACCC_SETUP_EIGEN3 OFF CACHE BOOL
   "Do not set up Eigen3 as part of Traccc" )
set( TRACCC_SETUP_THRUST ON CACHE BOOL
    "Do not set up Thrust as part of Traccc" ) 
set( TRACCC_SETUP_ALGEBRA_PLUGINS OFF CACHE BOOL
    "Do not set up Algebra Plugins as part of Traccc" ) 
set( TRACCC_SETUP_COVFIE OFF CACHE BOOL
    "Do not set up Covfie as part of Traccc" ) 
set( TRACCC_SETUP_DFELIBS OFF CACHE BOOL
    "Do not set up dfelibs as part of Traccc" ) 
set( TRACCC_SETUP_DETRAY OFF CACHE BOOL
    "Do not set up Detray as part of Traccc" ) 
set( TRACCC_SETUP_ACTS OFF CACHE BOOL
    "Do not set up ACTS as part of Traccc" ) 
set( TRACCC_SETUP_TBB OFF CACHE BOOL
    "Do not set up TBB as part of Traccc" ) 

set( TRACCC_BUILD_TESTING OFF CACHE BOOL
    "Turn off the build of the Traccc unit tests" ) 
set( TRACCC_BUILD_EXAMPLES OFF CACHE BOOL
    "Turn off the build of the Traccc examples" ) 

# Now set up its build.
FetchContent_MakeAvailable( traccc )