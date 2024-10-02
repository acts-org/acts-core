// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Detector/detail/ReferenceGenerators.hpp"
#include "Acts/Detector/interface/ISurfacesProvider.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/KDTree.hpp"

#include <array>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace Acts::Experimental {

/// @brief A wrapper class around a KDTree of surfaces
///
/// It also deals with the conversion from global query to
/// KDTree lookup positions
///
template <std::size_t kDIM = 2u, std::size_t bSize = 100u,
          typename reference_generator =
              detail::PolyhedronReferenceGenerator<1u, false>>
class KdtSurfaces {
 public:
  /// Broadcast the surface KDT type
  using KDTS =
      KDTree<kDIM, std::shared_ptr<Surface>, ActsScalar, std::array, bSize>;

  /// Broadcast the query definition
  using Query = std::array<ActsScalar, kDIM>;

  /// Broadcast the entry
  using Entry = std::pair<Query, std::shared_ptr<Surface>>;

  /// Constructor from a vector of surfaces
  ///
  /// @param gctx the geometry context of this call
  /// @param surfaces the surfaces to be filled into the tree
  /// @param casts the cast list from global position into kdtree local
  /// @param rgen the reference point generator
  KdtSurfaces(const GeometryContext& gctx,
              const std::vector<std::shared_ptr<Surface>>& surfaces,
              const std::array<BinningValue, kDIM>& casts,
              const reference_generator& rgen =
                  detail::PolyhedronReferenceGenerator<1u, false>{})
      : m_kdt(nullptr), m_casts(casts), m_rGenerator(rgen) {
    // Simple check if the dimension is correct
    if (kDIM == 0u) {
      throw std::invalid_argument(
          "KdtSurfaces: dimension and/or cast rules are incorrect.");
    }
    // Fill the tree from surfaces
    std::vector<Entry> kdtEntries;
    kdtEntries.reserve(surfaces.size());
    for (auto& s : surfaces) {
      // Generate the references and the center of gravity from it
      const auto references = m_rGenerator.references(gctx, *s);
      std::vector<Query> castedReferences;
      castedReferences.reserve(references.size());
      for (const auto& r : references) {
        //  Now cast into the correct fill position
        Query rc = {};
        fillCasts(r, rc, std::make_integer_sequence<std::size_t, kDIM>{});
        castedReferences.push_back(rc);
      }
      // Calculate the center of gravity in casted frame
      kdtEntries.push_back({cog(castedReferences), s});
    }
    // Create the KDTree
    m_kdt = std::make_unique<KDTS>(std::move(kdtEntries));
  }

  /// Query with a Range object
  ///
  /// @param range is the range to be queried
  ///
  /// @return the matching surfaces from the KDT structure
  std::vector<std::shared_ptr<Surface>> surfaces(
      const RangeXD<kDIM, ActsScalar>& range) const {
    // Strip the surfaces
    std::vector<std::shared_ptr<Surface>> surfacePtrs;
    auto surfaceQuery = m_kdt->rangeSearchWithKey(range);
    std::for_each(surfaceQuery.begin(), surfaceQuery.end(),
                  [&](auto& surf) { surfacePtrs.push_back(surf.second); });
    return surfacePtrs;
  }

  /// Query with an Extent object
  ///
  /// @param extent is the range Extent to be queried
  ///
  /// @return the matching surfaces fpulled from the KDT structure
  std::vector<std::shared_ptr<Surface>> surfaces(const Extent& extent) const {
    RangeXD<kDIM, ActsScalar> qRange;
    for (auto [ibv, v] : enumerate(m_casts)) {
      qRange[ibv] = extent.range(v);
    }
    return surfaces(qRange);
  }

 private:
  /// The KDTree as single source for the surfaces (maybe shared)
  std::unique_ptr<KDTS> m_kdt = nullptr;

  /// Cast values that turn a global position to lookup position
  std::array<BinningValue, kDIM> m_casts = {};

  /// Helper to generate reference points for filling
  reference_generator m_rGenerator;

  /// Unroll the cast loop
  /// @param position is the position of the update call
  /// @param a is the array to be filled
  template <typename Array, std::size_t... idx>
  void fillCasts(const Vector3& position, Array& a,
                 std::index_sequence<idx...> /*indices*/) const {
    ((a[idx] = VectorHelpers::cast(position, m_casts[idx])), ...);
  }

  /// Helper method to calculate the center of gravity in the
  /// casted frame (i.e. query frame)
  ///
  /// @param cQueries are the casted query positions
  /// @note will do nothing if vector size is equal to 1
  ///
  /// @note no checking on qQueries.empty() is done as the
  /// positions are to be provided by a generator which itself
  /// is tested for consistency
  ///
  /// @return the center of gravity as a query object
  Query cog(const std::vector<Query>& cQueries) const {
    // If there is only one position, return it
    if (cQueries.size() == 1) {
      return cQueries.front();
    }
    // Build the center of gravity of the n positions
    Query c{};
    float weight = 1. / cQueries.size();
    for (auto& q : cQueries) {
      std::transform(c.begin(), c.end(), q.begin(), c.begin(),
                     std::plus<ActsScalar>());
    }
    std::for_each(c.begin(), c.end(), [&](auto& v) { v *= weight; });
    return c;
  }
};

/// @brief Callable struct wrapper around the KDT surface structure
///
/// This allows to create small region based callable structs at
/// configuration level that are then connected to an InternalStructureBuilder
template <std::size_t kDIM = 2u, std::size_t bSize = 100u,
          typename reference_generator =
              detail::PolyhedronReferenceGenerator<1u, false>>
class KdtSurfacesProvider : public ISurfacesProvider {
 public:
  /// The prefilled surfaces in a KD tree structure, it is generally shared
  /// amongst different providers
  ///
  /// @param kdts the prefilled KDTree structure
  /// @param kregion the region where these are pulled from
  KdtSurfacesProvider(
      std::shared_ptr<KdtSurfaces<kDIM, bSize, reference_generator>> kdts,
      const Extent& kregion)
      : m_kdt(std::move(kdts)), m_region(kregion) {
    /// Sanity check that the KDTree is not empty
    if (m_kdt == nullptr) {
      throw std::invalid_argument(
          "KdtSurfacesProvider: no KDTree structure provided.");
    }
  }

  /// The call to provide the surfaces
  std::vector<std::shared_ptr<Surface>> surfaces(
      [[maybe_unused]] const GeometryContext& gctx) const final {
    return m_kdt->surfaces(m_region);
  }

 private:
  std::shared_ptr<KdtSurfaces<kDIM, bSize, reference_generator>> m_kdt =
      nullptr;
  /// The query region
  Extent m_region;
};

}  // namespace Acts::Experimental
