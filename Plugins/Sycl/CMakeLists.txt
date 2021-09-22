# This file is part of the Acts project.
#
# Copyright (C) 2020-2021 CERN for the benefit of the Acts project
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

# Set up the usage of VecMem.
include(ActsVecMem)

add_library(
  ActsPluginSycl SHARED
  # header files
  include/Acts/Plugins/Sycl/Seeding/detail/Types.hpp
  include/Acts/Plugins/Sycl/Seeding/CreateSeedsForGroupSycl.hpp
  include/Acts/Plugins/Sycl/Seeding/DeviceExperimentCuts.hpp
  include/Acts/Plugins/Sycl/Seeding/Seedfinder.hpp
  include/Acts/Plugins/Sycl/Seeding/Seedfinder.ipp
  include/Acts/Plugins/Sycl/Utilities/CalculateNdRange.hpp
  include/Acts/Plugins/Sycl/Utilities/DeviceSelector.hpp
  include/Acts/Plugins/Sycl/Utilities/ListPlatforms.hpp
  include/Acts/Plugins/Sycl/Utilities/QueueWrapper.hpp
  # implementation files
  src/Seeding/CreateSeedsForGroupSycl.cpp
  src/Seeding/DupletSearch.hpp
  src/Seeding/LinearTransform.hpp
  src/Seeding/SpacePointType.hpp
  src/Utilities/CalculateNdRange.cpp
  src/Utilities/DeviceSelector.cpp
  src/Utilities/ListPlatforms.cpp
  src/Utilities/QueueWrapper.cpp
)

target_include_directories(
  ActsPluginSycl
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(
  ActsPluginSycl
  PUBLIC ActsCore vecmem::core
  PRIVATE vecmem::sycl)

acts_target_setup_sycl(ActsPluginSycl DEPENDENCY PRIVATE)

install(
  TARGETS ActsPluginSycl
  EXPORT ActsPluginSyclTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(
  DIRECTORY include/Acts
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
