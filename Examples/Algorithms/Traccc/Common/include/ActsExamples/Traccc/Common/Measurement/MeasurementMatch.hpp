// This file is part of the Acts project.
//
// Copyright (C) 2023 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

// Acts include(s)
#include "Acts/Geometry/GeometryIdentifier.hpp"

// Acts examples include(s)
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/Traccc/Common/Util/IndexMap.hpp"

// System include(s).
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <map>
#include <vector>

// This file is for matching similar measurements in two collections of
// measurements.

namespace ActsExamples::Traccc::Common::Measurement {

/// @brief Get the geometry ID from the measurement through its source link.
/// @note Sourcelink is assumed to be of type IndexSourceLink.
inline Acts::GeometryIdentifier getGeometryID(
    const ActsExamples::BoundVariantMeasurement& measurement) {
  return std::visit(
      [](auto& m) {
        return m.sourceLink()
            .template get<ActsExamples::IndexSourceLink>()
            .geometryId();
      },
      measurement);
}

/// @brief Checks if two measurements are equal based on their geometry ID and position alone.
/// @param measurement1 the first measurement.
/// @param measurement2 the second measurement.
/// @param maxDistance the tolerance of the difference in local position.
/// The maximum distance between the local positions of the measurements must be
/// less or equal to this value to be considered equal.
/// @returns true or false depending on whether they are considered equal.
inline bool measurementEqual(
    const ActsExamples::BoundVariantMeasurement& measurement1,
    const ActsExamples::BoundVariantMeasurement& measurement2,
    const double maxDistance = .001) {
  auto gidEq = getGeometryID(measurement1) == getGeometryID(measurement2);

  auto sqNorm =
      (Conversion::getLocal(measurement1) - Conversion::getLocal(measurement2))
          .squaredNorm();
  auto locEq = sqNorm <= maxDistance * maxDistance;

  return gidEq && locEq;
}

/// @brief Generates a hash for the measurement.
/// This hash is used for the locality sensitive hashing to calculate the match
/// map. Thus, this hash is not sensitive to small variations in position that
/// could could from numerical errors.
inline std::size_t measurementHash(
    const ActsExamples::BoundVariantMeasurement& measurement) {
  // The hash function can be optimized to reduce collisions.
  return static_cast<std::size_t>(getGeometryID(measurement).value());
}

namespace {
const auto wrappedHash =
    std::function<std::size_t(const ActsExamples::BoundVariantMeasurement&)>(
        measurementHash);
const auto wrappedEq =
    std::function<bool(const ActsExamples::BoundVariantMeasurement&,
                       const ActsExamples::BoundVariantMeasurement&)>(
        [](const ActsExamples::BoundVariantMeasurement& m1,
           const ActsExamples::BoundVariantMeasurement& m2) {
          return measurementEqual(m1, m2);
        });
}  // namespace

/// @brief Creates a match map by matching the elements in two collection based on their geometry id and approximate position.
/// The two collections of measurements are assumed to contain the same
/// measurements. However, equivalent measurements may be at different indices
/// in the collections. This function determines which indexes correspond to
/// matching measurements.
inline auto matchMap(
    const std::vector<ActsExamples::BoundVariantMeasurement>& from,
    const std::vector<ActsExamples::BoundVariantMeasurement>& to) {
  return Util::matchMap(from, to, wrappedHash, wrappedEq);
}

}  // namespace ActsExamples::Traccc::Common::Measurement
