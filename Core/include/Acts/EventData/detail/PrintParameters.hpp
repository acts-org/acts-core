// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"

#include <cstdint>
#include <iosfwd>

namespace Acts {

class Surface;

namespace detail {

/// Print bound track parameters content to the output stream.
///
/// @param os The output stream
/// @param surface Bound parameters reference surface
/// @param params Bound parameters vector
/// @param cov Optional bound parameters covariance matrix
void printBoundParameters(std::ostream& os, const Surface& surface,
                          const BoundVector& params,
                          const BoundSquareMatrix* cov = nullptr);

/// Print free track parameters content to the output stream.
///
/// @param os The output stream
/// @param params Free parameters vector
/// @param cov Optional free parameters covariance matrix
void printFreeParameters(std::ostream& os, const FreeVector& params,
                         const FreeMatrix* cov = nullptr);

/// Print bound measurement content to the output stream.
///
/// @param os The output stream
/// @param size Size of the measurement space
/// @param indices Which parameters are measured, must contain size elements
/// @param params Parameters vector data, must contain size elements
/// @param cov Optional Covariance matrix data, must contain sizexsize elements
void printMeasurement(std::ostream& os, BoundIndices size,
                      const std::uint8_t* indices, const ActsScalar* params,
                      const ActsScalar* cov);

/// Print free measurement content to the output stream.
///
/// @param os The output stream
/// @param size Size of the measurement space
/// @param indices Which parameters are measured, must contain size elements
/// @param params Parameters vector data, must contain size elements
/// @param cov Optional Covariance matrix data, must contain sizexsize elements
void printMeasurement(std::ostream& os, FreeIndices size,
                      const std::uint8_t* indices, const ActsScalar* params,
                      const ActsScalar* cov);

}  // namespace detail
}  // namespace Acts
