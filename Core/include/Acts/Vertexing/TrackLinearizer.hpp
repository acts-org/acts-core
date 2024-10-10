// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
