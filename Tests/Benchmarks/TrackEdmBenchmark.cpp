// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/EventData/SubspaceHelpers.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/TrackStateType.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/EventData/detail/GenerateParameters.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Utilities/TrackHelpers.hpp"

#include <iostream>
#include <numeric>
#include <random>
#include <type_traits>

using namespace Acts;

class BenchmarkSourceLink final {
 public:
  using Index = std::uint32_t;

  /// Construct from geometry identifier and index.
  constexpr BenchmarkSourceLink(Acts::GeometryIdentifier gid, Index idx)
      : m_geometryId(gid), m_index(idx) {}

  BenchmarkSourceLink() = default;
  BenchmarkSourceLink(const BenchmarkSourceLink&) = default;
  BenchmarkSourceLink(BenchmarkSourceLink&&) = default;
  BenchmarkSourceLink& operator=(const BenchmarkSourceLink&) = default;
  BenchmarkSourceLink& operator=(BenchmarkSourceLink&&) = default;

  /// Access the index.
  constexpr Index index() const { return m_index; }

  Acts::GeometryIdentifier geometryId() const { return m_geometryId; }

 private:
  Acts::GeometryIdentifier m_geometryId;
  Index m_index = 0;

  friend bool operator==(const BenchmarkSourceLink& lhs,
                         const BenchmarkSourceLink& rhs) {
    return (lhs.geometryId() == rhs.geometryId()) &&
           (lhs.m_index == rhs.m_index);
  }
};

int main(int /*argc*/, char** /*argv[]*/) {
  std::size_t runs = 1000000;
  std::size_t nTracks = 10000;

  VectorMultiTrajectory mtj;
  VectorTrackContainer vtc;
  TrackContainer tc{vtc, mtj};

  VectorMultiTrajectory mtjOut;
  VectorTrackContainer vtcOut;
  TrackContainer output{vtcOut, mtjOut};

  GeometryIdentifier gid;
  gid.setVolume(5);
  gid.setLayer(3);
  gid.setSensitive(1);

  static_assert(sizeof(BenchmarkSourceLink) <= ACTS_SOURCELINK_SBO_SIZE);

  static_assert(std::is_trivially_move_constructible_v<BenchmarkSourceLink>);

  std::uniform_int_distribution<> nStatesDist(1, 20);
  std::uniform_int_distribution<> measDimDist(1, 3);
  std::uniform_real_distribution<> typeDist(0, 1);
  std::uniform_real_distribution<> copyDist(0, 1);
  std::mt19937 rng{42};

  std::vector<std::shared_ptr<Surface>> surfaces;
  std::vector<std::pair<BoundVector, BoundMatrix>> parametersVector;
  for (std::size_t s = 0; s < 50; ++s) {
    surfaces.push_back(Surface::makeShared<PlaneSurface>(
        Transform3::Identity(), std::make_shared<RectangleBounds>(50, 50)));

    parametersVector.push_back(
        detail::Test::generateBoundParametersCovariance(rng, {}));
  }

  std::size_t nSurface = 0;
  auto surface = [&]() {
    nSurface++;
    return surfaces.at(nSurface % surfaces.size());
  };

  std::size_t nParams = 0;
  auto parameters = [&]() -> const std::pair<BoundVector, BoundMatrix>& {
    nParams++;
    return parametersVector.at(nParams % parametersVector.size());
  };

  auto perigee = Surface::makeShared<PerigeeSurface>(Vector3::Zero());

  std::cout << "Creating " << nTracks << " tracks x " << runs << " runs"
            << std::endl;
  for (std::size_t r = 0; r < runs; ++r) {
    tc.clear();
    output.clear();

    for (std::size_t i = 0; i < nTracks; ++i) {
      auto track = tc.makeTrack();

      std::size_t nStates = nStatesDist(rng);

      for (std::size_t j = 0; j < nStates; ++j) {
        auto trackState = track.appendTrackState(TrackStatePropMask::All);
        trackState.setReferenceSurface(surface());

        trackState.jacobian().setZero();
        trackState.jacobian().row(j % eBoundSize).setOnes();

        double crit = typeDist(rng);

        if (crit < 0.1) {
          // hole
          trackState.typeFlags().set(TrackStateFlag::HoleFlag);
        } else if (crit < 0.2) {
          // material
          trackState.typeFlags().set(TrackStateFlag::MaterialFlag);
        } else {
          BenchmarkSourceLink bsl{gid, 123};
          std::size_t measdim = measDimDist(rng);

          const auto& [predicted, covariance] = parameters();
          trackState.predicted() = predicted;
          trackState.predictedCovariance() = covariance;

          visit_measurement(
              measdim,
              [&]<std::size_t N>(std::integral_constant<std::size_t, N> /*d*/) {
                trackState.allocateCalibrated(ActsVector<N>::Ones(),
                                              ActsSquareMatrix<N>::Identity());

                std::array<std::uint8_t, eBoundSize> indices{0};
                std::iota(indices.begin(), indices.end(), 0);
                trackState.setProjectorSubspaceIndices(indices);
              });

          trackState.typeFlags().set(TrackStateFlag::MeasurementFlag);
          if (crit < 0.4) {
            // outlier
            trackState.typeFlags().set(TrackStateFlag::OutlierFlag);
          }
        }
      }

      track.setReferenceSurface(perigee);

      const auto& [ref, cov] = parameters();
      track.parameters() = ref;
      track.covariance() = cov;

      track.linkForward();

      calculateTrackQuantities(track);
    }

    for (const auto& track : tc) {
      if (copyDist(rng) > 0.1) {
        // copy only 10% of tracks
        continue;
      }

      auto target = output.makeTrack();
      target.copyFrom(track);
    }
  }

  return 0;
}
