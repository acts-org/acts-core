// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Digitization/ModuleClusters.hpp"

#include "Acts/Clusterization/Clusterization.hpp"
#include "Acts/Utilities/Helpers.hpp"
#include "ActsExamples/Digitization/MeasurementCreation.hpp"
#include "ActsFatras/Digitization/Channelizer.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>

namespace ActsExamples {

void ModuleClusters::add(DigitizedParameters params, simhit_t simhit) {
  ModuleValue mval;
  mval.paramIndices = std::move(params.indices);
  mval.paramValues = std::move(params.values);
  mval.paramVariances = std::move(params.variances);
  mval.sources = {simhit};

  if (m_merge && !params.cluster.channels.empty()) {
    // Break-up the cluster
    for (const auto& cell : params.cluster.channels) {
      ModuleValue mval_cell = mval;
      mval_cell.value = cell;
      m_moduleValues.push_back(std::move(mval_cell));
    }
  } else {
    // pass-through mode or smeared indices only
    mval.value = std::move(params.cluster);
    m_moduleValues.push_back(std::move(mval));
  }
}

std::vector<std::pair<DigitizedParameters, std::set<ModuleClusters::simhit_t>>>
ModuleClusters::digitizedParameters() {
  if (m_merge) {  // (re-)build the clusters
    merge();
  }
  std::vector<std::pair<DigitizedParameters, std::set<simhit_t>>> retv;
  for (ModuleValue& mval : m_moduleValues) {
    if (std::holds_alternative<Cluster::Cell>(mval.value)) {
      // Should never happen! Either the cluster should have
      // passed-through or the cells merged into clusters in the
      // merge() step. Treat this as a bug.
      throw std::runtime_error("Invalid cluster!");
    }
    DigitizedParameters dpars;
    dpars.indices = mval.paramIndices;
    dpars.values = mval.paramValues;
    dpars.variances = mval.paramVariances;
    dpars.cluster = std::get<Cluster>(mval.value);
    retv.emplace_back(std::move(dpars), mval.sources);
  }
  return retv;
}

// Needed for clusterization
int getCellRow(const ModuleValue& mval) {
  if (std::holds_alternative<ActsExamples::Cluster::Cell>(mval.value)) {
    return std::get<ActsExamples::Cluster::Cell>(mval.value).bin[0];
  }
  throw std::domain_error("ModuleValue does not contain cell!");
}

int getCellColumn(const ActsExamples::ModuleValue& mval) {
  if (std::holds_alternative<ActsExamples::Cluster::Cell>(mval.value)) {
    return std::get<ActsExamples::Cluster::Cell>(mval.value).bin[1];
  }
  throw std::domain_error("ModuleValue does not contain cell!");
}

int& getCellLabel(ActsExamples::ModuleValue& mval) {
  return mval.label;
}

void clusterAddCell(std::vector<ModuleValue>& cl, const ModuleValue& ce) {
  cl.push_back(ce);
}

std::vector<ModuleValue> ModuleClusters::createCellCollection() {
  std::vector<ModuleValue> cells;
  for (ModuleValue& mval : m_moduleValues) {
    if (std::holds_alternative<Cluster::Cell>(mval.value)) {
      cells.push_back(mval);
    }
  }
  return cells;
}

void ModuleClusters::merge() {
  std::vector<ModuleValue> cells = createCellCollection();

  std::vector<ModuleValue> newVals;

  if (!cells.empty()) {
    // Case where we actually have geometric clusters
    std::vector<std::vector<ModuleValue>> merged =
        Acts::Ccl::createClusters<std::vector<ModuleValue>,
                                  std::vector<std::vector<ModuleValue>>>(
            cells, Acts::Ccl::DefaultConnect<ModuleValue>(m_commonCorner));

    for (std::vector<ModuleValue>& cellv : merged) {
      // At this stage, the cellv vector contains cells that form a
      // consistent cluster based on a connected component analysis
      // only. Still have to check if they match based on the other
      // indices (a good example of this would a for a timing
      // detector).

      for (std::vector<ModuleValue>& remerged : mergeParameters(cellv)) {
        newVals.push_back(squash(remerged));
      }
    }
    m_moduleValues = std::move(newVals);
  } else {
    // no geo clusters
    for (std::vector<ModuleValue>& merged : mergeParameters(m_moduleValues)) {
      newVals.push_back(squash(merged));
    }
    m_moduleValues = std::move(newVals);
  }
}

// ATTN: returns vector of index into `indices'
std::vector<std::size_t> ModuleClusters::nonGeoEntries(
    std::vector<Acts::BoundIndices>& indices) {
  std::vector<std::size_t> retv;
  for (std::size_t i = 0; i < indices.size(); i++) {
    auto idx = indices.at(i);
    if (!rangeContainsValue(m_geoIndices, idx)) {
      retv.push_back(i);
    }
  }
  return retv;
}

// Merging based on parameters
std::vector<std::vector<ModuleValue>> ModuleClusters::mergeParameters(
    std::vector<ModuleValue> values) {
  std::vector<std::vector<ModuleValue>> retv;

  std::vector<bool> used(values.size(), false);
  for (std::size_t i = 0; i < values.size(); i++) {
    if (used.at(i)) {
      continue;
    }

    retv.emplace_back();
    std::vector<ModuleValue>& thisvec = retv.back();

    // Value has not yet been claimed, so claim it
    thisvec.push_back(std::move(values.at(i)));
    used.at(i) = true;

    // Values previously visited by index `i' have already been added
    // to a cluster or used to seed a new cluster, so start at the
    // next unseen one
    for (std::size_t j = i + 1; j < values.size(); j++) {
      // Still may have already been used, so check it
      if (used.at(j)) {
        continue;
      }

      // Now look for a match between current cluster and value `j' Consider
      // them matched until we find evidence to the contrary. This
      // way, merging still works when digitization is done by
      // clusters only
      bool matched = true;

      // The cluster to be merged into can have more than one
      // associated value at this point, so we have to consider them
      // all
      for (ModuleValue& thisval : thisvec) {
        // Loop over non-geometric dimensions
        for (auto k : nonGeoEntries(thisval.paramIndices)) {
          Acts::ActsScalar p_i = thisval.paramValues.at(k);
          Acts::ActsScalar p_j = values.at(j).paramValues.at(k);
          Acts::ActsScalar v_i = thisval.paramVariances.at(k);
          Acts::ActsScalar v_j = values.at(j).paramVariances.at(k);

          Acts::ActsScalar left = 0, right = 0;
          if (p_i < p_j) {
            left = p_i + m_nsigma * std::sqrt(v_i);
            right = p_j - m_nsigma * std::sqrt(v_j);
          } else {
            left = p_j + m_nsigma * std::sqrt(v_j);
            right = p_i - m_nsigma * std::sqrt(v_i);
          }
          if (left < right) {
            // We know these two don't match, so break out of the
            // dimension loop
            matched = false;
            break;
          }
        }  // Loop over `k' (non-geo dimensions)
        if (matched) {
          // The value under consideration matched at least one
          // associated to the current cluster so no need to keep
          // checking others in current cluster
          break;
        }
      }  // Loop on current cluster
      if (matched) {
        // Claim value `j'
        used.at(j) = true;
        thisvec.push_back(std::move(values.at(j)));
      }
    }  // Loop on `j'
  }  // Loop on `i'
  return retv;
}

ModuleValue ModuleClusters::squash(std::vector<ModuleValue>& values) {
  ModuleValue mval;
  Acts::ActsScalar tot = 0;
  Acts::ActsScalar tot2 = 0;
  std::vector<Acts::ActsScalar> weights;

  // First, start by computing cell weights
  for (ModuleValue& other : values) {
    if (std::holds_alternative<Cluster::Cell>(other.value)) {
      weights.push_back(std::get<Cluster::Cell>(other.value).activation);
    } else {
      weights.push_back(1);
    }
    tot += weights.back();
    tot2 += weights.back() * weights.back();
  }

  // Now, go over the non-geometric indices
  for (std::size_t i = 0; i < values.size(); i++) {
    ModuleValue& other = values.at(i);
    for (std::size_t j = 0; j < other.paramIndices.size(); j++) {
      auto idx = other.paramIndices.at(j);
      if (!rangeContainsValue(m_geoIndices, idx)) {
        if (!rangeContainsValue(mval.paramIndices, idx)) {
          mval.paramIndices.push_back(idx);
        }
        if (mval.paramValues.size() < (j + 1)) {
          mval.paramValues.push_back(0);
          mval.paramVariances.push_back(0);
        }
        Acts::ActsScalar f = weights.at(i) / (tot > 0 ? tot : 1);
        Acts::ActsScalar f2 =
            weights.at(i) * weights.at(i) / (tot2 > 0 ? tot2 : 1);
        mval.paramValues.at(j) += f * other.paramValues.at(j);
        mval.paramVariances.at(j) += f2 * other.paramVariances.at(j);
      }
    }
  }

  // Now do the geometric indices
  Cluster clus;

  const auto& binningData = m_segmentation.binningData();
  Acts::Vector2 pos(0., 0.);
  Acts::Vector2 var(0., 0.);

  std::size_t b0min = std::numeric_limits<std::size_t>::max();
  std::size_t b0max = 0;
  std::size_t b1min = std::numeric_limits<std::size_t>::max();
  std::size_t b1max = 0;

  for (std::size_t i = 0; i < values.size(); i++) {
    ModuleValue& other = values.at(i);
    if (!std::holds_alternative<Cluster::Cell>(other.value)) {
      continue;
    }

    Cluster::Cell ch = std::get<Cluster::Cell>(other.value);
    auto bin = ch.bin;

    std::size_t b0 = bin[0];
    std::size_t b1 = bin[1];

    b0min = std::min(b0min, b0);
    b0max = std::max(b0max, b0);
    b1min = std::min(b1min, b1);
    b1max = std::max(b1max, b1);

    float p0 = binningData[0].center(b0);
    float w0 = binningData[0].width(b0);
    float p1 = binningData[1].center(b1);
    float w1 = binningData[1].width(b1);

    pos += Acts::Vector2(weights.at(i) * p0, weights.at(i) * p1);
    // Assume uniform distribution to compute error
    // N.B. This will overestimate the variance
    // but it's better than nothing for now
    var += Acts::Vector2(weights.at(i) * weights.at(i) * w0 * w0 / 12,
                         weights.at(i) * weights.at(i) * w1 * w1 / 12);

    clus.channels.push_back(std::move(ch));

    // Will have the right value at last iteration Do it here to
    // avoid having bogus values when there are no clusters
    clus.sizeLoc0 = b0max - b0min + 1;
    clus.sizeLoc1 = b1max - b1min + 1;
  }

  if (tot > 0) {
    pos /= tot;
    var /= (tot * tot);
  }

  for (auto idx : m_geoIndices) {
    mval.paramIndices.push_back(idx);
    mval.paramValues.push_back(pos[idx]);
    mval.paramVariances.push_back(var[idx]);
  }

  mval.value = std::move(clus);

  // Finally do the hit association
  for (ModuleValue& other : values) {
    mval.sources.merge(other.sources);
  }

  return mval;
}

}  // namespace ActsExamples
