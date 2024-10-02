// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/GeoModel/detail/GeoModelExtentHelper.hpp"

#include "Acts/Plugins/GeoModel/detail/GeoModelBinningHelper.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Enumerate.hpp"

#include <boost/algorithm/string.hpp>

std::vector<Acts::BinningValue>
Acts::detail::GeoModelExentHelper::readBoundsConstaints(
    const std::string& boundsEntry, const std::string& ctype) {
  std::vector<std::string> boundsEntrySplit;
  boost::split(boundsEntrySplit, boundsEntry, boost::is_any_of(","));
  if (boundsEntrySplit.size() < 2u) {
    throw std::invalid_argument(
        "GeoModelBlueprintCreater: Bounds entry has to have at least 2 "
        "entries (type, values)");
  }
  std::set<Acts::BinningValue> constraints;
  // Switch on the bounds type
  if (boundsEntrySplit[0u] == "cyl") {
    // Capture the values
    std::vector<std::string> valuesEntry = {boundsEntrySplit.begin() + 1,
                                            boundsEntrySplit.end()};
    if (valuesEntry.size() < 4u) {
      throw std::invalid_argument(
          "GeoModelBlueprintCreater: Cylinder bounds entry has to have at "
          "least 4 entries (rmin, rmax, zmin, zmax)");
    }
    // Raw database values to extent entries
    constexpr std::array<BinningValue, 6u> bvCyl = {
        BinningValue::binR, BinningValue::binR,   BinningValue::binZ,
        BinningValue::binZ, BinningValue::binPhi, BinningValue::binPhi};

    for (auto [iv, value] : enumerate(valuesEntry)) {
      if (value == ctype || value[0u] == ctype[0u]) {
        constraints.insert(bvCyl.at(iv));
      }
    }
  }
  return {constraints.begin(), constraints.end()};
}

std::vector<Acts::BinningValue>
Acts::detail::GeoModelExentHelper::readBinningConstraints(
    const std::vector<std::string>& binningEntry) {
  std::set<BinningValue> constraints;
  // Loop over the single binning Entries
  for (const auto& sbe : binningEntry) {
    if (sbe.empty()) {
      continue;
    }
    std::vector<std::string> sbTokens;
    boost::split(sbTokens, sbe, boost::is_any_of(","));
    BinningValue bv =
        Acts::detail::GeoModelBinningHelper::toBinningValue(sbTokens[0]);
    if (sbTokens.size() > 1u) {
      std::vector<std::string> valueTokens = {sbTokens.begin() + 1,
                                              sbTokens.end()};
      if (!valueTokens.empty() && valueTokens[0] == "bound") {
        constraints.insert(bv);
      }
    }
  }

  return {constraints.begin(), constraints.end()};
}

std::tuple<Acts::VolumeBounds::BoundsType, Acts::Extent>
Acts::detail::GeoModelExentHelper::extentFromTable(
    const std::vector<std::string>& boundsEntrySplit,
    const Acts::Extent& externalExtent, const Acts::Extent& internalExtent,
    bool roundInternalExtent) {
  // Check the bounds entry
  if (boundsEntrySplit.size() < 2u) {
    throw std::invalid_argument(
        "GeoModelBlueprintCreater: Bounds entry has to have at least 2 "
        "entries (type, values)");
  }

  // Start with the mother extent
  // -> and shrink it to size
  VolumeBounds::BoundsType boundsType = VolumeBounds::BoundsType::eOther;
  Extent extent;
  // Switch on the bounds type
  if (boundsEntrySplit[0u] == "cyl") {
    // Set the bounds type
    boundsType = VolumeBounds::BoundsType::eCylinder;
    // Capture the values
    std::vector<std::string> valuesEntry = {boundsEntrySplit.begin() + 1,
                                            boundsEntrySplit.end()};
    if (valuesEntry.size() < 4u) {
      throw std::invalid_argument(
          "GeoModelBlueprintCreater: Cylinder bounds entry has to have at "
          "least 4 entries (rmin, rmax, zmin, zmax)");
    }
    // Raw database values to extent entries
    constexpr std::array<BinningValue, 6u> bvCyl = {
        BinningValue::binR, BinningValue::binR,   BinningValue::binZ,
        BinningValue::binZ, BinningValue::binPhi, BinningValue::binPhi};
    for (auto [iv, value] : enumerate(valuesEntry)) {
      // Get the binning value
      BinningValue bValue = bvCyl.at(iv);
      ActsScalar val = std::numeric_limits<ActsScalar>::max();
      bool isMin = (iv % 2 == 0);
      // Case "e" : exxternal extent
      if (value == "e") {
        // External parameters do not constrain it
        if (!externalExtent.constrains(bValue)) {
          throw std::invalid_argument(
              "GeoModelExtentHelper: External extent does not constrain. ");
        }
        val = isMin ? externalExtent.min(bValue) : externalExtent.max(bValue);
      } else if (value == "i" || value[0u] == 'i') {
        // Add the envelope
        ActsScalar envelope = 0.;
        if (value.size() > 2u) {
          std::vector<std::string> valEntry;
          boost::split(valEntry, value, boost::is_any_of("+"));
          envelope = std::stod(valEntry[1]);
        }

        // Internals do not constrain it
        if (!internalExtent.constrains(bValue)) {
          throw std::invalid_argument(
              "GeoModelExtentHelper: Internals do not constrain.");
        }
        val = isMin ? internalExtent.min(bValue) - envelope
                    : internalExtent.max(bValue) + envelope;
        // Case "i" : Inherited from mother
      } else {
        val = std::stod(value);
      }
      // Case value is a number -> shrink mother to it or set it
      if (isMin) {
        extent.setMin(bValue, val);
      } else {
        extent.setMax(bValue, val);
      }
    }
  }
  // Round up / down if configured
  if (roundInternalExtent) {
    for (const auto& bv : allBinningValues()) {
      if (internalExtent.constrains(bv)) {
        extent.setMin(bv, std::floor(extent.min(bv)));
        extent.setMax(bv, std::ceil(extent.max(bv)));
      }
    }
  }

  return std::make_tuple(boundsType, extent);
}
