// This file is part of the Acts project.
//
// Copyright (C) 2019-2024 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
  auto geoID = candidates.front().referenceSurface().geometryId();
  // Find the appropriate cuts
  auto cuts = m_config.find(geoID);
  if (cuts == m_config.end()) {
    // for now we consider missing cuts an unrecoverable error
    // TODO consider other options e.g. do not add measurements at all (not
    // even as outliers)
    return CombinatorialKalmanFilterError::MeasurementSelectionFailed;
  }

  assert(!cuts->chi2CutOff.empty());
  const std::vector<double>& chi2CutOff = cuts->chi2CutOff;
  const double maxChi2Cut =
      std::min(*std::max_element(chi2CutOff.begin(), chi2CutOff.end()),
               getCut<traj_t>(candidates.front(), cuts, chi2CutOff, logger));
  const std::size_t numMeasurementsCut = getCut<traj_t>(
      candidates.front(), cuts, cuts->numMeasurementsCutOff, logger);

  if (numMeasurementsCut == 0ul) {
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
    if (chi2 >= maxChi2Cut) {
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

  // If there are no measurements below the chi2 cut off, return the
  // measurement with the best chi2 and tag it as an outlier
  if (passedCandidates == 0ul) {
    ACTS_VERBOSE("No measurement candidate. Return an outlier measurement chi2="
                 << minChi2);
    isOutlier = true;
  }

  if (passedCandidates <= 1ul || numMeasurementsCut == 1ul) {
    // return single item range, no sorting necessary
    ACTS_VERBOSE("Returning only 1 element");
    return Result::success(std::make_pair(candidates.begin() + minIndex,
                                          candidates.begin() + minIndex + 1));
  }

  std::sort(
      candidates.begin(), candidates.begin() + passedCandidates,
      [](const auto& tsa, const auto& tsb) { return tsa.chi2() < tsb.chi2(); });

  if (passedCandidates <= numMeasurementsCut) {
    ACTS_VERBOSE("Number of selected measurements: "
                 << passedCandidates << ", max: " << numMeasurementsCut);
    return Result::success(std::make_pair(
        candidates.begin(), candidates.begin() + passedCandidates));
  }

  ACTS_VERBOSE("Number of selected measurements: "
               << numMeasurementsCut << ", max: " << numMeasurementsCut);

  return Result::success(std::make_pair(
      candidates.begin(), candidates.begin() + numMeasurementsCut));
}

template <typename traj_t, typename cut_value_t>
cut_value_t MeasurementSelector::getCut(
    const typename traj_t::TrackStateProxy& trackState,
    const Acts::MeasurementSelector::Config::Iterator selector,
    const std::vector<cut_value_t>& cuts, const Logger& logger) {
  const auto& etaBins = selector->etaBins;
  if (etaBins.empty()) {
    return cuts[0];  // shortcut if no etaBins
  }
  const auto eta = std::atanh(std::cos(trackState.predicted()[eBoundTheta]));
  const auto abseta = std::abs(eta);
  std::size_t bin = 0;
  for (auto etaBin : etaBins) {
    if (etaBin >= abseta) {
      break;
    }
    bin++;
  }
  if (bin >= cuts.size()) {
    bin = cuts.size() - 1;
  }
  ACTS_VERBOSE("Get cut for eta=" << eta << ": " << cuts[bin]);
  return cuts[bin];
}

}  // namespace Acts
