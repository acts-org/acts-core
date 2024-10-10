// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

namespace Acts {

template <typename traj_t>
Result<
    std::pair<typename std::vector<typename traj_t::TrackStateProxy>::iterator,
              typename std::vector<typename traj_t::TrackStateProxy>::iterator>>
MeasurementSelector::select(
    std::vector<typename traj_t::TrackStateProxy>& candidates, bool& isOutlier,
    const Logger& logger) const {
  using Result = Result<std::pair<
      typename std::vector<typename traj_t::TrackStateProxy>::iterator,
      typename std::vector<typename traj_t::TrackStateProxy>::iterator>>;

  ACTS_VERBOSE("Invoked MeasurementSelector");

  // Return error if no measurement
  if (candidates.empty()) {
    return CombinatorialKalmanFilterError::MeasurementSelectionFailed;
  }

  // Get geoID of this surface
  GeometryIdentifier geoID = candidates.front().referenceSurface().geometryId();
  // Get the theta of the first track state
  const double theta = candidates.front().predicted()[eBoundTheta];
  // Find the appropriate cuts
  const auto cutsResult = getCuts(geoID, theta);
  if (!cutsResult.ok()) {
    return cutsResult.error();
  }
  const Cuts& cuts = *cutsResult;

  if (cuts.numMeasurements == 0ul) {
    return CombinatorialKalmanFilterError::MeasurementSelectionFailed;
  }

  double minChi2 = std::numeric_limits<double>::max();
  std::size_t minIndex = std::numeric_limits<std::size_t>::max();

  isOutlier = false;

  // Loop over all measurements to select the compatible measurements
  // Sort track states which do not satisfy the chi2 cut to the end.
  // When done trackStateIterEnd will point to the first element that
  // does not satisfy the chi2 cut.
  std::size_t passedCandidates = 0ul;
  for (std::size_t i(0ul); i < candidates.size(); ++i) {
    auto& trackState = candidates[i];

    // We access the dynamic size of the matrix here but use them later
    // through a template function which accesses the data pointer
    // with compile time size. That way the Eigen math operations are
    // still done with compile time size and no dynamic memory allocation
    // is needed.
    double chi2 = calculateChi2(
        trackState.effectiveCalibrated().data(),
        trackState.effectiveCalibratedCovariance().data(),
        trackState.predicted(), trackState.predictedCovariance(),
        trackState.boundSubspaceIndices(), trackState.calibratedSize());
    trackState.chi2() = chi2;

    if (chi2 < minChi2) {
      minChi2 = chi2;
      minIndex = i;
    }

    // only consider track states which pass the chi2 cut
    if (chi2 >= cuts.chi2Measurement) {
      continue;
    }

    if (passedCandidates != i) {
      std::swap(candidates[passedCandidates], candidates[i]);
      if (minIndex == i) {
        minIndex = passedCandidates;
      }
    }
    ++passedCandidates;
  }

  // Handle if there are no measurements below the chi2 cut off
  if (passedCandidates == 0ul) {
    if (minChi2 < cuts.chi2Outlier) {
      ACTS_VERBOSE(
          "No measurement candidate. Return an outlier measurement chi2="
          << minChi2);
      isOutlier = true;
      return Result::success(std::make_pair(candidates.begin() + minIndex,
                                            candidates.begin() + minIndex + 1));
    } else {
      ACTS_VERBOSE("No measurement candidate. Return empty chi2=" << minChi2);
      return Result::success(
          std::make_pair(candidates.begin(), candidates.begin()));
    }
  }

  if (passedCandidates <= 1ul) {
    // return single item range, no sorting necessary
    ACTS_VERBOSE("Returning only 1 element chi2=" << minChi2);
    return Result::success(std::make_pair(candidates.begin() + minIndex,
                                          candidates.begin() + minIndex + 1));
  }

  std::sort(
      candidates.begin(), candidates.begin() + passedCandidates,
      [](const auto& tsa, const auto& tsb) { return tsa.chi2() < tsb.chi2(); });

  ACTS_VERBOSE("Number of selected measurements: "
               << passedCandidates << ", max: " << cuts.numMeasurements);

  return Result::success(std::make_pair(
      candidates.begin(),
      candidates.begin() + std::min(cuts.numMeasurements, passedCandidates)));
}

}  // namespace Acts
