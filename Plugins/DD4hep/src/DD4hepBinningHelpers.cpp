// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/DD4hep/DD4hepBinningHelpers.hpp"

#include <numbers>

std::vector<Acts::Experimental::ProtoBinning>
Acts::DD4hepBinningHelpers::convertBinning(
    const dd4hep::DetElement &dd4hepElement, const std::string &bname) {
  // Return proto binning vector
  std::vector<Experimental::ProtoBinning> protoBinnings;

  for (const auto &[ab, bVal] : allowedBinnings) {
    auto type =
        getParamOr<std::string>(bname + "_" + ab + "_type", dd4hepElement, "");
    if (!type.empty()) {
      // Default binning is bound
      auto bType = Acts::AxisBoundaryType::Bound;
      // Equidistant or variable binning
      AxisType aType =
          type == "equidistant" ? AxisType::Equidistant : AxisType::Variable;
      int nBins = getParamOr<int>(bname + "_" + ab + "_n", dd4hepElement, 0);
      int nExpansion =
          getParamOr<int>(bname + "_" + ab + "_exp", dd4hepElement, 0);
      // Indicate auto-range checking
      bool autoRange = getParamOr<bool>(bname + "_" + ab + "_autorange",
                                        dd4hepElement, false);
      // Equidistant binning
      if (aType == AxisType::Equidistant) {
        if (autoRange) {
          protoBinnings.push_back(
              Experimental::ProtoBinning(bVal, bType, nBins, nExpansion));
        } else {
          // Equidistant binning
          double minDefault =
              bVal == AxisDirection::AxisPhi ? -std::numbers::pi : 0.;
          double maxDefault =
              bVal == AxisDirection::AxisPhi ? std::numbers::pi : 0.;
          auto min = getParamOr<double>(bname + "_" + ab + "_min",
                                        dd4hepElement, minDefault);
          auto max = getParamOr<double>(bname + "_" + ab + "_max",
                                        dd4hepElement, maxDefault);
          // Check for closed phi binning
          if (bVal == AxisDirection::AxisPhi &&
              (max - min) > 1.9 * std::numbers::pi) {
            bType = Acts::AxisBoundaryType::Closed;
          }
          protoBinnings.push_back(Experimental::ProtoBinning(
              bVal, bType, min, max, nBins, nExpansion));
        }
      } else {
        // Variable binning
        std::vector<double> edges;
        for (int ib = 0; ib <= nBins; ++ib) {
          edges.push_back(getParamOr<double>(
              bname + "_" + ab + "_b" + std::to_string(ib), dd4hepElement, 0.));
        }
        // Check for closed phi binning
        if (bVal == AxisDirection::AxisPhi &&
            (edges.back() - edges.front()) > 1.9 * std::numbers::pi) {
          bType = Acts::AxisBoundaryType::Closed;
        }
        protoBinnings.push_back(
            Experimental::ProtoBinning(bVal, bType, edges, nExpansion));
      }
    }
  }
  return protoBinnings;
}
