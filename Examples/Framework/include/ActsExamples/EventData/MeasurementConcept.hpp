// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryIdentifier.hpp"

#include <concepts>

namespace ActsExamples {

template <typename T>
concept MeasurementConcept = requires(const T& m) {
  { m.size() } -> std::integral;
  { m.geometryId() } -> std::same_as<Acts::GeometryIdentifier>;
  { m.subspaceIndexVector() };
  { m.parameters() };
  { m.covariance() };
};

}  // namespace ActsExamples
