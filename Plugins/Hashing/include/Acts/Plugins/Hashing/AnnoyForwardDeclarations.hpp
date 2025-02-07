// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
