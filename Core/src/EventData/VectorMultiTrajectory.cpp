// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/EventData/VectorMultiTrajectory.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/Types.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <iomanip>
#include <ostream>
#include <type_traits>

#include <boost/histogram.hpp>
#include <boost/histogram/axis/category.hpp>
#include <boost/histogram/make_histogram.hpp>

namespace Acts {

auto VectorMultiTrajectory::addTrackState_impl(
    TrackStatePropMask mask, IndexType iprevious) -> IndexType {
  using PropMask = TrackStatePropMask;

  m_index.emplace_back();
  IndexData& p = m_index.back();
  IndexType index = m_index.size() - 1;
  m_previous.emplace_back(iprevious);
  m_next.emplace_back(kInvalid);

  p.allocMask = mask;

  // always set, but can be null
  m_referenceSurfaces.emplace_back(nullptr);

  assert(m_params.size() == m_cov.size());

  if (ACTS_CHECK_BIT(mask, PropMask::Predicted)) {
    m_params.emplace_back();
    m_cov.emplace_back();
    p.ipredicted = m_params.size() - 1;
  }

  if (ACTS_CHECK_BIT(mask, PropMask::Filtered)) {
    m_params.emplace_back();
    m_cov.emplace_back();
    p.ifiltered = m_params.size() - 1;
  }

  if (ACTS_CHECK_BIT(mask, PropMask::Smoothed)) {
    m_params.emplace_back();
    m_cov.emplace_back();
    p.ismoothed = m_params.size() - 1;
  }

  assert(m_params.size() == m_cov.size());

  if (ACTS_CHECK_BIT(mask, PropMask::Jacobian)) {
    m_jac.emplace_back();
    p.ijacobian = m_jac.size() - 1;
  }

  m_sourceLinks.emplace_back(std::nullopt);
  p.iUncalibrated = m_sourceLinks.size() - 1;

  m_measOffset.push_back(kInvalid);
  m_measCovOffset.push_back(kInvalid);

  if (ACTS_CHECK_BIT(mask, PropMask::Calibrated)) {
    m_sourceLinks.emplace_back(std::nullopt);
    p.iCalibratedSourceLink = m_sourceLinks.size() - 1;

    m_projectors.push_back(0);
    p.iprojector = m_projectors.size() - 1;
  }

  // dynamic columns
  for (auto& [key, vec] : m_dynamic) {
    vec->add();
  }

  return index;
}

void VectorMultiTrajectory::addTrackStateComponents_impl(
    IndexType istate, TrackStatePropMask mask) {
  using PropMask = TrackStatePropMask;

  IndexData& p = m_index[istate];
  PropMask currentMask = p.allocMask;

  assert(m_params.size() == m_cov.size());

  if (ACTS_CHECK_BIT(mask, PropMask::Predicted) &&
      !ACTS_CHECK_BIT(currentMask, PropMask::Predicted)) {
    m_params.emplace_back();
    m_cov.emplace_back();
    p.ipredicted = m_params.size() - 1;
  }

  if (ACTS_CHECK_BIT(mask, PropMask::Filtered) &&
      !ACTS_CHECK_BIT(currentMask, PropMask::Filtered)) {
    m_params.emplace_back();
    m_cov.emplace_back();
    p.ifiltered = m_params.size() - 1;
  }

  if (ACTS_CHECK_BIT(mask, PropMask::Smoothed) &&
      !ACTS_CHECK_BIT(currentMask, PropMask::Smoothed)) {
    m_params.emplace_back();
    m_cov.emplace_back();
    p.ismoothed = m_params.size() - 1;
  }

  assert(m_params.size() == m_cov.size());

  if (ACTS_CHECK_BIT(mask, PropMask::Jacobian) &&
      !ACTS_CHECK_BIT(currentMask, PropMask::Jacobian)) {
    m_jac.emplace_back();
    p.ijacobian = m_jac.size() - 1;
  }

  if (ACTS_CHECK_BIT(mask, PropMask::Calibrated) &&
      !ACTS_CHECK_BIT(currentMask, PropMask::Calibrated)) {
    m_sourceLinks.emplace_back(std::nullopt);
    p.iCalibratedSourceLink = m_sourceLinks.size() - 1;

    m_projectors.push_back(0);
    p.iprojector = m_projectors.size() - 1;
  }

  p.allocMask |= mask;
}

void VectorMultiTrajectory::shareFrom_impl(IndexType iself, IndexType iother,
                                           TrackStatePropMask shareSource,
                                           TrackStatePropMask shareTarget) {
  // auto other = getTrackState(iother);
  IndexData& self = m_index[iself];
  IndexData& other = m_index[iother];

  assert(ACTS_CHECK_BIT(getTrackState(iother).getMask(), shareSource) &&
         "Source has incompatible allocation");

  using PM = TrackStatePropMask;

  IndexType sourceIndex{kInvalid};
  switch (shareSource) {
    case PM::Predicted:
      sourceIndex = other.ipredicted;
      break;
    case PM::Filtered:
      sourceIndex = other.ifiltered;
      break;
    case PM::Smoothed:
      sourceIndex = other.ismoothed;
      break;
    case PM::Jacobian:
      sourceIndex = other.ijacobian;
      break;
    default:
      throw std::domain_error{"Unable to share this component"};
  }

  assert(sourceIndex != kInvalid);

  switch (shareTarget) {
    case PM::Predicted:
      assert(shareSource != PM::Jacobian);
      self.ipredicted = sourceIndex;
      break;
    case PM::Filtered:
      assert(shareSource != PM::Jacobian);
      self.ifiltered = sourceIndex;
      break;
    case PM::Smoothed:
      assert(shareSource != PM::Jacobian);
      self.ismoothed = sourceIndex;
      break;
    case PM::Jacobian:
      assert(shareSource == PM::Jacobian);
      self.ijacobian = sourceIndex;
      break;
    default:
      throw std::domain_error{"Unable to share this component"};
  }
}

void VectorMultiTrajectory::unset_impl(TrackStatePropMask target,
                                       IndexType istate) {
  using PM = TrackStatePropMask;

  switch (target) {
    case PM::Predicted:
      m_index[istate].ipredicted = kInvalid;
      break;
    case PM::Filtered:
      m_index[istate].ifiltered = kInvalid;
      break;
    case PM::Smoothed:
      m_index[istate].ismoothed = kInvalid;
      break;
    case PM::Jacobian:
      m_index[istate].ijacobian = kInvalid;
      break;
    case PM::Calibrated:
      m_measOffset[istate] = kInvalid;
      m_measCovOffset[istate] = kInvalid;
      break;
    default:
      throw std::domain_error{"Unable to unset this component"};
  }
}

void VectorMultiTrajectory::clear_impl() {
  m_index.clear();
  m_previous.clear();
  m_next.clear();
  m_params.clear();
  m_cov.clear();
  m_meas.clear();
  m_measOffset.clear();
  m_measCov.clear();
  m_measCovOffset.clear();
  m_jac.clear();
  m_sourceLinks.clear();
  m_projectors.clear();
  m_referenceSurfaces.clear();
  for (auto& [key, vec] : m_dynamic) {
    vec->clear();
  }
}

void detail_vmt::VectorMultiTrajectoryBase::Statistics::toStream(
    std::ostream& os, std::size_t n) {
  using namespace boost::histogram;
  using cat = axis::category<std::string>;

  auto& h = hist;

  auto column_axis = axis::get<cat>(h.axis(0));
  auto type_axis = axis::get<axis::category<>>(h.axis(1));

  auto p = [&](const auto& key, const auto v, const std::string suffix = "") {
    os << std::setw(20) << key << ": ";
    if constexpr (std::is_same_v<std::decay_t<decltype(v)>, double>) {
      os << std::fixed << std::setw(8) << std::setprecision(2) << v / n
         << suffix;
    } else {
      os << std::fixed << std::setw(8) << static_cast<double>(v) / n << suffix;
    }
    os << std::endl;
  };

  for (int t = 0; t < type_axis.size(); t++) {
    os << (type_axis.bin(t) == 1 ? "meas" : "other") << ":" << std::endl;
    double total = 0;
    for (int c = 0; c < column_axis.size(); c++) {
      std::string key = column_axis.bin(c);
      auto v = h.at(c, t);
      if (key == "count") {
        p(key, static_cast<std::size_t>(v));
        continue;
      }
      p(key, v / 1024 / 1024, "M");
      total += v;
    }
    p("total", total / 1024 / 1024, "M");
  }
}

void VectorMultiTrajectory::reserve(std::size_t n) {
  m_index.reserve(n);
  m_previous.reserve(n);
  m_next.reserve(n);
  m_params.reserve(n * 2);
  m_cov.reserve(n * 2);
  m_meas.reserve(n * 2);
  m_measOffset.reserve(n);
  m_measCov.reserve(n * 2 * 2);
  m_measCovOffset.reserve(n);
  m_jac.reserve(n);
  m_sourceLinks.reserve(n);
  m_projectors.reserve(n);
  m_referenceSurfaces.reserve(n);

  for (auto& [key, vec] : m_dynamic) {
    vec->reserve(n);
  }
}

void VectorMultiTrajectory::copyDynamicFrom_impl(IndexType dstIdx,
                                                 HashedString key,
                                                 const std::any& srcPtr) {
  auto it = m_dynamic.find(key);
  if (it == m_dynamic.end()) {
    throw std::invalid_argument{
        "Destination container does not have matching dynamic column"};
  }

  it->second->copyFrom(dstIdx, srcPtr);
}

}  // namespace Acts
