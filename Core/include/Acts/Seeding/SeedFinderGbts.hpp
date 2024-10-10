// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// TODO: update to C++17 style

#include "Acts/EventData/Seed.hpp"
#include "Acts/EventData/SpacePointData.hpp"
#include "Acts/Seeding/SeedFinderConfig.hpp"
#include "Acts/Seeding/SeedFinderGbtsConfig.hpp"
#include "Acts/TrackFinding/RoiDescriptor.hpp"
#include "Acts/Utilities/KDTree.hpp"

#include <array>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace Acts {

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
  //   using internal_sp_t = InternalSpacePoint<external_spacepoint_t>;
  //   using tree_t = KDTree<NDims, internal_sp_t *, ActsScalar, std::array, 4>;

  // constructors
  SeedFinderGbts(const SeedFinderGbtsConfig<external_spacepoint_t> &config,
                 const GbtsGeometry<external_spacepoint_t> &gbtsgeo);

  ~SeedFinderGbts();  //!!! is it dangerous not to use default? got def in ipp
  SeedFinderGbts() = default;
  SeedFinderGbts(const SeedFinderGbts<external_spacepoint_t> &) = delete;
  SeedFinderGbts<external_spacepoint_t> &operator=(
      const SeedFinderGbts<external_spacepoint_t> &) = delete;

  void loadSpacePoints(
      const std::vector<GbtsSP<external_spacepoint_t>> &gbtsSPvect);

  // inner
  template <typename output_container_t>
  void createSeeds(
      const Acts::RoiDescriptor & /*roi*/,
      const Acts::GbtsGeometry<external_spacepoint_t> & /*gbtsgeo*/,
      output_container_t & /*out_cont*/);
  // outer
  std::vector<seed_t> createSeeds(
      const Acts::RoiDescriptor & /*roi*/,
      const Acts::GbtsGeometry<external_spacepoint_t> & /*gbtsgeo*/);

 private:
  enum Dim { DimPhi = 0, DimR = 1, DimZ = 2 };

  // config object
  SeedFinderGbtsConfig<external_spacepoint_t> m_config;

  void runGbts_TrackFinder(
      std::vector<GbtsTrigTracklet<external_spacepoint_t>> &vTracks,
      const Acts::RoiDescriptor &roi,
      const Acts::GbtsGeometry<external_spacepoint_t> &gbtsgeo);

  // needs to be member of class so can accessed by all member functions
  GbtsDataStorage<external_spacepoint_t> *m_storage;

  // for create seeds:
  std::vector<TrigInDetTriplet<external_spacepoint_t>> m_triplets;
};

}  // namespace Acts

#include "Acts/Seeding/SeedFinderGbts.ipp"
