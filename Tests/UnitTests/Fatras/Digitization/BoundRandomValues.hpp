// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <array>

namespace ActsFatras {

/// Generate ranom numbers within the regular, centric Rectangle.
struct RectangleRandom {
  double xmax;
  double ymax;

  /// Constructor from half lengths.
  ///
  /// @param xmax_ the half length in x
  /// @param ymax_ the half length in y
  RectangleRandom(double xmax_, double ymax_) : xmax(xmax_), ymax(ymax_) {}

  /// Given two random numbers @param r0 and @param r1
  /// generate a x/y position inside the Rectangle @return
  ///
  /// @note r0, r1 need to be within [0,1]
  Acts::Vector2 operator()(double r0, double r1) const {
    return {(2 * r0 - 1) * xmax, (2 * r1 - 1) * ymax};
  }
};

/// Create Random variables inside a regular Trapezoid.
struct TrapezoidRandom {
  double xminy;
  double xmaxy;
  double ymin;
  double ymax;

  /// Constructor for TrapezoidBounds : PlaneBounds
  ///
  /// @param xminy_ the x value at minimum y value
  /// @param xmaxy_ the x value at maximum y value
  /// @param ymax_ the half length in y
  TrapezoidRandom(double xminy_, double xmaxy_, double ymax_)
      : xminy(xminy_), xmaxy(xmaxy_), ymin(-ymax_), ymax(ymax_) {}

  /// Constructor for DiscTrapezoidBounds : DiscBounds
  ///
  /// @param xminy_ the x value at minimum y value
  /// @param xmaxy_ the x value at maximum y value
  /// @param ymin_ the minimum y value (conicides with rmin)
  /// @param ymax_ the maximum y value
  TrapezoidRandom(double xminy_, double xmaxy_, double ymin_, double ymax_)
      : xminy(xminy_), xmaxy(xmaxy_), ymin(ymin_), ymax(ymax_) {}

  /// Given two random numbers @param r0 and @param r1
  /// generate a x/y position inside the Trapezoid @return
  ///
  /// @note r0, r1 need to be within [0,1]
  Acts::Vector2 operator()(double r0, double r1) const {
    double y = ymin + (ymax - ymin) * r1;
    double xmax = xminy + (xmaxy - xminy) / (ymax - ymin) * (y - ymin);
    double x = (2 * r0 - 1) * xmax;
    return {x, y};
  }
};

/// Generate ranom numbers within disc ring
struct DiscRandom {
  double rmin;
  double rmax;
  double phimin;
  double phimax;

  /// Constructor for RadiablBounds : DiscBounds
  ///
  /// @param rmin_ the minimum r of the disc
  /// @param rmax_ the maximum r of the disc
  /// @param phimin_ the minimum phi value of the disc
  /// @param phimax_ the maximum phi value of the disc
  DiscRandom(double rmin_, double rmax_, double phimin_, double phimax_)
      : rmin(rmin_), rmax(rmax_), phimin(phimin_), phimax(phimax_) {}

  /// Given two random numbers @param r0 and @param r1
  /// generate a x/y position inside the Disc @return
  ///
  /// @note r0, r1 need to be within [0,1]
  Acts::Vector2 operator()(double r0, double r1) const {
    double r = rmin + (rmax - rmin) * r0;
    double phi = phimin + (phimax - phimin) * r1;
    return {r * std::cos(phi), r * std::sin(phi)};
  }
};

/// Generate random numbers within an Annulus object
struct AnnulusRandom {
  double rmin;
  double rmax;
  double phimins;   // strip system
  double phimaxs;   // strip system
  double originxs;  // strip system origin x
  double originys;  // strip system origin y

  /// Constructor for RadiablBounds : DiscBounds
  ///
  /// @param rmin_ the minimum r of the disc
  /// @param rmax_ the maximum r of the disc
  /// @param phimins_ the minimum phi - strip system
  /// @param phimaxs_ the maximum phi - strip system
  /// @param originxs_ the origin x - strip system
  /// @param originys_ the origin y - strip system
  AnnulusRandom(double rmin_, double rmax_, double phimins_, double phimaxs_,
                double originxs_, double originys_)
      : rmin(rmin_),
        rmax(rmax_),
        phimins(phimins_),
        phimaxs(phimaxs_),
        originxs(originxs_),
        originys(originys_) {}

  /// Given two random numbers @param r0 and @param r1
  /// generate a x/y position inside Annulus shape and  @return
  ///
  /// @note r0, r1 need to be within [0,1]
  Acts::Vector2 operator()(double r0, double r1) const {
    double r = rmin + (rmax - rmin) * r0;
    double phi = phimins + (phimaxs - phimins) * r1;
    return {r * std::cos(phi), r * std::sin(phi)};
  }
};

}  // namespace ActsFatras
