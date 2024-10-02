// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <boost/test/data/test_case.hpp>

#include "Acts/Definitions/Units.hpp"
#include "Acts/Surfaces/CurvilinearSurface.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RegularSurface.hpp"

#include <cmath>
#include <vector>

namespace {

namespace bdata = boost::unit_test::data;
using namespace Acts;

// reference surfaces
// this includes only those surfaces that can take unbounded local positions as
// inputs, i.e. no angles or strictly positive radii.
const auto surfaces =
    bdata::make(std::vector<std::shared_ptr<const RegularSurface>>{
        Surface::makeShared<CylinderSurface>(
            Transform3::Identity(), 10 /* radius */, 100 /* half-length z */),
        // TODO perigee roundtrip local->global->local does not seem to work
        // Surface::makeShared<PerigeeSurface>(Vector3(0, 0, -1.5)),
        CurvilinearSurface(Vector3::Zero(), Vector3::UnitX()).planeSurface(),
        CurvilinearSurface(Vector3::Zero(), Vector3::UnitY()).planeSurface(),
        CurvilinearSurface(Vector3::Zero(), Vector3::UnitZ()).planeSurface(),
    });
// positions
const auto posAngle = bdata::xrange(-M_PI, M_PI, 0.50);
const auto posPositiveNonzero = bdata::xrange(0.25, 1.0, 0.25);
const auto posPositive = bdata::make(0.0) + posPositiveNonzero;
const auto posSymmetric = bdata::xrange(-1.0, 1.0, 0.50);
// time
const auto ts = bdata::make(1.0);
// direction angles
const auto phis = bdata::make({0.0, M_PI, -M_PI, M_PI_2, -M_PI_2});
const auto thetasNoForwardBackward = bdata::xrange(M_PI_4, M_PI, M_PI_4);
const auto thetas = bdata::make({0.0, M_PI}) + thetasNoForwardBackward;
// absolute momenta
const auto ps = bdata::make({1.0, 10.0});
// charges
const auto qsNonZero = bdata::make({-UnitConstants::e, UnitConstants::e});
const auto qsAny = bdata::make({
    -2 * UnitConstants::e,
    -1 * UnitConstants::e,
    0 * UnitConstants::e,
    1 * UnitConstants::e,
    2 * UnitConstants::e,
});

}  // namespace
