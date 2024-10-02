// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackContainerBackendConcept.hpp"
#include "Acts/EventData/detail/DynamicColumn.hpp"
#include "Acts/EventData/detail/DynamicKeyIterator.hpp"
#include "Acts/Utilities/HashedString.hpp"

#include <any>
#include <cassert>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Acts {
class Surface;
template <typename T>
struct IsReadOnlyTrackContainer;

namespace detail_vtc {

class VectorTrackContainerBase {
 public:
  using IndexType = MultiTrajectoryTraits::IndexType;
  static constexpr auto kInvalid = MultiTrajectoryTraits::kInvalid;
  static constexpr auto MeasurementSizeMax =
      MultiTrajectoryTraits::MeasurementSizeMax;

  using Parameters =
      typename detail_lt::FixedSizeTypes<eBoundSize, false>::CoefficientsMap;
  using Covariance =
      typename detail_lt::FixedSizeTypes<eBoundSize, false>::CovarianceMap;

  using ConstParameters =
      typename detail_lt::FixedSizeTypes<eBoundSize, true>::CoefficientsMap;
  using ConstCovariance =
      typename detail_lt::FixedSizeTypes<eBoundSize, true>::CovarianceMap;

 protected:
  VectorTrackContainerBase() = default;

  VectorTrackContainerBase(const VectorTrackContainerBase& other);

  VectorTrackContainerBase(VectorTrackContainerBase&& other) = default;

  // BEGIN INTERFACE HELPER

  template <bool EnsureConst, typename T>
  static std::any component_impl(T& instance, HashedString key,
                                 IndexType itrack) {
    using namespace Acts::HashedStringLiteral;
    if constexpr (EnsureConst) {
      static_assert(std::is_const_v<std::remove_reference_t<T>>,
                    "Is not const");
    }

    using namespace Acts::HashedStringLiteral;
    switch (key) {
      case "tipIndex"_hash:
        return &instance.m_tipIndex[itrack];
      case "stemIndex"_hash:
        return &instance.m_stemIndex[itrack];
      case "params"_hash:
        return &instance.m_params[itrack];
      case "cov"_hash:
        return &instance.m_cov[itrack];
      case "nMeasurements"_hash:
        return &instance.m_nMeasurements[itrack];
      case "nHoles"_hash:
        return &instance.m_nHoles[itrack];
      case "chi2"_hash:
        return &instance.m_chi2[itrack];
      case "ndf"_hash:
        return &instance.m_ndf[itrack];
      case "nOutliers"_hash:
        return &instance.m_nOutliers[itrack];
      case "nSharedHits"_hash:
        return &instance.m_nSharedHits[itrack];
      default:
        auto it = instance.m_dynamic.find(key);
        if (it == instance.m_dynamic.end()) {
          throw std::runtime_error("Unable to handle this component");
        }

        std::conditional_t<EnsureConst, const detail::DynamicColumnBase*,
                           detail::DynamicColumnBase*>
            col = it->second.get();
        assert(col && "Dynamic column is null");
        return col->get(itrack);
    }
  }

  bool checkConsistency() const {
    std::size_t size = m_tipIndex.size();

    bool result = true;
    result = result && m_tipIndex.size() == size;
    assert(result);
    result = result && m_stemIndex.size() == size;
    assert(result);
    result = result && m_particleHypothesis.size() == size;
    assert(result);
    result = result && m_params.size() == size;
    assert(result);
    result = result && m_cov.size() == size;
    assert(result);
    result = result && m_referenceSurfaces.size() == size;
    assert(result);
    result = result && m_nMeasurements.size() == size;
    assert(result);
    result = result && m_nHoles.size() == size;
    assert(result);
    result = result && m_chi2.size() == size;
    assert(result);
    result = result && m_ndf.size() == size;
    assert(result);
    result = result && m_nOutliers.size() == size;
    assert(result);
    result = result && m_nSharedHits.size() == size;

    for (const auto& [key, col] : m_dynamic) {
      (void)key;
      result = result && col->size() == size;
    }

    return result;
  }

 public:
  constexpr bool hasColumn_impl(HashedString key) const {
    using namespace Acts::HashedStringLiteral;
    switch (key) {
      default:
        return m_dynamic.contains(key);
    }
  }

  const Surface* referenceSurface_impl(IndexType itrack) const {
    return m_referenceSurfaces[itrack].get();
  }

  ParticleHypothesis particleHypothesis_impl(IndexType itrack) const {
    return m_particleHypothesis[itrack];
  }

  std::size_t size_impl() const {
    assert(checkConsistency());
    return m_tipIndex.size();
  }

  detail::DynamicKeyRange<detail::DynamicColumnBase> dynamicKeys_impl() const {
    return {m_dynamic.begin(), m_dynamic.end()};
  }

  // END INTERFACE HELPER

  std::vector<IndexType> m_tipIndex;
  std::vector<IndexType> m_stemIndex;
  std::vector<ParticleHypothesis> m_particleHypothesis;
  std::vector<typename detail_lt::FixedSizeTypes<eBoundSize>::Coefficients>
      m_params;
  std::vector<typename detail_lt::FixedSizeTypes<eBoundSize>::Covariance> m_cov;
  std::vector<std::shared_ptr<const Surface>> m_referenceSurfaces;

  std::vector<unsigned int> m_nMeasurements;
  std::vector<unsigned int> m_nHoles;
  std::vector<float> m_chi2;
  std::vector<unsigned int> m_ndf;
  std::vector<unsigned int> m_nOutliers;
  std::vector<unsigned int> m_nSharedHits;

  std::unordered_map<HashedString, std::unique_ptr<detail::DynamicColumnBase>>
      m_dynamic;
  std::vector<HashedString> m_dynamicKeys;
};

}  // namespace detail_vtc

class VectorTrackContainer;
class ConstVectorTrackContainer;

template <>
struct IsReadOnlyTrackContainer<VectorTrackContainer> : std::false_type {};

class VectorTrackContainer final : public detail_vtc::VectorTrackContainerBase {
 public:
  VectorTrackContainer() : VectorTrackContainerBase{} {}
  VectorTrackContainer(const VectorTrackContainer& other) = default;
  VectorTrackContainer(VectorTrackContainer&&) = default;

  VectorTrackContainer(const ConstVectorTrackContainer& other);

 public:
  // BEGIN INTERFACE

  std::any component_impl(HashedString key, IndexType itrack) {
    return detail_vtc::VectorTrackContainerBase::component_impl<false>(
        *this, key, itrack);
  }

  std::any component_impl(HashedString key, IndexType itrack) const {
    return detail_vtc::VectorTrackContainerBase::component_impl<true>(
        *this, key, itrack);
  }

  IndexType addTrack_impl();

  void removeTrack_impl(IndexType itrack);

  template <typename T>
  constexpr void addColumn_impl(const std::string_view& key) {
    HashedString hashedKey = hashString(key);
    m_dynamic.insert({hashedKey, std::make_unique<detail::DynamicColumn<T>>()});
  }

  Parameters parameters(IndexType itrack) {
    return Parameters{m_params[itrack].data()};
  }

  ConstParameters parameters(IndexType itrack) const {
    return ConstParameters{m_params[itrack].data()};
  }

  Covariance covariance(IndexType itrack) {
    return Covariance{m_cov[itrack].data()};
  }

  ConstCovariance covariance(IndexType itrack) const {
    return ConstCovariance{m_cov[itrack].data()};
  }

  void copyDynamicFrom_impl(IndexType dstIdx, HashedString key,
                            const std::any& srcPtr);

  void ensureDynamicColumns_impl(
      const detail_vtc::VectorTrackContainerBase& other);

  void reserve(IndexType size);
  void clear();

  void setReferenceSurface_impl(IndexType itrack,
                                std::shared_ptr<const Surface> surface) {
    m_referenceSurfaces[itrack] = std::move(surface);
  }

  void setParticleHypothesis_impl(
      IndexType itrack, const ParticleHypothesis& particleHypothesis) {
    m_particleHypothesis[itrack] = particleHypothesis;
  }

  // END INTERFACE
};

static_assert(TrackContainerBackend<VectorTrackContainer>,
              "VectorTrackContainer does not fulfill TrackContainerBackend");

class ConstVectorTrackContainer;

template <>
struct IsReadOnlyTrackContainer<ConstVectorTrackContainer> : std::true_type {};

class ConstVectorTrackContainer final
    : public detail_vtc::VectorTrackContainerBase {
 public:
  ConstVectorTrackContainer() : VectorTrackContainerBase{} {}

  ConstVectorTrackContainer(const ConstVectorTrackContainer& other) = default;
  ConstVectorTrackContainer(const VectorTrackContainer& other)
      : VectorTrackContainerBase{other} {
    assert(checkConsistency());
  }

  ConstVectorTrackContainer(ConstVectorTrackContainer&&) = default;
  ConstVectorTrackContainer(VectorTrackContainer&& other)
      : VectorTrackContainerBase{std::move(other)} {
    assert(checkConsistency());
  }

 public:
  // BEGIN INTERFACE

  std::any component_impl(HashedString key, IndexType itrack) const {
    return detail_vtc::VectorTrackContainerBase::component_impl<true>(
        *this, key, itrack);
  }

  ConstParameters parameters(IndexType itrack) const {
    return ConstParameters{m_params[itrack].data()};
  }

  ConstCovariance covariance(IndexType itrack) const {
    return ConstCovariance{m_cov[itrack].data()};
  }

  // END INTERFACE
};

static_assert(
    TrackContainerBackend<ConstVectorTrackContainer>,
    "ConstVectorTrackContainer does not fulfill TrackContainerBackend");

inline VectorTrackContainer::VectorTrackContainer(
    const ConstVectorTrackContainer& other)
    : VectorTrackContainerBase{other} {
  assert(checkConsistency());
}

}  // namespace Acts
