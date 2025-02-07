// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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

Acts::AxisDirection Acts::ProtoAxis::getAxisDirection() const {
  return m_axisDir;
}

const Acts::IAxis& Acts::ProtoAxis::getAxis() const {
  return *m_axis;
}

void Acts::ProtoAxis::setRange(double minE, double maxE) {
  if (!m_autorange) {
    throw std::invalid_argument("ProtoAxis::setRange: Range is already set.");
  }
  if (m_axis->getType() != AxisType::Equidistant) {
    throw std::invalid_argument(
        "ProtoAxis::setRange: Range can only be set for equidistant binning.");
  }
  m_axis = IAxis::createEquidistant(m_axis->getBoundaryType(), minE, maxE,
                                    m_axis->getNBins());
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
