// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/Intersection.hpp"

namespace Acts {

bool detail::checkPathLength(double pathLength, double nearLimit,
                             double farLimit, const Logger& logger) {
  // TODO why?
  const double tolerance = s_onSurfaceTolerance;

  ACTS_VERBOSE(" -> near limit, far limit, distance: "
               << nearLimit << ", " << farLimit << ", " << pathLength);

  const bool coCriterion = pathLength > nearLimit;
  const bool cpCriterion = pathLength < farLimit + tolerance;

  const bool accept = coCriterion && cpCriterion;

  if (accept) {
    ACTS_VERBOSE("Intersection is WITHIN limit");
  } else {
    ACTS_VERBOSE("Intersection is OUTSIDE limit because: ");
    if (!coCriterion) {
      ACTS_VERBOSE("- intersection path length "
                   << pathLength << " <= near limit " << nearLimit);
    }
    if (!cpCriterion) {
      ACTS_VERBOSE("- intersection path length "
                   << pathLength << " is over the far limit "
                   << (farLimit + tolerance) << " (including tolerance of "
                   << tolerance << ")");
    }
  }

  return accept;
}

}  // namespace Acts
