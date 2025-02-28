// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/ProtoAxis.hpp"

#include <sstream>

namespace {
void checkConsistency(Acts::AxisDirection aDir, Acts::AxisBoundaryType abType) {
  if (abType == Acts::AxisBoundaryType::Closed &&
      aDir != Acts::AxisDirection::AxisPhi &&
      aDir != Acts::AxisDirection::AxisRPhi) {
    std::string msg =
        "ProtoAxis: Invalid axis boundary type 'Closed' for direction '";
    msg += axisDirectionName(aDir) +
           "'. Closed boundary type is only valid for "
           "AxisPhi and AxisRPhi directions.";
    throw std::invalid_argument(msg);
  }
}
}  // namespace

Acts::ProtoAxis::ProtoAxis(AxisDirection aDir, Acts::AxisBoundaryType abType,
                           const std::vector<double>& edges)
    : m_axisDir(aDir), m_axis(IAxis::createVariable(abType, edges)) {
  checkConsistency(aDir, abType);
}

Acts::ProtoAxis::ProtoAxis(AxisDirection aDir, AxisBoundaryType abType,
                           double minE, double maxE, std::size_t nbins)
    : m_axisDir(aDir),
      m_axis(IAxis::createEquidistant(abType, minE, maxE, nbins)) {
  checkConsistency(aDir, abType);
}

Acts::ProtoAxis::ProtoAxis(AxisDirection aDir, AxisBoundaryType abType,
                           std::size_t nbins)
    : m_axisDir(aDir),
      m_axis(IAxis::createEquidistant(abType, 0., 1., nbins)),
      m_autorange(true) {
  checkConsistency(aDir, abType);
}

Acts::ProtoAxis::ProtoAxis(const ProtoAxis& other)
    : m_axisDir(other.m_axisDir), m_autorange(other.m_autorange) {
  const auto& axis = other.getAxis();
  if (!m_autorange) {
    const auto& edges = axis.getBinEdges();
    if (axis.getType() == AxisType::Variable) {
      m_axis = IAxis::createVariable(axis.getBoundaryType(), edges);
    } else {
      m_axis = IAxis::createEquidistant(axis.getBoundaryType(), edges.front(),
                                        edges.back(), axis.getNBins());
    }
  } else {
    m_axis = IAxis::createEquidistant(axis.getBoundaryType(), 0., 1.,
                                      axis.getNBins());
  }
}

Acts::ProtoAxis& Acts::ProtoAxis::operator=(const ProtoAxis& other) {
  if (this != &other) {
    m_axisDir = other.m_axisDir;
    m_autorange = other.m_autorange;
    const auto& axis = other.getAxis();
    if (!m_autorange) {
      const auto& edges = axis.getBinEdges();
      if (axis.getType() == AxisType::Variable) {
        m_axis = IAxis::createVariable(axis.getBoundaryType(), edges);
      } else {
        m_axis = IAxis::createEquidistant(axis.getBoundaryType(), edges.front(),
                                          edges.back(), axis.getNBins());
      }
    } else {
      m_axis = IAxis::createEquidistant(axis.getBoundaryType(), 0., 1.,
                                        axis.getNBins());
    }
  }
  return *this;
}

Acts::AxisDirection Acts::ProtoAxis::getAxisDirection() const {
  return m_axisDir;
}

const Acts::IAxis& Acts::ProtoAxis::getAxis() const {
  return *m_axis;
}

void Acts::ProtoAxis::setRange(double minE, double maxE) {
  if (minE > maxE) {
    throw std::invalid_argument(
        "ProtoAxis::setRange: minE > maxE is not allowed.");
  }

  if (m_axis->getType() == AxisType::Equidistant) {
    m_axis = IAxis::createEquidistant(m_axis->getBoundaryType(), minE, maxE,
                                      m_axis->getNBins());
  } else {
    std::vector<double> edges = m_axis->getBinEdges();
    // Clip it to min/max
    std::erase_if(edges,
                  [minE, maxE](double e) { return (e < minE || e > maxE); });
    // Add the min and max
    edges.emplace_back(minE);
    edges.emplace_back(maxE);
    std::ranges::sort(edges);
    m_axis = IAxis::createVariable(m_axis->getBoundaryType(), edges);
  }

  // Force autorange to be false
  m_autorange = false;
}

bool Acts::ProtoAxis::isAutorange() const {
  return m_autorange;
}

void Acts::ProtoAxis::toStream(std::ostream& os) const {
  os << toString();
}

std::string Acts::ProtoAxis::toString() const {
  std::stringstream ss;
  const auto& axis = getAxis();
  ss << "ProtoAxis: " << axis.getNBins() << " bins in "
     << axisDirectionName(getAxisDirection());
  ss << (axis.getType() == AxisType::Variable ? ", variable "
                                              : ", equidistant ");
  if (!m_autorange) {
    const auto& edges = axis.getBinEdges();
    ss << "within [" << edges.front() << ", " << edges.back() << "]";
  } else {
    ss << "within automatic range";
  }
  return ss.str();
}

// ostream operator implementation
std::ostream& Acts::operator<<(std::ostream& os,
                               const std::vector<ProtoAxis>& as) {
  for (const auto& a : as) {
    os << a.toString() << '\n';
  }
  return os;
}
