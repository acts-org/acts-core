// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <covfie/core/algebra/affine.hpp>
#include <covfie/core/backend/primitive/array.hpp>
#include <covfie/core/backend/primitive/constant.hpp>
#include <covfie/core/backend/transformer/affine.hpp>
#include <covfie/core/backend/transformer/clamp.hpp>
#include <covfie/core/backend/transformer/linear.hpp>
#include <covfie/core/backend/transformer/strided.hpp>
#include <covfie/core/field.hpp>
#include <covfie/core/field_view.hpp>
#include <covfie/core/parameter_pack.hpp>

// acts includes
#include "Acts/MagneticField/BFieldMapUtils.hpp"
#include "Acts/MagneticField/ConstantBField.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"

namespace Acts::CovfiePlugin {

using BuilderBackend =
    covfie::backend::strided<covfie::vector::size3,
                             covfie::backend::array<covfie::vector::float3>>;

using InterpolatedField = covfie::field<covfie::backend::clamp<
    covfie::backend::affine<covfie::backend::linear<BuilderBackend>>>>;

using ConstantField = covfie::field<
    covfie::backend::constant<covfie::vector::float3, covfie::vector::float3>>;

/// @brief Creates a covfie field from an interpolated magnetic field.
/// @param magneticField The acts interpolated magnetic field.
/// @return An affine linear strided covfie field.
InterpolatedField covfieField(
    const Acts::InterpolatedMagneticField& magneticField);

/// @brief Creates a covfie field from a constant B field.
/// @param magneticField The acts constant magnetic field.
/// @return A constant covfie field.
ConstantField covfieField(const Acts::ConstantBField& magneticField);

/// @brief Creates a covfie field from a magnetic field provider by sampling it.
/// The field must be defined within min (inclusive) and max (inclusive).
/// @param magneticField The acts magnetic field provider.
/// @param cache The acts cache.
/// @param nPoints 3D array of containing the number of bins for each axis.
/// @param min (min_x, min_y, min_z)
/// @param max (max_x, max_y, max_z)
/// @return An affine linear strided covfie field.
InterpolatedField covfieField(const Acts::MagneticFieldProvider& magneticField,
                              Acts::MagneticFieldProvider::Cache& cache,
                              const std::array<std::size_t, 3>& nPoints,
                              const Acts::Vector3& min,
                              const Acts::Vector3& max);

}  // namespace Acts::CovfiePlugin
