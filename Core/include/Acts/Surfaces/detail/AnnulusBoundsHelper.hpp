// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <memory>
#include <tuple>
#include <vector>

namespace Acts {

class AnnulusBounds;

namespace detail::AnnulusBoundsHelper {

/// @brief The factory function to create an annulus bounds
///
/// @param transform the transform of final surface object
/// @param rMin minimal radius in disc frame
/// @param rMax maximal radius in disc frame
/// @param vertices the vertices of the cutout trapezoid
///
/// @note - for the time being, the vertices follow ROOT::TGeo convention
/// i.e. [0 - 3] vertices defince clockwise (sic!) the plane at -z
/// and [4 - 7] vertices define clockwise (sic!) the plane at +z
///
/// @return AnnulusBounds
std::tuple<std::shared_ptr<AnnulusBounds>, Transform3> create(
    const Transform3& transform, double rMin, double rMax,
    std::vector<Vector2> vertices);

}  // namespace detail::AnnulusBoundsHelper
}  // namespace Acts
