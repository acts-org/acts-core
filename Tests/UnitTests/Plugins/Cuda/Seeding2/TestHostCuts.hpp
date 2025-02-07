// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

// Local include(s).
#include "TestSpacePoint.hpp"

// Acts include(s).
#include "Acts/Seeding/IExperimentCuts.hpp"

/// Custom selection cuts for the test, used on the host
class TestHostCuts : public Acts::IExperimentCuts<TestSpacePoint> {
 public:
  /// Returns seed weight bonus/malus depending on detector considerations.
  /// @param bottom bottom space point of the current seed
  /// @param middle middle space point of the current seed
  /// @param top top space point of the current seed
  /// @return seed weight to be added to the seed's weight
  float seedWeight(
      const Acts::InternalSpacePoint<TestSpacePoint>& bottom,
      const Acts::InternalSpacePoint<TestSpacePoint>& middle,
      const Acts::InternalSpacePoint<TestSpacePoint>& top) const final;

  /// @param weight the current seed weight
  /// @param bottom bottom space point of the current seed
  /// @param middle middle space point of the current seed
  /// @param top top space point of the current seed
  /// @return true if the seed should be kept, false if the seed should be
  /// discarded
  bool singleSeedCut(
      float weight, const Acts::InternalSpacePoint<TestSpacePoint>& bottom,
      const Acts::InternalSpacePoint<TestSpacePoint>&,
      const Acts::InternalSpacePoint<TestSpacePoint>&) const final;

  /// @param seedCandidates contains collection of seed candidates created for one middle
  /// space point in a std::tuple format
  /// @return vector of seed candidates that pass the cut
  std::vector<typename Acts::CandidatesForMiddleSp<
      const Acts::InternalSpacePoint<TestSpacePoint>>::value_type>
  cutPerMiddleSP(
      std::vector<typename Acts::CandidatesForMiddleSp<
          const Acts::InternalSpacePoint<TestSpacePoint>>::value_type>
          seedCandidates) const final;

};  // struct TestHostCuts
