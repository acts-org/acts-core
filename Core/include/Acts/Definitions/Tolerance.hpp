// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <limits>

namespace Acts {

/// Tolerance for being numerical equal for geometry building
static constexpr ActsScalar s_epsilon =
    3 * std::numeric_limits<ActsScalar>::epsilon();

/// Tolerance for being on Surface
///
/// @note This is intentionally given w/o an explicit unit to avoid having
///       to include the units header unnecessarily. With the native length
///       unit of mm this corresponds to 0.1um.
static constexpr ActsScalar s_onSurfaceTolerance = 1e-4;

/// Tolerance for not being within curvilinear projection
/// this allows using the same curvilinear frame to eta = 6,
/// validity tested with IntegrationTests/PropagationTest
static constexpr ActsScalar s_curvilinearProjTolerance = 0.999995;

}  // namespace Acts
