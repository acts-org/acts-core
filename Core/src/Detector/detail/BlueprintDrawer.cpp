// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Detector/detail/BlueprintDrawer.hpp"

#include <vector>

namespace {

/// @brief Generate the shape string
/// @param s the shape of the object
/// @param c the color of the object
/// @return a string with the shape and color
std::string shapeStr(
    const Acts::Experimental::detail::BlueprintDrawer::Options::Node& node) {
  return "[shape=\"" + node.shape + "\";style=\"filled\";fillcolor=\"" +
         node.color + "\"];";
}

/// @brief Generate text output
///
/// @param node the node options
/// @param label the label text
/// @param info the info text
std::string labelStr(
    const Acts::Experimental::detail::BlueprintDrawer::Options::Node& node,
    const std::string& label, const std::vector<std::string>& info = {}) {
  std::string lText = "[label=<<font face=\"";
  lText += node.face;
  lText += "\" point-size=\"";
  lText += std::to_string(node.labelText);
  lText += "\">" + label;
  if (!info.empty()) {
    lText += "</font><br/>";
    lText += "<font face=\"";
    lText += node.face;
    lText += "\" point-size=\"";
    lText += std::to_string(node.infoText);
    lText += "\">";
    for (const auto& i : info) {
      lText += i;
      lText += "<br/>";
    }
  }
  lText += "</font>";
  lText += ">];";
  return lText;
}

}  // namespace

void Acts::Experimental::detail::BlueprintDrawer::dotStream(
    std::ostream& ss, const Acts::Experimental::Blueprint::Node& node,
    const Options& options) {
  // Root / leaf or branch
  if (node.isRoot()) {
    ss << "digraph " << options.graphName << " {" << '\n';
    ss << node.name << " " << labelStr(options.root, node.name, node.auxiliary)
       << '\n';
    ss << node.name << " " << shapeStr(options.root) << '\n';

  } else if (node.isLeaf()) {
    ss << node.name << " " << labelStr(options.leaf, node.name, node.auxiliary)
       << '\n';
    ss << node.name << " "
       << ((node.internalsBuilder != nullptr) ? shapeStr(options.leaf)
                                              : shapeStr(options.gap))
       << '\n';
  } else {
    ss << node.name << " "
       << labelStr(options.branch, node.name, node.auxiliary) << '\n';
    ss << node.name << " " << shapeStr(options.branch) << '\n';
  }
  // Recursive for children
  for (const auto& c : node.children) {
    ss << node.name << " -> " << c->name << ";" << '\n';
    dotStream(ss, *c, options);
  }

  // Shape
  Options::Node shape = node.isLeaf() ? options.shape : options.virtualShape;
  std::stringstream bts;
  bts << node.boundsType;
  ss << node.name + "_shape " << shapeStr(shape) << '\n';
  ss << node.name + "_shape "
     << labelStr(shape, bts.str(),
                 {"t = " + toString(node.transform.translation(), 1),
                  "b = " + toString(node.boundaryValues, 1)})
     << '\n';
  ss << node.name << " -> " << node.name + "_shape [ arrowhead = \"none\" ];"
     << '\n';

  // Sub node detection
  if (node.internalsBuilder != nullptr) {
    ss << node.name + "_int " << shapeStr(options.internals) << '\n';
    ss << node.name << " -> " << node.name + "_int;" << '\n';
  }

  if (node.geoIdGenerator != nullptr) {
    ss << node.name + "_geoID " << shapeStr(options.geoID) << '\n';
    ss << node.name << " -> " << node.name + "_geoID;" << '\n';
  }

  if (node.rootVolumeFinderBuilder != nullptr) {
    ss << node.name + "_roots " << shapeStr(options.roots) << '\n';
    ss << node.name << " -> " << node.name + "_roots;" << '\n';
  }

  if (node.isRoot()) {
    ss << "}" << '\n';
  }
}
