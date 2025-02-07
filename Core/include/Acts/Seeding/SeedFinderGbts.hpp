// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

// TODO: update to C++17 style

#include "Acts/EventData/Seed.hpp"
#include "Acts/EventData/SpacePointData.hpp"
#include "Acts/Seeding/SeedFinderConfig.hpp"
#include "Acts/Seeding/SeedFinderGbtsConfig.hpp"
#include "Acts/TrackFinding/RoiDescriptor.hpp"
#include "Acts/Utilities/Logger.hpp"

#include <array>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Acts::Experimental {

template <typename external_spacepoint_t>
struct GbtsTrigTracklet {
 public:
  GbtsTrigTracklet(std::vector<const GbtsSP<external_spacepoint_t> *> &vSP,
                   std::vector<TrigInDetTriplet<external_spacepoint_t>> &tbuf)
      : m_track(vSP), m_seeds(tbuf) {}

  std::vector<const GbtsSP<external_spacepoint_t> *> m_track;
  std::vector<TrigInDetTriplet<external_spacepoint_t>> m_seeds;
};

template <typename external_spacepoint_t>
class SeedFinderGbts {
 public:
  static constexpr std::size_t NDims = 3;

  using seed_t = Seed<external_spacepoint_t>;

  // constructors
  SeedFinderGbts(const SeedFinderGbtsConfig<external_spacepoint_t> &config,
                 const GbtsGeometry<external_spacepoint_t> &gbtsgeo,
                 std::unique_ptr<const Acts::Logger> logger =
                     Acts::getDefaultLogger("Finder",
                                            Acts::Logging::Level::INFO));

  ~SeedFinderGbts() = default;
  SeedFinderGbts() = default;
  SeedFinderGbts(const SeedFinderGbts<external_spacepoint_t> &) = delete;
  SeedFinderGbts<external_spacepoint_t> &operator=(
      const SeedFinderGbts<external_spacepoint_t> &) = delete;

  void loadSpacePoints(
      const std::vector<GbtsSP<external_spacepoint_t>> &gbtsSPvect);

  // inner
  template <typename output_container_t>
  void createSeeds(const RoiDescriptor & /*roi*/,
                   const GbtsGeometry<external_spacepoint_t> & /*gbtsgeo*/,
                   output_container_t & /*out_cont*/);
  // outer
  std::vector<seed_t> createSeeds(
      const RoiDescriptor & /*roi*/,
      const GbtsGeometry<external_spacepoint_t> & /*gbtsgeo*/);

 private:
  enum Dim { DimPhi = 0, DimR = 1, DimZ = 2 };

  // config object
  SeedFinderGbtsConfig<external_spacepoint_t> m_config;

  void runGbts_TrackFinder(
      std::vector<GbtsTrigTracklet<external_spacepoint_t>> &vTracks,
      const RoiDescriptor &roi,
      const GbtsGeometry<external_spacepoint_t> &gbtsgeo);

  // needs to be member of class so can accessed by all member functions
  std::unique_ptr<GbtsDataStorage<external_spacepoint_t>> m_storage{nullptr};

  // for create seeds:
  std::vector<TrigInDetTriplet<external_spacepoint_t>> m_triplets;

  const Acts::Logger &logger() const { return *m_logger; }
  std::unique_ptr<const Acts::Logger> m_logger =
      Acts::getDefaultLogger("Finder", Acts::Logging::Level::INFO);
};

}  // namespace Acts::Experimental

#include "Acts/Seeding/SeedFinderGbts.ipp"
