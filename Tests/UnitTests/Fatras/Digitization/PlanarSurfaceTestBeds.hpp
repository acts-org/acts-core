// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/DiscBounds.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/DiscTrapezoidBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinningType.hpp"

#include <array>
#include <tuple>
#include <vector>

#include "BoundRandomValues.hpp"

namespace ActsFatras {

using Randomizer = std::function<Acts::Vector2(double, double)>;

using PlanarTestBed =
    std::tuple<std::string, std::shared_ptr<const Acts::Surface>,
               Acts::BinUtility, Randomizer>;

/// Helper struct to create a testbed for Digitization steps
struct PlanarSurfaceTestBeds {
  /// Call operator for creating a testbed of different surfaces.
  /// It returns a testbed for all planar surface types and the
  /// corresponding cartesian/polar segmentation.
  ///
  /// @param rScale is a parameter how far the random numbers
  /// should be generated (1 -> inside to boundary, 1.1 -> 10% outside)
  std::vector<PlanarTestBed> operator()(double rScale) const {
    double irScale = (2. - rScale);

    // Pixel test in Rectangle
    double xhalf = 3.;
    double yhalf = 6.5;
    auto rectangle = std::make_shared<Acts::RectangleBounds>(xhalf, yhalf);
    auto rSurface = Acts::Surface::makeShared<Acts::PlaneSurface>(
        Acts::Transform3::Identity(), rectangle);
    Acts::BinUtility pixelated(15, -xhalf, xhalf, Acts::open,
                               Acts::BinningValue::binX);
    pixelated += Acts::BinUtility(26, -yhalf, yhalf, Acts::open,
                                  Acts::BinningValue::binY);
    RectangleRandom rRandom(xhalf * rScale, yhalf * rScale);

    // Cartesian strip test in Trapezoid
    double xhalfminy = 2.;
    double xhalfmaxy = 3.5;
    yhalf = 4.;
    auto trapezoid =
        std::make_shared<Acts::TrapezoidBounds>(xhalfminy, xhalfmaxy, yhalf);
    auto tSurface = Acts::Surface::makeShared<Acts::PlaneSurface>(
        Acts::Transform3::Identity(), trapezoid);
    Acts::BinUtility stripsX(35, -xhalfmaxy, xhalfmaxy, Acts::open,
                             Acts::BinningValue::binX);
    stripsX += Acts::BinUtility(1, -yhalf, yhalf, Acts::open,
                                Acts::BinningValue::binY);
    TrapezoidRandom tRandom(xhalfminy * rScale, xhalfmaxy * rScale,
                            yhalf * rScale);

    // Phi strip test in DiscTrapezoid
    double rmin = 2.;
    double rmax = 7.5;
    double xmin = 2.;
    double xmax = 3.5;
    double ymax = std::sqrt(rmax * rmax - xmax * xmax);
    double alpha = std::max(atan2(xmin, rmin), atan2(xmax, ymax));

    auto discTrapezoid =
        std::make_shared<Acts::DiscTrapezoidBounds>(xmin, xmax, rmin, rmax);
    auto dtSurface = Acts::Surface::makeShared<Acts::DiscSurface>(
        Acts::Transform3::Identity(), discTrapezoid);
    Acts::BinUtility stripsPhi(1, rmin, rmax, Acts::open,
                               Acts::BinningValue::binR);
    stripsPhi += Acts::BinUtility(25, M_PI_2 - alpha, M_PI_2 + alpha,
                                  Acts::open, Acts::BinningValue::binPhi);
    TrapezoidRandom dtRandom(xmin * rScale, xmax * rScale, rmin * irScale,
                             ymax * rScale);

    // Raidal disc test
    auto discRadial =
        std::make_shared<Acts::RadialBounds>(rmin, rmax, M_PI_4, M_PI_2);
    auto dSurface = Acts::Surface::makeShared<Acts::DiscSurface>(
        Acts::Transform3::Identity(), discRadial);
    Acts::BinUtility rphiseg(10, rmin, rmax, Acts::open,
                             Acts::BinningValue::binR);
    rphiseg += Acts::BinUtility(20, (M_PI_2 - M_PI_4), (M_PI_2 + M_PI_4),
                                Acts::open, Acts::BinningValue::binPhi);

    DiscRandom dRandom(rmin * irScale, rmax * rScale,
                       (M_PI_2 - M_PI_4) * irScale, (M_PI_2 + M_PI_4) * rScale);

    // Annulus disc test
    rmin = 2.5;
    rmax = 5.5;
    Acts::Vector2 aorigin(0.1, -0.3);
    double phimin = -0.25;
    double phimax = 0.38;
    auto annulus = std::make_shared<Acts::AnnulusBounds>(rmin, rmax, phimin,
                                                         phimax, aorigin);
    auto aSurface = Acts::Surface::makeShared<Acts::DiscSurface>(
        Acts::Transform3::Identity() *
            Acts::Translation3(-aorigin.x(), -aorigin.y(), 0.),
        annulus);

    auto vertices = annulus->vertices(72);
    std::for_each(vertices.begin(), vertices.end(), [&](Acts::Vector2& v) {
      double r = Acts::VectorHelpers::perp(v);
      rmin = std::min(rmin, r);
      rmax = std::max(rmax, r);
    });

    Acts::BinUtility stripsPhiA(1, rmin, rmax, Acts::open,
                                Acts::BinningValue::binR);
    stripsPhiA += Acts::BinUtility(12, phimin, phimax, Acts::open,
                                   Acts::BinningValue::binPhi);
    AnnulusRandom aRandom(rmin * irScale, rmax * rScale, phimin * rScale,
                          phimax * rScale, aorigin.x(), aorigin.y());

    return {{"Rectangle", std::move(rSurface), pixelated, rRandom},
            {"Trapezoid", std::move(tSurface), stripsX, tRandom},
            {"DiscTrapezoid", std::move(dtSurface), stripsPhi, dtRandom},
            {"DiscRadial", std::move(dSurface), rphiseg, dRandom},
            {"Annulus", std::move(aSurface), stripsPhiA, aRandom}};
  }
};

}  // namespace ActsFatras
