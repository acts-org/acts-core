// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/Utilities/GroupBy.hpp"
#include "ActsExamples/Utilities/Range.hpp"

#include <algorithm>
#include <cassert>
#include <utility>

#include <boost/bimap.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>

namespace ActsExamples {
namespace detail {

// extract the geometry identifier from a variety of types
struct GeometryIdGetter {
  // explicit geometry identifier are just forwarded
  constexpr Acts::GeometryIdentifier operator()(
      Acts::GeometryIdentifier geometryId) const {
    return geometryId;
  }
  // encoded geometry ids are converted back to geometry identifiers.
  constexpr Acts::GeometryIdentifier operator()(
      Acts::GeometryIdentifier::Value encoded) const {
    return Acts::GeometryIdentifier(encoded);
  }
  // support elements in map-like structures.
  template <typename T>
  constexpr Acts::GeometryIdentifier operator()(
      const std::pair<Acts::GeometryIdentifier, T>& mapItem) const {
    return mapItem.first;
  }
  // support elements that implement `.geometryId()`.
  template <typename T>
  inline auto operator()(const T& thing) const
      -> decltype(thing.geometryId(), Acts::GeometryIdentifier()) {
    return thing.geometryId();
  }
  // support reference_wrappers around such types as well
  template <typename T>
  inline auto operator()(std::reference_wrapper<T> thing) const
      -> decltype(thing.get().geometryId(), Acts::GeometryIdentifier()) {
    return thing.get().geometryId();
  }
};

struct CompareGeometryId {
  // indicate that comparisons between keys and full objects are allowed.
  using is_transparent = void;
  // compare two elements using the automatic key extraction.
  template <typename Left, typename Right>
  constexpr bool operator()(Left&& lhs, Right&& rhs) const {
    return GeometryIdGetter()(lhs) < GeometryIdGetter()(rhs);
  }
};

}  // namespace detail

/// Store elements that know their detector geometry id, e.g. simulation hits.
///
/// @tparam T type to be stored, must be compatible with `CompareGeometryId`
///
/// The container stores an arbitrary number of elements for any geometry
/// id. Elements can be retrieved via the geometry id; elements can be selected
/// for a specific geometry id or for a larger range, e.g. a volume or a layer
/// within the geometry hierarchy using the helper functions below. Elements can
/// also be accessed by index that uniquely identifies each element regardless
/// of geometry id.
template <typename T>
using GeometryIdMultiset =
    boost::container::flat_multiset<T, detail::CompareGeometryId>;

/// Store elements indexed by an geometry id.
///
/// @tparam T type to be stored
///
/// The behaviour is the same as for the `GeometryIdMultiset` except that the
/// stored elements do not know their geometry id themself. When iterating
/// the iterator elements behave as for the `std::map`, i.e.
///
///     for (const auto& entry: elements) {
///         auto id = entry.first; // geometry id
///         const auto& el = entry.second; // stored element
///     }
///
template <typename T>
using GeometryIdMultimap =
    GeometryIdMultiset<std::pair<Acts::GeometryIdentifier, T>>;

/// Select all elements within the given volume.
template <typename T>
inline Range<typename GeometryIdMultiset<T>::const_iterator> selectVolume(
    const GeometryIdMultiset<T>& container,
    Acts::GeometryIdentifier::Value volume) {
  auto cmp = Acts::GeometryIdentifier().withVolume(volume);
  auto beg = std::lower_bound(container.begin(), container.end(), cmp,
                              detail::CompareGeometryId{});
  // WARNING overflows to volume==0 if the input volume is the last one
  cmp = Acts::GeometryIdentifier().withVolume(volume + 1u);
  // optimize search by using the lower bound as start point. also handles
  // volume overflows since the geo id would be located before the start of
  // the upper edge search window.
  auto end =
      std::lower_bound(beg, container.end(), cmp, detail::CompareGeometryId{});
  return makeRange(beg, end);
}

/// Select all elements within the given volume.
template <typename T>
inline auto selectVolume(const GeometryIdMultiset<T>& container,
                         Acts::GeometryIdentifier id) {
  return selectVolume(container, id.volume());
}

/// Select all elements within the given layer.
template <typename T>
inline Range<typename GeometryIdMultiset<T>::const_iterator> selectLayer(
    const GeometryIdMultiset<T>& container,
    Acts::GeometryIdentifier::Value volume,
    Acts::GeometryIdentifier::Value layer) {
  auto cmp = Acts::GeometryIdentifier().withVolume(volume).withLayer(layer);
  auto beg = std::lower_bound(container.begin(), container.end(), cmp,
                              detail::CompareGeometryId{});
  // WARNING resets to layer==0 if the input layer is the last one
  cmp = Acts::GeometryIdentifier().withVolume(volume).withLayer(layer + 1u);
  // optimize search by using the lower bound as start point. also handles
  // volume overflows since the geo id would be located before the start of
  // the upper edge search window.
  auto end =
      std::lower_bound(beg, container.end(), cmp, detail::CompareGeometryId{});
  return makeRange(beg, end);
}

// Select all elements within the given layer.
template <typename T>
inline auto selectLayer(const GeometryIdMultiset<T>& container,
                        Acts::GeometryIdentifier id) {
  return selectLayer(container, id.volume(), id.layer());
}

/// Select all elements for the given module / sensitive surface.
template <typename T>
inline Range<typename GeometryIdMultiset<T>::const_iterator> selectModule(
    const GeometryIdMultiset<T>& container, Acts::GeometryIdentifier geoId) {
  // module is the lowest level and defines a single geometry id value
  return makeRange(container.equal_range(geoId));
}

/// Select all elements for the given module / sensitive surface.
template <typename T>
inline auto selectModule(const GeometryIdMultiset<T>& container,
                         Acts::GeometryIdentifier::Value volume,
                         Acts::GeometryIdentifier::Value layer,
                         Acts::GeometryIdentifier::Value sensitive) {
  return selectModule(container, Acts::GeometryIdentifier()
                                     .withVolume(volume)
                                     .withLayer(layer)
                                     .withSensitive(sensitive));
}

/// Select all elements for the lowest non-zero identifier component.
///
/// Zero values of lower components are interpreted as wildcard search patterns
/// that select all element at the given geometry hierarchy and below. This only
/// applies to the lower components and not to intermediate zeros.
///
/// Examples:
/// - volume=2,layer=0,sensitive=3 -> select all elements in the sensitive
/// - volume=1,layer=2,sensitive=0 -> select all elements in the layer
/// - volume=3,layer=0,sensitive=0 -> select all elements in the volume
///
/// @note An identifier with all components set to zero selects the whole input
///   container.
/// @note Boundary and approach surfaces do not really fit into the geometry
///   hierarchy and must be set to zero for the selection. If they are set on an
///   input identifier, the behaviour of this search method is undefined.
template <typename T>
inline Range<typename GeometryIdMultiset<T>::const_iterator>
selectLowestNonZeroGeometryObject(const GeometryIdMultiset<T>& container,
                                  Acts::GeometryIdentifier geoId) {
  assert((geoId.boundary() == 0u) && "Boundary component must be zero");
  assert((geoId.approach() == 0u) && "Approach component must be zero");

  if (geoId.sensitive() != 0u) {
    return selectModule(container, geoId);
  } else if (geoId.layer() != 0u) {
    return selectLayer(container, geoId);
  } else if (geoId.volume() != 0u) {
    return selectVolume(container, geoId);
  } else {
    return makeRange(container.begin(), container.end());
  }
}

/// Iterate over groups of elements belonging to each module/ sensitive surface.
template <typename T>
inline GroupBy<typename GeometryIdMultiset<T>::const_iterator,
               detail::GeometryIdGetter>
groupByModule(const GeometryIdMultiset<T>& container) {
  return makeGroupBy(container, detail::GeometryIdGetter());
}

/// The accessor for the GeometryIdMultiset container
///
/// It wraps up a few lookup methods to be used in the Combinatorial Kalman
/// Filter
template <typename T>
struct GeometryIdMultisetAccessor {
  using Container = GeometryIdMultiset<T>;
  using Key = Acts::GeometryIdentifier;
  using Value = typename GeometryIdMultiset<T>::value_type;
  using Iterator = typename GeometryIdMultiset<T>::const_iterator;

  // pointer to the container
  const Container* container = nullptr;
};

/// A map that allows mapping back and forth between ACTS and Athena Geometry
/// Ids
using GeometryIdMapActsAthena =
    boost::bimap<std::uint64_t, Acts::GeometryIdentifier>;

}  // namespace ActsExamples
