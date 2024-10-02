// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TrackFinding/HoughTransformSeeder.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "ActsExamples/EventData/GeometryContainers.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/ProtoTrack.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/TrackFinding/DefaultHoughFunctions.hpp"
#include "ActsExamples/Utilities/GroupBy.hpp"
#include "ActsExamples/Utilities/Range.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <variant>

static inline int quant(double min, double max, unsigned nSteps, double val);
static inline double unquant(double min, double max, unsigned nSteps, int step);
template <typename T>
static inline std::string to_string(std::vector<T> v);

ActsExamples::HoughTransformSeeder::HoughTransformSeeder(
    ActsExamples::HoughTransformSeeder::Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("HoughTransformSeeder", lvl),
      m_cfg(std::move(cfg)),
      m_logger(Acts::getDefaultLogger("HoughTransformSeeder", lvl)) {
  // require spacepoints or input measurements (or both), but at least one kind
  // of input
  bool foundInput = false;
  for (const auto& spName : m_cfg.inputSpacePoints) {
    if (!(spName.empty())) {
      foundInput = true;
    }

    auto& handle = m_inputSpacePoints.emplace_back(
        std::make_unique<ReadDataHandle<SimSpacePointContainer>>(
            this,
            "InputSpacePoints#" + std::to_string(m_inputSpacePoints.size())));
    handle->initialize(spName);
  }
  if (!(m_cfg.inputMeasurements.empty())) {
    foundInput = true;
  }

  if (!foundInput) {
    throw std::invalid_argument(
        "HoughTransformSeeder: Missing some kind of input (measurements of "
        "spacepoints)");
  }

  if (m_cfg.outputProtoTracks.empty()) {
    throw std::invalid_argument(
        "HoughTransformSeeder: Missing hough tracks output collection");
  }
  if (m_cfg.outputSeeds.empty()) {
    throw std::invalid_argument(
        "HoughTransformSeeder: Missing hough track seeds output collection");
  }

  if (m_cfg.inputSourceLinks.empty()) {
    throw std::invalid_argument(
        "HoughTransformSeeder: Missing source link input collection");
  }

  m_outputProtoTracks.initialize(m_cfg.outputProtoTracks);
  m_inputSourceLinks.initialize(m_cfg.inputSourceLinks);
  m_inputMeasurements.initialize(m_cfg.inputMeasurements);

  if (!m_cfg.trackingGeometry) {
    throw std::invalid_argument(
        "HoughTransformSeeder: Missing tracking geometry");
  }

  if (m_cfg.geometrySelection.empty()) {
    throw std::invalid_argument(
        "HoughTransformSeeder: Missing geometry selection");
  }
  // ensure geometry selection contains only valid inputs
  for (const auto& geoId : m_cfg.geometrySelection) {
    if ((geoId.approach() != 0u) || (geoId.boundary() != 0u) ||
        (geoId.sensitive() != 0u)) {
      throw std::invalid_argument(
          "HoughTransformSeeder: Invalid geometry selection: only volume and "
          "layer are allowed to be set");
    }
  }
  // remove geometry selection duplicates
  //
  // the geometry selections must be mutually exclusive, i.e. if we have a
  // selection that contains both a volume and a layer within that same volume,
  // we would create the space points for the layer twice.
  auto isDuplicate = [](Acts::GeometryIdentifier ref,
                        Acts::GeometryIdentifier cmp) {
    // code assumes ref < cmp and that only volume and layer can be non-zero
    // root node always contains everything
    if (ref.volume() == 0) {
      return true;
    }
    // unequal volumes always means separate hierarchies
    if (ref.volume() != cmp.volume()) {
      return false;
    }
    // within the same volume hierarchy only consider layers
    return (ref.layer() == cmp.layer());
  };
  // sort geometry selection so the unique filtering works
  std::ranges::sort(m_cfg.geometrySelection,
                    std::less<Acts::GeometryIdentifier>{});
  auto geoSelBeg = m_cfg.geometrySelection.begin();
  auto geoSelEnd = m_cfg.geometrySelection.end();
  auto geoSelLastUnique = std::unique(geoSelBeg, geoSelEnd, isDuplicate);
  if (geoSelLastUnique != geoSelEnd) {
    ACTS_WARNING("Removed " << std::distance(geoSelLastUnique, geoSelEnd)
                            << " geometry selection duplicates");
    m_cfg.geometrySelection.erase(geoSelLastUnique, geoSelEnd);
  }
  ACTS_INFO("Hough geometry selection:");
  for (const auto& geoId : m_cfg.geometrySelection) {
    ACTS_INFO("  " << geoId);
  }

  // Fill convenience variables
  m_step_x = (m_cfg.xMax - m_cfg.xMin) / m_cfg.houghHistSize_x;
  m_step_y = (m_cfg.yMax - m_cfg.yMin) / m_cfg.houghHistSize_y;
  for (unsigned i = 0; i <= m_cfg.houghHistSize_x; i++) {
    m_bins_x.push_back(
        unquant(m_cfg.xMin, m_cfg.xMax, m_cfg.houghHistSize_x, i));
  }
  for (unsigned i = 0; i <= m_cfg.houghHistSize_y; i++) {
    m_bins_y.push_back(
        unquant(m_cfg.yMin, m_cfg.yMax, m_cfg.houghHistSize_y, i));
  }

  m_cfg.fieldCorrector
      .connect<&ActsExamples::DefaultHoughFunctions::fieldCorrectionDefault>();
  m_cfg.layerIDFinder
      .connect<&ActsExamples::DefaultHoughFunctions::findLayerIDDefault>();
  m_cfg.sliceTester
      .connect<&ActsExamples::DefaultHoughFunctions::inSliceDefault>();
}

ActsExamples::ProcessCode ActsExamples::HoughTransformSeeder::execute(
    const AlgorithmContext& ctx) const {
  // clear our Hough measurements out from the previous iteration, if at all
  houghMeasurementStructs.clear();

  // add SPs to the inputs
  addSpacePoints(ctx);

  // add ACTS measurements
  addMeasurements(ctx);

  static thread_local ProtoTrackContainer protoTracks;
  protoTracks.clear();

  // loop over our subregions and run the Hough Transform on each
  for (int subregion : m_cfg.subRegions) {
    ACTS_DEBUG("Processing subregion " << subregion);
    ActsExamples::HoughHist m_houghHist = createHoughHist(subregion);

    for (unsigned y = 0; y < m_cfg.houghHistSize_y; y++) {
      for (unsigned x = 0; x < m_cfg.houghHistSize_x; x++) {
        if (passThreshold(m_houghHist, x, y)) {
          /* now we need to unpack the hits; there should be multiple track
             candidates if we have multiple hits in a given layer So the first
             thing is to unpack the indices (which is what we need) by layer */

          std::vector<std::vector<std::vector<Index>>> hitIndicesAll(
              m_cfg.nLayers);  // [layer,vector<Index]
          std::vector<std::size_t> nHitsPerLayer(m_cfg.nLayers);
          for (auto measurementIndex : m_houghHist.atLocalBins({y, x}).second) {
            HoughMeasurementStruct* meas =
                houghMeasurementStructs[measurementIndex].get();
            hitIndicesAll[meas->layer].push_back(meas->indices);
            nHitsPerLayer[meas->layer]++;
          }
          std::vector<std::vector<int>> combs = getComboIndices(nHitsPerLayer);

          for (auto [icomb, hit_indices] :
               Acts::enumerate(combs)) {  // loop over all the combination

            ProtoTrack protoTrack;
            for (unsigned layer = 0; layer < m_cfg.nLayers; layer++) {
              if (hit_indices[layer] >= 0) {
                for (auto index : hitIndicesAll[layer][hit_indices[layer]]) {
                  protoTrack.push_back(index);
                }
              }
            }
            protoTracks.push_back(protoTrack);
          }
        }
      }
    }
  }
  ACTS_DEBUG("Created " << protoTracks.size() << " proto track");

  m_outputProtoTracks(ctx, ProtoTrackContainer{protoTracks});
  // clear the vector
  houghMeasurementStructs.clear();
  return ActsExamples::ProcessCode::SUCCESS;
}

ActsExamples::HoughHist
ActsExamples::HoughTransformSeeder::createLayerHoughHist(unsigned layer,
                                                         int subregion) const {
  ActsExamples::HoughHist houghHist(
      Axis(0, m_cfg.houghHistSize_y, m_cfg.houghHistSize_y),
      Axis(0, m_cfg.houghHistSize_x, m_cfg.houghHistSize_x));
  for (unsigned index = 0; index < houghMeasurementStructs.size(); index++) {
    HoughMeasurementStruct* meas = houghMeasurementStructs[index].get();
    if (meas->layer != layer) {
      continue;
    }
    if (!(m_cfg.sliceTester(meas->z, meas->layer, subregion)).value()) {
      continue;
    }

    // This scans over y (pT) because that is more efficient in memory
    for (unsigned y_ = 0; y_ < m_cfg.houghHistSize_y; y_++) {
      unsigned y_bin_min = y_;
      unsigned y_bin_max = (y_ + 1);

      // Find the min/max x bins
      auto xBins =
          yToXBins(y_bin_min, y_bin_max, meas->radius, meas->phi, meas->layer);
      // Update the houghHist
      for (unsigned y = y_bin_min; y < y_bin_max; y++) {
        for (unsigned x = xBins.first; x < xBins.second; x++) {
          houghHist.atLocalBins({y, x}).first++;
          houghHist.atLocalBins({y, x}).second.insert(index);
        }
      }
    }
  }

  return houghHist;
}

ActsExamples::HoughHist ActsExamples::HoughTransformSeeder::createHoughHist(
    int subregion) const {
  ActsExamples::HoughHist houghHist(
      Axis(0, m_cfg.houghHistSize_y, m_cfg.houghHistSize_y),
      Axis(0, m_cfg.houghHistSize_x, m_cfg.houghHistSize_x));

  for (unsigned i = 0; i < m_cfg.nLayers; i++) {
    HoughHist layerHoughHist = createLayerHoughHist(i, subregion);
    for (unsigned x = 0; x < m_cfg.houghHistSize_x; ++x) {
      for (unsigned y = 0; y < m_cfg.houghHistSize_y; ++y) {
        if (layerHoughHist.atLocalBins({y, x}).first > 0) {
          houghHist.atLocalBins({y, x}).first++;
          houghHist.atLocalBins({y, x}).second.insert(
              layerHoughHist.atLocalBins({y, x}).second.begin(),
              layerHoughHist.atLocalBins({y, x}).second.end());
        }
      }
    }
  }

  return houghHist;
}

bool ActsExamples::HoughTransformSeeder::passThreshold(
    HoughHist const& houghHist, unsigned x, unsigned y) const {
  // Pass window threshold
  unsigned width = m_cfg.threshold.size() / 2;
  if (x < width || m_cfg.houghHistSize_x - x < width) {
    return false;
  }
  for (unsigned i = 0; i < m_cfg.threshold.size(); i++) {
    if (houghHist.atLocalBins({y, x - width + i}).first < m_cfg.threshold[i]) {
      return false;
    }
  }

  // Pass local-maximum check, if used
  if (m_cfg.localMaxWindowSize != 0) {
    for (int j = -m_cfg.localMaxWindowSize; j <= m_cfg.localMaxWindowSize;
         j++) {
      for (int i = -m_cfg.localMaxWindowSize; i <= m_cfg.localMaxWindowSize;
           i++) {
        if (i == 0 && j == 0) {
          continue;
        }
        if (y + j < m_cfg.houghHistSize_y && x + i < m_cfg.houghHistSize_x) {
          if (houghHist.atLocalBins({y + j, x + i}).first >
              houghHist.atLocalBins({y, x}).first) {
            return false;
          }
          if (houghHist.atLocalBins({y + j, x + i}).first ==
              houghHist.atLocalBins({y, x}).first) {
            if (houghHist.atLocalBins({y + j, x + i}).second.size() >
                houghHist.atLocalBins({y, x}).second.size()) {
              return false;
            }
            if (houghHist.atLocalBins({y + j, x + i}).second.size() ==
                    houghHist.atLocalBins({y, x}).second.size() &&
                j <= 0 && i <= 0) {
              return false;  // favor bottom-left (low phi, low neg q/pt)
            }
          }
        }
      }
    }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Helpers

// Quantizes val, given a range [min, max) split into nSteps. Returns the bin
// below.
static inline int quant(double min, double max, unsigned nSteps, double val) {
  return static_cast<int>((val - min) / (max - min) * nSteps);
}

// Returns the lower bound of the bin specified by step
static inline double unquant(double min, double max, unsigned nSteps,
                             int step) {
  return min + (max - min) * step / nSteps;
}

template <typename T>
static inline std::string to_string(std::vector<T> v) {
  std::ostringstream oss;
  oss << "[";
  if (!v.empty()) {
    std::copy(v.begin(), v.end() - 1, std::ostream_iterator<T>(oss, ", "));
    oss << v.back();
  }
  oss << "]";
  return oss.str();
}

double ActsExamples::HoughTransformSeeder::yToX(double y, double r,
                                                double phi) const {
  double d0 = 0;  // d0 correction TO DO allow for this
  double x =
      asin(r * ActsExamples::HoughTransformSeeder::m_cfg.kA * y - d0 / r) + phi;

  if (m_cfg.fieldCorrector.connected()) {
    x += (m_cfg.fieldCorrector(0, y, r)).value();
  }

  return x;
}

// Find the min/max x bins of the hit's line, in each y bin. Max is exclusive.
// Note this assumes yToX is monotonic. Returns {0, 0} if hit lies out of
// bounds.
std::pair<unsigned, unsigned> ActsExamples::HoughTransformSeeder::yToXBins(
    std::size_t yBin_min, std::size_t yBin_max, double r, double phi,
    unsigned layer) const {
  double x_min = yToX(m_bins_y[yBin_min], r, phi);
  double x_max = yToX(m_bins_y[yBin_max], r, phi);
  if (x_min > x_max) {
    std::swap(x_min, x_max);
  }
  if (x_max < m_cfg.xMin || x_min > m_cfg.xMax) {
    return {0, 0};  // out of bounds
  }

  // Get bins
  int x_bin_min = quant(m_cfg.xMin, m_cfg.xMax, m_cfg.houghHistSize_x, x_min);
  int x_bin_max = quant(m_cfg.xMin, m_cfg.xMax, m_cfg.houghHistSize_x, x_max) +
                  1;  // exclusive

  // Extend bins
  unsigned extend = getExtension(yBin_min, layer);
  x_bin_min -= extend;
  x_bin_max += extend;

  // Clamp bins
  if (x_bin_min < 0) {
    x_bin_min = 0;
  }
  if (x_bin_max > static_cast<int>(m_cfg.houghHistSize_x)) {
    x_bin_max = m_cfg.houghHistSize_x;
  }

  return {x_bin_min, x_bin_max};
}

// We allow variable extension based on the size of m_hitExtend_x. See comments
// below.
unsigned ActsExamples::HoughTransformSeeder::getExtension(
    unsigned y, unsigned layer) const {
  if (m_cfg.hitExtend_x.size() == m_cfg.nLayers) {
    return m_cfg.hitExtend_x[layer];
  }

  if (m_cfg.hitExtend_x.size() == m_cfg.nLayers * 2) {
    // different extension for low pt vs high pt, split in half but irrespective
    // of sign first nLayers entries of m_hitExtend_x is for low pt half, rest
    // are for high pt half
    if (y < m_cfg.houghHistSize_y / 4 || y > 3 * m_cfg.houghHistSize_y / 4) {
      return m_cfg.hitExtend_x[layer];
    }

    return m_cfg.hitExtend_x[m_cfg.nLayers + layer];
  }
  return 0;
}

/**
 * Given a list of sizes (of arrays), generates a list of all combinations of
 * indices to index one element from each array.
 *
 * For example, given [2 3], generates [(0 0) (1 0) (0 1) (1 1) (0 2) (1 2)].
 *
 * This basically amounts to a positional number system of where each digit has
 * its own base. The number of digits is sizes.size(), and the base of digit i
 * is sizes[i]. Then all combinations can be uniquely represented just by
 * counting from [0, nCombs).
 *
 * For a decimal number like 1357, you get the thousands digit with n / 1000 = n
 * / (10 * 10 * 10). So here, you get the 0th digit with n / (base_1 * base_2 *
 * base_3);
 */
std::vector<std::vector<int>>
ActsExamples::HoughTransformSeeder::getComboIndices(
    std::vector<std::size_t>& sizes) const {
  std::size_t nCombs = 1;
  std::vector<std::size_t> nCombs_prior(sizes.size());
  std::vector<int> temp(sizes.size(), 0);

  for (std::size_t i = 0; i < sizes.size(); i++) {
    if (sizes[i] > 0) {
      nCombs_prior[i] = nCombs;
      nCombs *= sizes[i];
    } else {
      temp[i] = -1;
    }
  }

  std::vector<std::vector<int>> combos(nCombs, temp);

  for (std::size_t icomb = 0; icomb < nCombs; icomb++) {
    std::size_t index = icomb;
    for (std::size_t isize = sizes.size() - 1; isize < sizes.size(); isize--) {
      if (sizes[isize] == 0) {
        continue;
      }
      combos[icomb][isize] = static_cast<int>(index / nCombs_prior[isize]);
      index = index % nCombs_prior[isize];
    }
  }

  return combos;
}

void ActsExamples::HoughTransformSeeder::addSpacePoints(
    const AlgorithmContext& ctx) const {
  // construct the combined input container of space point pointers from all
  // configured input sources.
  for (const auto& isp : m_inputSpacePoints) {
    const auto& spContainer = (*isp)(ctx);
    ACTS_DEBUG("Inserting " << spContainer.size() << " space points from "
                            << isp->key());
    for (auto& sp : spContainer) {
      double r = std::hypot(sp.x(), sp.y());
      double z = sp.z();
      float phi = std::atan2(sp.y(), sp.x());
      ResultUnsigned hitlayer = m_cfg.layerIDFinder(r).value();
      if (!(hitlayer.ok())) {
        continue;
      }
      std::vector<Index> indices;
      for (const auto& slink : sp.sourceLinks()) {
        const auto& islink = slink.get<IndexSourceLink>();
        indices.push_back(islink.index());
      }

      auto meas =
          std::shared_ptr<HoughMeasurementStruct>(new HoughMeasurementStruct(
              hitlayer.value(), phi, r, z, indices, HoughHitType::SP));
      houghMeasurementStructs.push_back(meas);
    }
  }
}

void ActsExamples::HoughTransformSeeder::addMeasurements(
    const AlgorithmContext& ctx) const {
  const auto& measurements = m_inputMeasurements(ctx);
  const auto& sourceLinks = m_inputSourceLinks(ctx);

  ACTS_DEBUG("Inserting " << measurements.size() << " space points from "
                          << m_cfg.inputMeasurements);

  for (Acts::GeometryIdentifier geoId : m_cfg.geometrySelection) {
    // select volume/layer depending on what is set in the geometry id
    auto range = selectLowestNonZeroGeometryObject(sourceLinks, geoId);
    // groupByModule only works with geometry containers, not with an
    // arbitrary range. do the equivalent grouping manually
    auto groupedByModule = makeGroupBy(range, detail::GeometryIdGetter());

    for (const auto& [moduleGeoId, moduleSourceLinks] : groupedByModule) {
      // find corresponding surface
      const Acts::Surface* surface =
          m_cfg.trackingGeometry->findSurface(moduleGeoId);
      if (surface == nullptr) {
        ACTS_ERROR("Could not find surface " << moduleGeoId);
        return;
      }

      for (auto& sourceLink : moduleSourceLinks) {
        // extract a local position/covariance independent of the concrete
        // measurement content. since we do not know if and where the local
        // parameters are contained in the measurement parameters vector, they
        // are transformed to the bound space where we do know their location.
        // if the local parameters are not measured, this results in a
        // zero location, which is a reasonable default fall-back.
        const ConstVariableBoundMeasurementProxy measurement =
            measurements.getMeasurement(sourceLink.index());

        assert(measurement.contains(Acts::eBoundLoc0) &&
               "Measurement does not contain the required bound loc0");
        assert(measurement.contains(Acts::eBoundLoc1) &&
               "Measurement does not contain the required bound loc1");

        auto boundLoc0 = measurement.indexOf(Acts::eBoundLoc0);
        auto boundLoc1 = measurement.indexOf(Acts::eBoundLoc1);

        Acts::Vector2 localPos{measurement.parameters()[boundLoc0],
                               measurement.parameters()[boundLoc1]};

        // transform local position to global coordinates
        Acts::Vector3 globalFakeMom(1, 1, 1);
        Acts::Vector3 globalPos =
            surface->localToGlobal(ctx.geoContext, localPos, globalFakeMom);
        double r = std::hypot(globalPos[Acts::ePos0], globalPos[Acts::ePos1]);
        double phi = std::atan2(globalPos[Acts::ePos1], globalPos[Acts::ePos0]);
        double z = globalPos[Acts::ePos2];
        ResultUnsigned hitlayer = m_cfg.layerIDFinder(r);
        if (hitlayer.ok()) {
          std::vector<Index> index;
          index.push_back(sourceLink.index());
          auto houghMeas = std::shared_ptr<HoughMeasurementStruct>(
              new HoughMeasurementStruct(hitlayer.value(), phi, r, z, index,
                                         HoughHitType::MEASUREMENT));
          houghMeasurementStructs.push_back(houghMeas);
        }
      }
    }
  }
}
