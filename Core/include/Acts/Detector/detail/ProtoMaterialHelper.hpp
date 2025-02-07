// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Geometry/GeometryContext.hpp"

namespace Acts {

class Surface;

namespace Experimental {

struct BinningDescription;

namespace detail::ProtoMaterialHelper {

/// @brief Method that attaches proto material to a surface given
/// a proto binning description
///
/// @param gctx is the geometry context, needed for extent measuring
/// @param surface is the portal where the material is attached
/// @param bDescription is the binning description for the proto binning
///
/// @return an (eventual) updated binning description for structured
///         screen logging output
BinningDescription attachProtoMaterial(const GeometryContext& gctx,
                                       Surface& surface,
                                       const BinningDescription& bDescription);

}  // namespace detail::ProtoMaterialHelper
}  // namespace Experimental
}  // namespace Acts
