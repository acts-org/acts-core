// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Utilities/Delegate.hpp"

namespace Acts {
struct LinearizedTrack;
class Surface;

using TrackLinearizer = Acts::Delegate<Result<LinearizedTrack>(
    const BoundTrackParameters& params, double linPointTime,
    const Surface& perigeeSurface, const Acts::GeometryContext& gctx,
    const Acts::MagneticFieldContext& mctx,
    MagneticFieldProvider::Cache& fieldCache)>;
}  // namespace Acts
