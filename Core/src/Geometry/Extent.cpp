// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Geometry/Extent.hpp"

#include "Acts/Utilities/VectorHelpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <iomanip>
#include <limits>

Acts::Extent::Extent(const ExtentEnvelope& envelope)
    : m_constrains(0), m_envelope(envelope) {
  m_range[toUnderlying(BinningValue::binR)] =
      Range1D<ActsScalar>(0., std::numeric_limits<ActsScalar>::max());
  m_range[toUnderlying(BinningValue::binPhi)] =
      Range1D<ActsScalar>(-M_PI, M_PI);
  m_range[toUnderlying(BinningValue::binRPhi)] =
      Range1D<ActsScalar>(0., std::numeric_limits<ActsScalar>::max());
  m_range[toUnderlying(BinningValue::binMag)] =
      Range1D<ActsScalar>(0., std::numeric_limits<ActsScalar>::max());
}

void Acts::Extent::extend(const Vector3& vtx,
                          const std::vector<BinningValue>& bValues,
                          bool applyEnv, bool fillHistograms) {
  for (auto bValue : bValues) {
    // Get the casted value given the binning value description
    ActsScalar cValue = VectorHelpers::cast(vtx, bValue);
    if (fillHistograms) {
      m_valueHistograms[toUnderlying(bValue)].push_back(cValue);
    }
    // Apply envelope as suggested
    ActsScalar lEnv = applyEnv ? m_envelope[bValue][0] : 0.;
    ActsScalar hEnv = applyEnv ? m_envelope[bValue][1] : 0.;
    ActsScalar mValue = cValue - lEnv;
    // Special protection for radial value
    if (bValue == BinningValue::binR && mValue < 0.) {
      mValue = std::max(mValue, 0.);
    }
    if (constrains(bValue)) {
      m_range[toUnderlying(bValue)].expand(mValue, cValue + hEnv);
    } else {
      m_range[toUnderlying(bValue)].shrink(mValue, cValue + hEnv);
    }
    m_constrains.set(toUnderlying(bValue));
  }
}

void Acts::Extent::extend(const Extent& rhs,
                          const std::vector<BinningValue>& bValues,
                          bool applyEnv) {
  for (auto bValue : bValues) {
    // The value is constraint, envelope can be optional
    if (rhs.constrains(bValue)) {
      ActsScalar lEnv = applyEnv ? m_envelope[bValue][0] : 0.;
      ActsScalar hEnv = applyEnv ? m_envelope[bValue][1] : 0.;
      if (constrains(bValue)) {
        m_range[toUnderlying(bValue)].expand(
            rhs.range()[toUnderlying(bValue)].min() - lEnv,
            rhs.range()[toUnderlying(bValue)].max() + hEnv);
      } else {
        m_range[toUnderlying(bValue)].shrink(
            rhs.range()[toUnderlying(bValue)].min() - lEnv,
            rhs.range()[toUnderlying(bValue)].max() + hEnv);
      }
      m_constrains.set(toUnderlying(bValue));
    } else if (rhs.envelope()[bValue] != zeroEnvelope) {
      // Only an envelope given, but value is not constraint -> apply envelope
      m_range[toUnderlying(bValue)].expand(
          m_range[toUnderlying(bValue)].min() - rhs.envelope()[bValue][0],
          m_range[toUnderlying(bValue)].max() + rhs.envelope()[bValue][1]);
      m_constrains.set(toUnderlying(bValue));
    }
  }
}

void Acts::Extent::addConstrain(const Acts::Extent& rhs,
                                const ExtentEnvelope& envelope) {
  for (const auto& bValue : allBinningValues()) {
    if (rhs.constrains(bValue) && !constrains(bValue)) {
      const auto& cRange = rhs.range(bValue);
      m_range[toUnderlying(bValue)].setMin(cRange.min() - envelope[bValue][0u]);
      m_range[toUnderlying(bValue)].setMax(cRange.max() + envelope[bValue][1u]);
      m_constrains.set(toUnderlying(bValue));
    }
  }
}

void Acts::Extent::set(BinningValue bValue, ActsScalar min, ActsScalar max) {
  ActsScalar minval = min;
  if (bValue == BinningValue::binR && minval < 0.) {
    minval = 0.;
  }
  m_range[toUnderlying(bValue)] = Range1D<ActsScalar>{minval, max};
  m_constrains.set(toUnderlying(bValue));
}

void Acts::Extent::setMin(BinningValue bValue, ActsScalar min) {
  ActsScalar minval = min;
  if (bValue == BinningValue::binR && minval < 0.) {
    minval = 0.;
  }
  m_range[toUnderlying(bValue)].setMin(0u, minval);
  m_constrains.set(toUnderlying(bValue));
}

void Acts::Extent::setMax(BinningValue bValue, ActsScalar max) {
  m_range[toUnderlying(bValue)].setMax(0u, max);
  m_constrains.set(toUnderlying(bValue));
}

void Acts::Extent::setEnvelope(const ExtentEnvelope& envelope) {
  m_envelope = envelope;
}

bool Acts::Extent::contains(const Vector3& vtx) const {
  Extent checkExtent;
  for (const auto& bv : allBinningValues()) {
    if (constrains(bv)) {
      ActsScalar vtxVal = VectorHelpers::cast(vtx, bv);
      checkExtent.set(bv, vtxVal, vtxVal);
    }
  }
  return contains(checkExtent);
}

bool Acts::Extent::contains(const Extent& rhs,
                            std::optional<BinningValue> bValue) const {
  // Helper to check including a constraint bit set check
  auto checkContainment = [&](BinningValue bvc) -> bool {
    if (!constrains(bvc)) {
      return true;
    }
    return (rhs.range()[toUnderlying(bvc)] <= m_range[toUnderlying(bvc)]);
  };

  // Check all
  if (!bValue.has_value()) {
    return std::ranges::all_of(allBinningValues(), checkContainment);
  }
  // Check specific
  return checkContainment(bValue.value());
}

bool Acts::Extent::intersects(const Extent& rhs,
                              std::optional<BinningValue> bValue) const {
  // Helper to check including a constraint bit set check
  auto checkIntersect = [&](BinningValue bvc) -> bool {
    if (!constrains(bvc) || !rhs.constrains(bvc)) {
      return false;
    }
    return (m_range[toUnderlying(bvc)] && rhs.range()[toUnderlying(bvc)]);
  };

  // Check all
  if (!bValue.has_value()) {
    return std::ranges::any_of(allBinningValues(), checkIntersect);
  }
  // Check specific
  return checkIntersect(bValue.value());
}

bool Acts::Extent::constrains(BinningValue bValue) const {
  return m_constrains.test(static_cast<std::size_t>(bValue));
}

bool Acts::Extent::constrains() const {
  return m_constrains.count() > 0;
}

bool Acts::Extent::operator==(const Extent& e) const {
  if (m_constrains != e.m_constrains) {
    return false;
  }
  if (m_envelope != e.m_envelope) {
    return false;
  }
  if (!(m_range == e.m_range)) {
    return false;
  }
  if (m_valueHistograms != e.m_valueHistograms) {
    return false;
  }
  return true;
}

std::string Acts::Extent::toString(const std::string& indent) const {
  std::stringstream sl;
  sl << indent << "Extent in space :" << std::endl;
  for (const auto& bv : allBinningValues()) {
    if (constrains(bv)) {
      sl << indent << "  - value :" << std::setw(10) << binningValueName(bv)
         << " | range = [" << m_range[toUnderlying(bv)].min() << ", "
         << m_range[toUnderlying(bv)].max() << "]" << std::endl;
    }
  }
  return sl.str();
}

// Overload of << operator for std::ostream for debug output
std::ostream& Acts::operator<<(std::ostream& sl, const Extent& rhs) {
  sl << rhs.toString();
  return sl;
}
