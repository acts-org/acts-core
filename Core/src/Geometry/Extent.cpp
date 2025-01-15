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
#include <numbers>

Acts::Extent::Extent(const ExtentEnvelope& envelope)
    : m_constrains(0), m_envelope(envelope) {
  m_range[toUnderlying(AxisDirection::AxisR)] =
      Range1D<double>(0., std::numeric_limits<double>::max());
  m_range[toUnderlying(AxisDirection::AxisPhi)] =
      Range1D<double>(-std::numbers::pi, std::numbers::pi);
  m_range[toUnderlying(AxisDirection::AxisRPhi)] =
      Range1D<double>(0., std::numeric_limits<double>::max());
  m_range[toUnderlying(AxisDirection::AxisMag)] =
      Range1D<double>(0., std::numeric_limits<double>::max());
}

void Acts::Extent::extend(const Vector3& vtx,
                          const std::vector<AxisDirection>& aDirs,
                          bool applyEnv, bool fillHistograms) {
  for (auto aDir : aDirs) {
    // Get the casted value given the binning value description
    double cValue = VectorHelpers::cast(vtx, aDir);
    if (fillHistograms) {
      m_valueHistograms[toUnderlying(aDir)].push_back(cValue);
    }
    // Apply envelope as suggested
    double lEnv = applyEnv ? m_envelope[aDir][0] : 0.;
    double hEnv = applyEnv ? m_envelope[aDir][1] : 0.;
    double mValue = cValue - lEnv;
    // Special protection for radial value
    if (aDir == AxisDirection::AxisR && mValue < 0.) {
      mValue = std::max(mValue, 0.);
    }
    if (constrains(aDir)) {
      m_range[toUnderlying(aDir)].expand(mValue, cValue + hEnv);
    } else {
      m_range[toUnderlying(aDir)].shrink(mValue, cValue + hEnv);
    }
    m_constrains.set(toUnderlying(aDir));
  }
}

void Acts::Extent::extend(const Extent& rhs,
                          const std::vector<AxisDirection>& aDirs,
                          bool applyEnv) {
  for (auto aDir : aDirs) {
    // The value is constraint, envelope can be optional
    if (rhs.constrains(aDir)) {
      double lEnv = applyEnv ? m_envelope[aDir][0] : 0.;
      double hEnv = applyEnv ? m_envelope[aDir][1] : 0.;
      if (constrains(aDir)) {
        m_range[toUnderlying(aDir)].expand(
            rhs.range()[toUnderlying(aDir)].min() - lEnv,
            rhs.range()[toUnderlying(aDir)].max() + hEnv);
      } else {
        m_range[toUnderlying(aDir)].shrink(
            rhs.range()[toUnderlying(aDir)].min() - lEnv,
            rhs.range()[toUnderlying(aDir)].max() + hEnv);
      }
      m_constrains.set(toUnderlying(aDir));
    } else if (rhs.envelope()[aDir] != zeroEnvelope) {
      // Only an envelope given, but value is not constraint -> apply envelope
      m_range[toUnderlying(aDir)].expand(
          m_range[toUnderlying(aDir)].min() - rhs.envelope()[aDir][0],
          m_range[toUnderlying(aDir)].max() + rhs.envelope()[aDir][1]);
      m_constrains.set(toUnderlying(aDir));
    }
  }
}

void Acts::Extent::addConstrain(const Acts::Extent& rhs,
                                const ExtentEnvelope& envelope) {
  for (const auto& aDir : allAxisDirections()) {
    if (rhs.constrains(aDir) && !constrains(aDir)) {
      const auto& cRange = rhs.range(aDir);
      m_range[toUnderlying(aDir)].setMin(cRange.min() - envelope[aDir][0u]);
      m_range[toUnderlying(aDir)].setMax(cRange.max() + envelope[aDir][1u]);
      m_constrains.set(toUnderlying(aDir));
    }
  }
}

void Acts::Extent::set(AxisDirection aDir, double min, double max) {
  double minval = min;
  if (aDir == AxisDirection::AxisR && minval < 0.) {
    minval = 0.;
  }
  m_range[toUnderlying(aDir)] = Range1D<double>{minval, max};
  m_constrains.set(toUnderlying(aDir));
}

void Acts::Extent::setMin(AxisDirection aDir, double min) {
  double minval = min;
  if (aDir == AxisDirection::AxisR && minval < 0.) {
    minval = 0.;
  }
  m_range[toUnderlying(aDir)].setMin(0u, minval);
  m_constrains.set(toUnderlying(aDir));
}

void Acts::Extent::setMax(AxisDirection aDir, double max) {
  m_range[toUnderlying(aDir)].setMax(0u, max);
  m_constrains.set(toUnderlying(aDir));
}

void Acts::Extent::setEnvelope(const ExtentEnvelope& envelope) {
  m_envelope = envelope;
}

bool Acts::Extent::contains(const Vector3& vtx) const {
  Extent checkExtent;
  for (const auto& bv : allAxisDirections()) {
    if (constrains(bv)) {
      double vtxVal = VectorHelpers::cast(vtx, bv);
      checkExtent.set(bv, vtxVal, vtxVal);
    }
  }
  return contains(checkExtent);
}

bool Acts::Extent::contains(const Extent& rhs,
                            std::optional<AxisDirection> aDir) const {
  // Helper to check including a constraint bit set check
  auto checkContainment = [&](AxisDirection bvc) -> bool {
    if (!constrains(bvc)) {
      return true;
    }
    return (rhs.range()[toUnderlying(bvc)] <= m_range[toUnderlying(bvc)]);
  };

  // Check all
  if (!aDir.has_value()) {
    return std::ranges::all_of(allAxisDirections(), checkContainment);
  }
  // Check specific
  return checkContainment(aDir.value());
}

bool Acts::Extent::intersects(const Extent& rhs,
                              std::optional<AxisDirection> aDir) const {
  // Helper to check including a constraint bit set check
  auto checkIntersect = [&](AxisDirection bvc) -> bool {
    if (!constrains(bvc) || !rhs.constrains(bvc)) {
      return false;
    }
    return (m_range[toUnderlying(bvc)] && rhs.range()[toUnderlying(bvc)]);
  };

  // Check all
  if (!aDir.has_value()) {
    return std::ranges::any_of(allAxisDirections(), checkIntersect);
  }
  // Check specific
  return checkIntersect(aDir.value());
}

bool Acts::Extent::constrains(AxisDirection aDir) const {
  return m_constrains.test(static_cast<std::size_t>(aDir));
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
  for (const auto& bv : allAxisDirections()) {
    if (constrains(bv)) {
      sl << indent << "  - value :" << std::setw(10) << axisDirectionName(bv)
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
