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
    std::ostream& ss, const Acts::Experimental::Gen2Blueprint::Node& node,
    const Options& options) {
  // Replace the "/" in node names
  std::string nodeName = node.name;
  std::replace(nodeName.begin(), nodeName.end(), '/', '_');

  // Root / leaf or branch
  if (node.isRoot()) {
    ss << "digraph " << options.graphName << " {" << '\n';
    ss << nodeName << " " << labelStr(options.root, nodeName, node.auxiliary)
       << '\n';
    ss << nodeName << " " << shapeStr(options.root) << '\n';

  } else if (node.isLeaf()) {
    ss << nodeName << " " << labelStr(options.leaf, nodeName, node.auxiliary)
       << '\n';
    ss << nodeName << " "
       << ((node.internalsBuilder != nullptr) ? shapeStr(options.leaf)
                                              : shapeStr(options.gap))
       << '\n';
  } else {
    ss << nodeName << " " << labelStr(options.branch, nodeName, node.auxiliary)
       << '\n';
    ss << nodeName << " " << shapeStr(options.branch) << '\n';
  }
  // Recursive for children
  for (const auto& c : node.children) {
    // Replace the "/" in node names
    std::string childName = c->name;
    std::replace(childName.begin(), childName.end(), '/', '_');
    ss << nodeName << " -> " << childName << ";" << '\n';
    dotStream(ss, *c, options);
  }

  // Shape
  Options::Node shape = node.isLeaf() ? options.shape : options.virtualShape;
  std::stringstream bts;
  bts << node.boundsType;
  ss << nodeName + "_shape " << shapeStr(shape) << '\n';
  ss << nodeName + "_shape "
     << labelStr(shape, bts.str(),
                 {"t = " + toString(node.transform.translation(), 1),
                  "b = " + toString(node.boundaryValues, 1)})
     << '\n';
  ss << nodeName << " -> " << nodeName + "_shape [ arrowhead = \"none\" ];"
     << '\n';

  // Sub node detection
  if (node.internalsBuilder != nullptr) {
    ss << nodeName + "_int " << shapeStr(options.internals) << '\n';
    ss << nodeName << " -> " << nodeName + "_int;" << '\n';
  }

  if (node.geoIdGenerator != nullptr) {
    ss << nodeName + "_geoID " << shapeStr(options.geoID) << '\n';
    ss << nodeName << " -> " << nodeName + "_geoID;" << '\n';
  }

  if (node.rootVolumeFinderBuilder != nullptr) {
    ss << nodeName + "_roots " << shapeStr(options.roots) << '\n';
    ss << nodeName << " -> " << nodeName + "_roots;" << '\n';
  }

  if (node.isRoot()) {
    ss << "}" << '\n';
  }
}
