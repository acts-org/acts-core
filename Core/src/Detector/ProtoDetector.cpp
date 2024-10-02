// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/ProtoDetector.hpp"

#include "Acts/Definitions/Algebra.hpp"

#include <cstddef>
#include <sstream>
#include <stdexcept>

void Acts::ProtoVolume::extendUp(Acts::ProtoVolume& ptVolume) {
  ptVolume.extent.extend(extent);
  if (container.has_value()) {
    for (auto& cv : container.value().constituentVolumes) {
      ptVolume.extent.extend(cv.extent);
      cv.extendUp(ptVolume);
    }
  }
}

void Acts::ProtoVolume::propagateMinDown(BinningValue bValue) {
  if (container.has_value()) {
    for (auto& cv : container.value().constituentVolumes) {
      cv.extent.set(bValue, extent.min(bValue), cv.extent.max(bValue));
      cv.propagateMinDown(bValue);
    }
  }
}

void Acts::ProtoVolume::propagateMaxDown(BinningValue bValue) {
  if (container.has_value()) {
    for (auto& cv : container.value().constituentVolumes) {
      cv.extent.set(bValue, cv.extent.min(bValue), extent.max(bValue));
      cv.propagateMaxDown(bValue);
    }
  }
}

void Acts::ProtoVolume::constrainDown(const Acts::ProtoVolume& ptVolume) {
  extent.addConstrain(ptVolume.extent);
  if (container.has_value()) {
    for (auto& cv : container.value().constituentVolumes) {
      cv.extent.addConstrain(extent);
    }
  }
}

void Acts::ProtoVolume::harmonize(bool legacy) {
  std::vector<BinningValue> otherConstrains;

  // Deal with the constituents
  if (container.has_value() && !container.value().constituentVolumes.empty()) {
    auto& cts = container.value();

    if (cts.constituentBinning.empty()) {
      std::string errorMsg = std::string("ProtoVolume '") + name +
                             std::string("' with constituents, but no binning");
      throw std::runtime_error(errorMsg);
    }

    // Check if there are any layers present
    bool layersPresent = false;
    for (const auto& cv : cts.constituentVolumes) {
      if (cv.internal.has_value()) {
        layersPresent = true;
        break;
      }
    }

    // If layers are present, it can't be a container in the legacy style
    auto binValue = cts.constituentBinning[0].binvalue;
    // Set the first last
    auto& fVolume = cts.constituentVolumes.front();
    auto& lVolume = cts.constituentVolumes.back();

    std::vector<float> borders = {};

    // The volumes should be harmonized in all other constraining values
    for (auto obValue : allBinningValues()) {
      if (obValue != binValue && extent.constrains(obValue)) {
        otherConstrains.push_back(obValue);
      }
    }

    // Legacy conversion - layers are kept untouched
    if (!layersPresent) {
      // Set the outer boundaries
      fVolume.extent.set(binValue, extent.min(binValue),
                         fVolume.extent.max(binValue));
      lVolume.extent.set(binValue, lVolume.extent.min(binValue),
                         extent.max(binValue));
      // Align the containers
      borders.push_back(static_cast<float>(fVolume.extent.min(binValue)));
      for (unsigned int iv = 1; iv < cts.constituentVolumes.size(); ++iv) {
        auto& lv = cts.constituentVolumes[iv - 1u];
        ActsScalar zero = lv.extent.min(binValue);
        ActsScalar low = lv.extent.max(binValue);

        auto& hv = cts.constituentVolumes[iv];
        ActsScalar high = hv.extent.min(binValue);
        ActsScalar mid = 0.5 * (low + high);
        ActsScalar max = hv.extent.max(binValue);
        lv.extent.set(binValue, zero, mid);
        hv.extent.set(binValue, mid, max);
        borders.push_back(mid);
      }
      borders.push_back(cts.constituentVolumes.back().extent.max(binValue));

    } else if (layersPresent && !legacy) {
      // Count the gaps
      std::size_t gaps = 0;
      std::vector<float> boundaries = {};
      // New container vector
      std::vector<ProtoVolume> updatedConstituents;
      ActsScalar containerMin = extent.min(binValue);
      if (fVolume.extent.min(binValue) > containerMin) {
        ProtoVolume gap;
        gap.name = name + "-gap-" + std::to_string(gaps++);
        gap.extent.set(binValue, containerMin, fVolume.extent.min(binValue));
        updatedConstituents.push_back(gap);
        borders.push_back(static_cast<float>(containerMin));
      }
      // Fill the gaps
      for (unsigned int iv = 1; iv < cts.constituentVolumes.size(); ++iv) {
        auto& lv = cts.constituentVolumes[iv - 1u];
        // This volume is one to save
        updatedConstituents.push_back(lv);
        borders.push_back(static_cast<float>(lv.extent.min(binValue)));
        // check if a gap to the next is needed
        ActsScalar low = lv.extent.max(binValue);
        auto& hv = cts.constituentVolumes[iv];
        ActsScalar high = hv.extent.min(binValue);
        if (high > low) {
          ProtoVolume gap;
          gap.name = name + "-gap-" + std::to_string(gaps++);
          gap.extent.set(binValue, low, high);
          updatedConstituents.push_back(gap);
          borders.push_back(static_cast<float>(low));
        }
      }
      ActsScalar constituentsMax = lVolume.extent.max(binValue);
      updatedConstituents.push_back(lVolume);
      borders.push_back(static_cast<float>(constituentsMax));
      // Check the container min/max setting
      ActsScalar containerMax = extent.max(binValue);
      if (constituentsMax < containerMax) {
        ProtoVolume gap;
        gap.name = name + "-gap-" + std::to_string(gaps++);
        gap.extent.set(binValue, constituentsMax, containerMax);
        updatedConstituents.push_back(gap);
        borders.push_back(static_cast<float>(containerMax));
      }
      cts.constituentVolumes = updatedConstituents;
    } else if (legacy && layersPresent) {
      borders = {0., 1.};
    }
    cts.constituentBinning = {
        BinningData(cts.constituentBinning[0].option, binValue, borders)};

    // Harmonize downwards
    for (auto& cv : cts.constituentVolumes) {
      cv.extent.extend(extent, otherConstrains);
      cv.harmonize(legacy);
    }
  }
}

std::string Acts::ProtoVolume::toString(const std::string& indent) const {
  std::string subIndent("  ");
  std::stringstream ss;
  ss << indent << "> volume: " << name << '\n';
  ss << indent << "  extent: ";
  ss << extent.toString(indent) << '\n';
  if (container.has_value()) {
    auto& cts = container.value();
    if (!cts.constituentVolumes.empty()) {
      ss << indent << "  container of " << cts.constituentVolumes.size()
         << " constituents. " << '\n';
      ss << indent << "  constituent binning:" << '\n';
      for (const auto& cb : cts.constituentBinning) {
        ss << cb.toString(indent) << '\n';
      }
      ss << indent << "  constituents are:" << '\n';
      for (const auto& cv : cts.constituentVolumes) {
        ss << cv.toString(indent + subIndent) << '\n';
      }
    }
  }
  return ss.str();
}

std::string Acts::ProtoDetector::toString(const std::string& indent) const {
  std::string subIndent("  ");
  std::stringstream ss;
  ss << indent << "> detector: " << name << '\n';
  ss << worldVolume.toString(indent + subIndent) << '\n';
  return ss.str();
}
