// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Seeding/AtlasSeedFinder.hpp"

#include <algorithm>

// space point structure with the bare minimum and reasonable default
// covariances. clusterList default is SCT (strip detector)
struct SpacePoint {
  float x = 0;
  float y = 0;
  float z = 0;
  float r = 0;
  float covr = 0.03;
  float covz = 0.03;
  std::pair<int, int> m_clusterList = std::pair<int, int>(1, 1);
  void setClusterList(int first, int second) {
    m_clusterList = std::pair<int, int>(first, second);
  }
  const std::pair<int, int> clusterList() const { return m_clusterList; }
  int surface = 0;
};

// call sequence to create seeds. Seeds are copied as the
// call to next() overwrites the previous seed object
std::vector<Acts::Legacy::Seed<SpacePoint>> runSeeding(
    std::vector<SpacePoint*> spVec) {
  Acts::Legacy::AtlasSeedFinder<SpacePoint> seedMaker;
  seedMaker.newEvent(0, spVec.begin(), spVec.end());
  seedMaker.find3Sp();
  const Acts::Legacy::Seed<SpacePoint>* seed = seedMaker.next();
  std::vector<Acts::Legacy::Seed<SpacePoint>> seedVec;
  while (seed != nullptr) {
    auto spIter = seed->spacePoints().begin();
    spIter++;
    spIter++;
    seedVec.push_back(*seed);
    seed = seedMaker.next();
  }
  return seedVec;
}

// used to sort seeds, ignores z
class seedComparator {
 public:
  bool operator()(const Acts::Legacy::Seed<SpacePoint>& s1,
                  const Acts::Legacy::Seed<SpacePoint>& s2) {
    auto sp1It = s1.spacePoints().begin();
    auto sp2It = s2.spacePoints().begin();
    for (int i = 0; i < 3; i++) {
      if ((*sp1It) != (*sp2It)) {
        return (*sp1It) < (*sp2It);
      }
      sp1It++;
      sp2It++;
    }
    return false;
  }
};

BOOST_AUTO_TEST_CASE(number_of_seeds_correct_) {
  // single particle event with 405MeV (just above default pT-cut)
  std::vector<SpacePoint*> spVec;
  std::vector<int> layerVec{1, 2, 2, 3, 4, 11, 13, 14};
  // clang-format off
  std::vector<float> xVec{-33.3403,
                          -48.2369,
                          -49.4129,
                          -88.8567,
                          -122.5566,
                          -283.169,
                          -412.277,
                          -462.5564};

  std::vector<float> yVec{2.7288,
                          4.5193,
                          4.6755,
                          11.1935,
                          18.7696,
                          83.1666,
                          179.1006,
                          232.9765};

  std::vector<float> zVec{-74.5553,
                          -91.9763,
                          -93.3541,
                          -139.779,
                          -179.889,
                          -381.403,
                          -568.641,
                          -654.2494};
  // clang-format on

  // creating space points and setting clusterList to pixel for
  // the detector region of the pixel detector
  for (unsigned int i = 0; i < layerVec.size(); i++) {
    SpacePoint* sp = new SpacePoint();
    sp->surface = layerVec.at(i);
    sp->x = xVec.at(i);
    sp->y = yVec.at(i);
    sp->z = zVec.at(i);
    sp->r = std::hypot(sp->x, sp->y);
    if (sp->r < 200.) {
      sp->setClusterList(1, 0);
    }
    spVec.push_back(sp);
  }
  // create seeds (without z component) that are found by the ATLAS seed finder
  // as reference
  Acts::Legacy::Seed<SpacePoint> s1(spVec.at(0), spVec.at(1), spVec.at(3), 0);
  Acts::Legacy::Seed<SpacePoint> s2(spVec.at(0), spVec.at(1), spVec.at(4), 0);
  Acts::Legacy::Seed<SpacePoint> s3(spVec.at(0), spVec.at(2), spVec.at(3), 0);
  Acts::Legacy::Seed<SpacePoint> s4(spVec.at(0), spVec.at(2), spVec.at(4), 0);
  Acts::Legacy::Seed<SpacePoint> s5(spVec.at(0), spVec.at(3), spVec.at(4), 0);
  Acts::Legacy::Seed<SpacePoint> s6(spVec.at(1), spVec.at(3), spVec.at(4), 0);
  Acts::Legacy::Seed<SpacePoint> s7(spVec.at(2), spVec.at(3), spVec.at(4), 0);
  Acts::Legacy::Seed<SpacePoint> s8(spVec.at(5), spVec.at(6), spVec.at(7), 0);
  std::vector<Acts::Legacy::Seed<SpacePoint>> refVec;
  refVec.push_back(s1);
  refVec.push_back(s2);
  refVec.push_back(s3);
  refVec.push_back(s4);
  refVec.push_back(s5);
  refVec.push_back(s6);
  refVec.push_back(s7);
  refVec.push_back(s8);

  auto seedVec = runSeeding(spVec);

  // sorting required for set_difference call. sorting assumes space points
  // inside seed are already sorted.
  std::ranges::sort(refVec, seedComparator());
  std::ranges::sort(seedVec, seedComparator());

  // difference between reference and result shows if results exactly the same
  // (i.e. difference is 0)
  std::vector<Acts::Legacy::Seed<SpacePoint>> diff;
  std::set_difference(refVec.begin(), refVec.end(), seedVec.begin(),
                      seedVec.end(), std::inserter(diff, diff.begin()),
                      seedComparator());
  BOOST_CHECK(diff.empty());
  for (auto sp : spVec) {
    delete sp;
  }
}
