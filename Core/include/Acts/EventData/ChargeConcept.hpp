// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/Types.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/HashedString.hpp"

#include <any>
#include <concepts>
#include <type_traits>

namespace Acts {

template <typename C>
concept ChargeConcept = requires(C c, C c2, float f, double d) {
  { C{f} };

  { c == c2 } -> std::same_as<bool>;
  { c != c2 } -> std::same_as<bool>;

  { c.absQ() } -> std::same_as<float>;
  { c.extractCharge(d) } -> std::same_as<float>;
  { c.extractMomentum(d) } -> std::same_as<double>;
  { c.qOverP(d, d) } -> std::same_as<double>;
};

}  // namespace Acts
