// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/Blueprint.hpp"
#include "Acts/Utilities/BinningData.hpp"

namespace Acts::Experimental::detail::BlueprintHelper {

/// @brief Sort the nodes in the blueprint container node
///
/// @param node the node for which the children should be sorted
/// @param recursive if the sorting should be done recursively to children
void sort(Blueprint::Node& node, bool recursive = true);

/// @brief Fill the gaps in the blueprint container node
///
/// @param node the node for with the gaps should be filled
/// @param adjustToParent nodes, if nodes should be adjusted to parent
void fillGaps(Blueprint::Node& node, bool adjustToParent = true);

/// @brief Fill the gaps in the Cylindrical container node
///
/// @param node the node for with the gaps should be filled
/// @param adjustToParent nodes, if nodes should be adjusted to parent
void fillGapsCylindrical(Blueprint::Node& node, bool adjustToParent = true);

/// @brief Fill the gaps in the Cuboidal container node
///
/// @param node the node for with the gaps should be filled
/// @param adjustToParent nodes, if nodes should be adjusted to parent
void fillGapsCuboidal(Blueprint::Node& node, bool adjustToParent = true);

}  // namespace Acts::Experimental::detail::BlueprintHelper
