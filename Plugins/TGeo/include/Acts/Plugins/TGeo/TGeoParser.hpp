// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/Utilities/AxisDefinitions.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "TGeoMatrix.h"

class TGeoNode;
class TGeoVolume;

namespace Acts {

/// @brief TGeoParser is a helper struct that
/// walks recursively through a TGeometry and selects by
/// string comparison the TGeoNodes that match the criteria
///
/// It also buils up the global transform for the conversion
/// into an ACTS Surface
struct TGeoParser {
  using ParseRange = std::pair<double, double>;

  struct SelectedNode {
    // The selected geo node
    const TGeoNode* node = nullptr;
    // The transform to global
    std::unique_ptr<TGeoMatrix> transform = nullptr;
  };

  /// @brief Nested state struct
  ///
  /// This is needed for the recursive parsing of the
  /// geometry, it collects the information during the parsing process
  /// and keeps track of the built up transform
  struct State {
    // The current volume
    TGeoVolume* volume = nullptr;
    // The current node
    TGeoNode* node = nullptr;
    // Bool on branch
    bool onBranch = false;
    // The currently collected nodes
    std::vector<SelectedNode> selectedNodes = {};
  };

  /// @brief Nested configuration struct
  ///
  /// This contains the parsing configuration
  struct Options {
    /// Identify the volume by name
    std::vector<std::string> volumeNames = {};
    /// Identify the sensor(s)/target(s) by name
    std::vector<std::string> targetNames = {};
    /// The local axis definition of TGeo object wrt Acts::Surface
    std::string localAxes = "XYZ";
    /// Scaling from TGeo to ROOT
    double unit = 1 * UnitConstants::cm;
    /// Parse restrictions, several can apply
    std::vector<std::pair<AxisDirection, ParseRange> > parseRanges = {};
  };

  /// The parsing module, it takes the top Volume and recursively steps down
  /// @param state [out] The parsing state configuration, passed through
  /// @param options [in] The parsing options as required
  /// @param gmatrix The current built-up transform to global at this depth
  static void select(State& state, const Options& options,
                     const TGeoMatrix& gmatrix = TGeoIdentity("ID"));

  /// Simple utility function that recursively finds the node by the volume name
  /// in the tgeo branch.
  /// @param currentNode [in] the pointer to the current node in the branch
  /// @param volumeName  [in] the name of the volume to be searched for
  /// @return the pointer to the node corresponding to the volume
  static TGeoNode* findNodeRecursive(TGeoNode* currentNode,
                                     const char* volumeName);
};
}  // namespace Acts
