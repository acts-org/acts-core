// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/detail/IndexedSurfacesGenerator.hpp"
#include "Acts/Geometry/Extent.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Navigation/InternalNavigation.hpp"
#include "Acts/Navigation/NavigationDelegates.hpp"
#include "Acts/Plugins/Json/AlgebraJsonConverter.hpp"
#include "Acts/Plugins/Json/DetrayJsonHelper.hpp"
#include "Acts/Plugins/Json/GridJsonConverter.hpp"
#include "Acts/Plugins/Json/IndexedGridJsonHelper.hpp"
#include "Acts/Utilities/AxisFwd.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/GridAxisGenerators.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/TypeList.hpp"

#include <climits>
#include <vector>

namespace Acts {

using namespace GridAxisGenerators;

namespace IndexedSurfacesJsonConverter {

/// @brief Convert the single delegate if it is of the type of the reference
///
/// @note It will do nothing if the type does not match
///
/// @param jIndexedSurfaces the json object to be filled
/// @param delegate the delegate to be translated
/// @param detray if the detray json format is written
/// @param refInstance is a reference instance of potential type casting
template <typename instance_type>
void convert(nlohmann::json& jIndexedSurfaces,
             const Experimental::InternalNavigationDelegate& delegate,
             bool detray, [[maybe_unused]] const instance_type& refInstance) {
  using GridType =
      typename instance_type::template grid_type<std::vector<std::size_t>>;
  // Defining a Delegate type
  using DelegateType = Experimental::IndexedSurfacesAllPortalsNavigation<
      GridType, Experimental::IndexedSurfacesNavigation>;
  using SubDelegateType = Experimental::IndexedSurfacesNavigation<GridType>;

  // Get the instance
  const auto* instance = delegate.instance();
  auto castedDelegate = dynamic_cast<const DelegateType*>(instance);
  if (castedDelegate != nullptr) {
    // Get the surface updator
    auto indexedSurfaces = std::get<SubDelegateType>(castedDelegate->updators);
    jIndexedSurfaces = IndexedGridJsonHelper::convertImpl<SubDelegateType>(
        indexedSurfaces, detray, detray);
    if (detray) {
      nlohmann::json jAccLink;
      jAccLink["type"] =
          DetrayJsonHelper::accelerationLink(indexedSurfaces.casts);
      jAccLink["index"] = std::numeric_limits<std::size_t>::max();
      jIndexedSurfaces["grid_link"] = jAccLink;
    }
  }
}

/// @brief Unrolling function for catching the right instance
///
/// @param jIndexedSurfaces the json object to be filled
/// @param delegate the delegate to be translated
/// @param detray if the detray json format is written
template <typename... Args>
void unrollConvert(nlohmann::json& jIndexedSurfaces,
                   const Experimental::InternalNavigationDelegate& delegate,
                   bool detray, TypeList<Args...> /*unused*/) {
  (convert(jIndexedSurfaces, delegate, detray, Args{}), ...);
}

/// Convert a surface updator
///
/// @param delegate the delegate to be translated
/// @param detray if the detray json format is written
///
/// @note this is the entry point of the conversion, i.e. top of the
/// unrolling loop
///
/// @return a json object representing the surface updator
static inline nlohmann::json toJson(
    const Experimental::InternalNavigationDelegate& delegate,
    bool detray = false) {
  // Convert if dynamic cast happens to work
  nlohmann::json jIndexedSurfaces;
  unrollConvert(jIndexedSurfaces, delegate, detray,
                GridAxisGenerators::PossibleAxes{});
  // Return the newly filled ones
  if (!jIndexedSurfaces.is_null()) {
    jIndexedSurfaces["type"] = "IndexedSurfaces";
  }
  return jIndexedSurfaces;
}

/// @brief Convert the single delegate if it is of the type of the reference
///
/// @param jSurfaceNavigation the json file to read from
///
/// @return the surface navigation delegate
Experimental::InternalNavigationDelegate fromJson(
    const nlohmann::json& jSurfaceNavigation);

}  // namespace IndexedSurfacesJsonConverter
}  // namespace Acts
