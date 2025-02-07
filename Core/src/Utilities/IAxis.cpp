// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Utilities/IAxis.hpp"

#include "Acts/Utilities/Axis.hpp"

#include <algorithm>
#include <stdexcept>

std::unique_ptr<Acts::IAxis> Acts::IAxis::createEquidistant(
    AxisBoundaryType aBoundaryType, double min, double max, std::size_t nbins) {
  using enum AxisType;
  using enum AxisBoundaryType;

  if (min >= max) {
    std::string msg = "IAxis: Invalid axis range'";
    msg += "', min edge (" + std::to_string(min) + ") ";
    msg += " needs to be smaller than max edge (";
    msg += std::to_string(max) + ").";
    throw std::invalid_argument(msg);
  }
  if (nbins < 1u) {
    throw std::invalid_argument(
        "IAxis: Invalid binning, at least one bin is needed.");
  }

  switch (aBoundaryType) {
    case Open:
      return std::make_unique<Axis<Equidistant, Open>>(min, max, nbins);
    case Bound:
      return std::make_unique<Axis<Equidistant, Bound>>(min, max, nbins);
    case Closed:
      return std::make_unique<Axis<Equidistant, Closed>>(min, max, nbins);
    default:  // should never happen
      throw std::logic_error("Unknown axis boundary type");
  }
}

std::unique_ptr<Acts::IAxis> Acts::IAxis::createVariable(
    AxisBoundaryType aBoundaryType, const std::vector<double>& edges) {
  using enum AxisType;
  using enum AxisBoundaryType;

  // Not enough edges
  if (edges.size() < 2) {
    throw std::invalid_argument(
        "IAxis: Invalid binning, at least two bin edges are needed.");
  }

  // Not sorted
  if (!std::ranges::is_sorted(edges)) {
    throw std::invalid_argument(
        "IAxis: Invalid binning, bin edges are not sorted.");
  }
  switch (aBoundaryType) {
    case Open:
      return std::make_unique<Axis<Variable, Open>>(edges);
    case Bound:
      return std::make_unique<Axis<Variable, Bound>>(edges);
    case Closed:
      return std::make_unique<Axis<Variable, Closed>>(edges);
    default:  // should never happen
      throw std::logic_error("Unknown axis boundary type");
  }
}
