// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/EventData/ScalingCalibrator.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Utilities/CalibrationContext.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include <TCollection.h>
#include <TFile.h>
#include <TH2.h>
#include <TKey.h>
#include <TList.h>
#include <TString.h>

namespace Acts {
class VectorMultiTrajectory;
}  // namespace Acts

namespace detail {

std::pair<Acts::GeometryIdentifier, std::string> parseMapKey(
    const std::string& mapkey) {
  std::regex reg("^map_([0-9]+)-([0-9]+)-([0-9]+)_([xy]_.*)$");
  std::smatch matches;

  if (!std::regex_search(mapkey, matches, reg) || matches.size() != 5) {
    throw std::runtime_error("Invalid map key: " + mapkey);
  }

  std::size_t vol = std::stoull(matches[1].str());
  std::size_t lyr = std::stoull(matches[2].str());
  std::size_t mod = std::stoull(matches[3].str());

  auto geoId =
      Acts::GeometryIdentifier().withVolume(vol).withLayer(lyr).withSensitive(
          mod);

  std::string var(matches[4].str());

  return {geoId, var};
}

std::map<Acts::GeometryIdentifier, ActsExamples::ScalingCalibrator::MapTuple>
readMaps(const std::filesystem::path& path) {
  std::map<Acts::GeometryIdentifier, ActsExamples::ScalingCalibrator::MapTuple>
      maps;

  TFile ifile(path.c_str(), "READ");
  if (ifile.IsZombie()) {
    throw std::runtime_error("Unable to open TFile: " + path.string());
  }

  TList* lst = ifile.GetListOfKeys();
  assert(lst != nullptr);

  for (auto it = lst->begin(); it != lst->end(); ++it) {
    TKey* key = static_cast<TKey*>(*it);
    if (key != nullptr && std::strcmp(key->GetClassName(), "TH2D") == 0) {
      auto [geoId, var] = parseMapKey(key->GetName());

      TH2D hist;
      key->Read(&hist);

      if (var == "x_offset") {
        maps[geoId].x_offset = hist;
      } else if (var == "x_scale") {
        maps[geoId].x_scale = hist;
      } else if (var == "y_offset") {
        maps[geoId].y_offset = hist;
      } else if (var == "y_scale") {
        maps[geoId].y_scale = hist;
      } else {
        throw std::runtime_error("Unrecognized var: " + var);
      }
    }
  }
  return maps;
}

std::bitset<3> readMask(const std::filesystem::path& path) {
  TFile ifile(path.c_str(), "READ");
  if (ifile.IsZombie()) {
    throw std::runtime_error("Unable to open TFile: " + path.string());
  }

  TString* tstr = ifile.Get<TString>("v_mask");
  if (tstr == nullptr) {
    throw std::runtime_error("Unable to read mask");
  }

  return std::bitset<3>(std::string{*tstr});
}

}  // namespace detail

ActsExamples::ScalingCalibrator::ScalingCalibrator(
    const std::filesystem::path& path)
    : m_calib_maps{::detail::readMaps(path)},
      m_mask{::detail::readMask(path)} {}

void ActsExamples::ScalingCalibrator::calibrate(
    const MeasurementContainer& measurements, const ClusterContainer* clusters,
    const Acts::GeometryContext& /*gctx*/,
    const Acts::CalibrationContext& /*cctx*/,
    const Acts::SourceLink& sourceLink,
    Acts::VectorMultiTrajectory::TrackStateProxy& trackState) const {
  trackState.setUncalibratedSourceLink(Acts::SourceLink{sourceLink});
  const IndexSourceLink& idxSourceLink = sourceLink.get<IndexSourceLink>();

  assert((idxSourceLink.index() < measurements.size()) &&
         "Source link index is outside the container bounds");

  auto geoId = trackState.referenceSurface().geometryId();
  auto mgid =
      Acts::GeometryIdentifier()
          .withVolume(geoId.volume() *
                      static_cast<Acts::GeometryIdentifier::Value>(m_mask[2]))
          .withLayer(geoId.layer() *
                     static_cast<Acts::GeometryIdentifier::Value>(m_mask[1]))
          .withSensitive(
              geoId.sensitive() *
              static_cast<Acts::GeometryIdentifier::Value>(m_mask[0]));
  const Cluster& cl = clusters->at(idxSourceLink.index());
  ConstantTuple ct = m_calib_maps.at(mgid).at(cl.sizeLoc0, cl.sizeLoc1);

  const ConstVariableBoundMeasurementProxy measurement =
      measurements.getMeasurement(idxSourceLink.index());

  assert(measurement.contains(Acts::eBoundLoc0) &&
         "Measurement does not contain the required bound loc0");
  assert(measurement.contains(Acts::eBoundLoc1) &&
         "Measurement does not contain the required bound loc1");

  auto boundLoc0 = measurement.indexOf(Acts::eBoundLoc0);
  auto boundLoc1 = measurement.indexOf(Acts::eBoundLoc1);

  Acts::visit_measurement(measurement.size(), [&](auto N) -> void {
    constexpr std::size_t kMeasurementSize = decltype(N)::value;
    const ConstFixedBoundMeasurementProxy<kMeasurementSize> fixedMeasurement =
        static_cast<ConstFixedBoundMeasurementProxy<kMeasurementSize>>(
            measurement);

    Acts::ActsVector<kMeasurementSize> calibratedParameters =
        fixedMeasurement.parameters();
    Acts::ActsSquareMatrix<kMeasurementSize> calibratedCovariance =
        fixedMeasurement.covariance();

    calibratedParameters[boundLoc0] += ct.x_offset;
    calibratedParameters[boundLoc1] += ct.y_offset;
    calibratedCovariance(boundLoc0, boundLoc0) *= ct.x_scale;
    calibratedCovariance(boundLoc1, boundLoc1) *= ct.y_scale;

    trackState.allocateCalibrated(calibratedParameters, calibratedCovariance);
    trackState.setProjectorSubspaceIndices(fixedMeasurement.subspaceIndices());
  });
}
