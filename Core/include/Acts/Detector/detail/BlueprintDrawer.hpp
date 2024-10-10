// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Detector/Blueprint.hpp"

#include <ostream>
#include <string>

namespace Acts::Experimental::detail::BlueprintDrawer {

/// @brief Nested options struct for the drawer
struct Options {
  struct Node {
    /// ROOT node definitions
    std::string shape = "circle";
    std::string color = "darkorange";

    /// Font properties
    std::string face = "sans-serif";
    int labelText = 12;
    int infoText = 10;

    /// Info properties
    int precision = 1;
  };

  /// @brief The name of the graph
  std::string graphName = "blueprint";

  // Main node types to come
  Node root = Node{};
  Node branch = Node{"diamond", "white"};
  Node leaf = Node{"box", "darkolivegreen1"};
  Node gap = Node{"box", "darkolivegreen3"};

  // Sub node types to come
  Node shape = Node{"cylinder", "lightgrey"};
  Node virtualShape = Node{"cylinder", "white"};
  Node internals = Node{"doubleoctagon", "cadetblue1"};
  Node geoID = Node{"box", "azure"};
  Node roots = Node{"box", "darkkhaki"};
};

/// @brief  Turn into a dot output by writing into a stream
///
/// @tparam the stream type to be used
///
/// @param ss the stream into which the dot output should be written
/// @param node the node to be drawn
/// @param options the options for the drawer
void dotStream(std::ostream& ss, const Blueprint::Node& node,
               const Options& options = Options{});

}  // namespace Acts::Experimental::detail::BlueprintDrawer
