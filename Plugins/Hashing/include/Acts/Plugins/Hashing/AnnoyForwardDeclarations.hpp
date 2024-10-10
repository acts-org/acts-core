// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

namespace Annoy {
// Forward declarations for the Annoy classes we use.
struct AngularEuclidean;  // Metric used
struct Kiss32Random;      // Random type, not a template
template <typename S, typename T, typename Distance, typename Random,
          typename BuildPolicy>
class AnnoyIndex;  // AnnoyIndex template class

class AnnoyIndexSingleThreadedBuildPolicy;  // Build policy
}  // namespace Annoy

// Define commonly used Annoy types
namespace Acts {
using AnnoyMetric = Annoy::AngularEuclidean;
using AnnoyModel =
    Annoy::AnnoyIndex<unsigned int, double, AnnoyMetric, Annoy::Kiss32Random,
                      Annoy::AnnoyIndexSingleThreadedBuildPolicy>;
}  // namespace Acts
