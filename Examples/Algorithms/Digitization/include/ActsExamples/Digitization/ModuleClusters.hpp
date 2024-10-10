// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Clusterization/Clusterization.hpp"
#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "ActsExamples/Digitization/MeasurementCreation.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/SimHit.hpp"

#include <algorithm>
#include <cstddef>
#include <set>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace ActsExamples {
struct DigitizedParameters;

struct ModuleValue {
  std::vector<Acts::BoundIndices> paramIndices = {};
  std::vector<Acts::ActsScalar> paramValues = {};
  std::vector<Acts::ActsScalar> paramVariances = {};
  std::variant<Cluster, Cluster::Cell> value;
  std::set<SimHitContainer::size_type> sources = {};
  Acts::Ccl::Label label = {Acts::Ccl::NO_LABEL};
};

class ModuleClusters {
 public:
  using simhit_t = SimHitContainer::size_type;

  ModuleClusters(Acts::BinUtility segmentation,
                 std::vector<Acts::BoundIndices> geoIndices, bool merge,
                 double nsigma, bool commonCorner)
      : m_segmentation(std::move(segmentation)),
        m_geoIndices(std::move(geoIndices)),
        m_merge(merge),
        m_nsigma(nsigma),
        m_commonCorner(commonCorner) {}

  void add(DigitizedParameters params, simhit_t simhit);
  std::vector<std::pair<DigitizedParameters, std::set<simhit_t>>>
  digitizedParameters();

 private:
  Acts::BinUtility m_segmentation;
  std::vector<Acts::BoundIndices> m_geoIndices;
  std::vector<ModuleValue> m_moduleValues;
  bool m_merge;
  double m_nsigma;
  bool m_commonCorner;

  std::vector<ModuleValue> createCellCollection();
  void merge();
  ModuleValue squash(std::vector<ModuleValue>& values);
  std::vector<std::size_t> nonGeoEntries(
      std::vector<Acts::BoundIndices>& indices);
  std::vector<std::vector<ModuleValue>> mergeParameters(
      std::vector<ModuleValue> values);
};
}  // namespace ActsExamples
