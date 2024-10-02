// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/EventData/VectorTrackContainer.hpp"

#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/Utilities/HashedString.hpp"

#include <iterator>

namespace Acts {

namespace detail_vtc {

VectorTrackContainerBase::VectorTrackContainerBase(
    const VectorTrackContainerBase& other)
    : m_tipIndex{other.m_tipIndex},
      m_stemIndex{other.m_stemIndex},
      m_particleHypothesis{other.m_particleHypothesis},
      m_params{other.m_params},
      m_cov{other.m_cov},
      m_referenceSurfaces{other.m_referenceSurfaces},
      m_nMeasurements{other.m_nMeasurements},
      m_nHoles{other.m_nHoles},
      m_chi2{other.m_chi2},
      m_ndf{other.m_ndf},
      m_nOutliers{other.m_nOutliers},
      m_nSharedHits{other.m_nSharedHits} {
  for (const auto& [key, value] : other.m_dynamic) {
    m_dynamic.insert({key, value->clone()});
  }
  m_dynamicKeys = other.m_dynamicKeys;
  assert(checkConsistency());
}
}  // namespace detail_vtc

VectorTrackContainer::IndexType VectorTrackContainer::addTrack_impl() {
  assert(checkConsistency());

  m_tipIndex.emplace_back(kInvalid);
  m_stemIndex.emplace_back(kInvalid);

  m_particleHypothesis.emplace_back(ParticleHypothesis::pion());
  m_params.emplace_back();
  m_cov.emplace_back();
  m_referenceSurfaces.emplace_back();

  m_nMeasurements.emplace_back();
  m_nHoles.emplace_back();

  m_chi2.emplace_back();
  m_ndf.emplace_back();

  m_nOutliers.emplace_back();
  m_nSharedHits.emplace_back();

  // dynamic columns
  for (auto& [key, vec] : m_dynamic) {
    vec->add();
  }

  assert(checkConsistency());

  return m_tipIndex.size() - 1;
}

void VectorTrackContainer::removeTrack_impl(IndexType itrack) {
  auto erase = [&](auto& vec) {
    assert(itrack < vec.size() && "Index is out of range");
    auto it = vec.begin();
    std::advance(it, itrack);
    vec.erase(it);
  };

  erase(m_tipIndex);
  erase(m_stemIndex);

  erase(m_particleHypothesis);

  erase(m_params);
  erase(m_cov);
  erase(m_referenceSurfaces);

  erase(m_nMeasurements);
  erase(m_nHoles);

  erase(m_chi2);
  erase(m_ndf);

  erase(m_nOutliers);
  erase(m_nSharedHits);

  for (auto& [key, vec] : m_dynamic) {
    vec->erase(itrack);
  }
}

void VectorTrackContainer::copyDynamicFrom_impl(IndexType dstIdx,
                                                HashedString key,
                                                const std::any& srcPtr) {
  auto it = m_dynamic.find(key);
  if (it == m_dynamic.end()) {
    throw std::invalid_argument{
        "Destination container does not have matching dynamic column"};
  }

  it->second->copyFrom(dstIdx, srcPtr);
}

void VectorTrackContainer::ensureDynamicColumns_impl(
    const detail_vtc::VectorTrackContainerBase& other) {
  for (auto& [key, value] : other.m_dynamic) {
    if (!m_dynamic.contains(key)) {
      m_dynamic[key] = value->clone(true);
    }
  }
}

void VectorTrackContainer::reserve(IndexType size) {
  m_tipIndex.reserve(size);
  m_stemIndex.reserve(size);

  m_particleHypothesis.reserve(size);
  m_params.reserve(size);
  m_cov.reserve(size);
  m_referenceSurfaces.reserve(size);

  m_nMeasurements.reserve(size);
  m_nHoles.reserve(size);

  m_chi2.reserve(size);
  m_ndf.reserve(size);

  m_nOutliers.reserve(size);
  m_nSharedHits.reserve(size);

  for (auto& [key, vec] : m_dynamic) {
    vec->reserve(size);
  }
}

void VectorTrackContainer::clear() {
  m_tipIndex.clear();
  m_stemIndex.clear();

  m_particleHypothesis.clear();
  m_params.clear();
  m_cov.clear();
  m_referenceSurfaces.clear();

  m_nMeasurements.clear();
  m_nHoles.clear();

  m_chi2.clear();
  m_ndf.clear();

  m_nOutliers.clear();
  m_nSharedHits.clear();

  for (auto& [key, vec] : m_dynamic) {
    vec->clear();
  }
}

}  // namespace Acts
