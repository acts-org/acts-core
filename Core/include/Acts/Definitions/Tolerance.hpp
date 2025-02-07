// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <limits>

namespace Acts {

/// Tolerance for being numerical equal for geometry building
static constexpr double s_epsilon = 3 * std::numeric_limits<double>::epsilon();

/// Tolerance for being on Surface
///
/// @note This is intentionally given w/o an explicit unit to avoid having
///       to include the units header unnecessarily. With the native length
///       unit of mm this corresponds to 0.1um.
static constexpr double s_onSurfaceTolerance = 1e-4;

/// Tolerance for not being within curvilinear projection
/// this allows using the same curvilinear frame to eta = 6,
/// validity tested with IntegrationTests/PropagationTest
static constexpr double s_curvilinearProjTolerance = 0.999995;

}  // namespace Acts
