// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Utilities/GraphViz.hpp"

#include <algorithm>
#include <sstream>
#include <string>

#include <boost/algorithm/string/join.hpp>

namespace Acts::GraphViz {

std::ostream& operator<<(std::ostream& os, const Shape& shape) {
  switch (shape) {
    using enum Shape;

    case Box:
      os << "box";
      break;
    case Polygon:
      os << "polygon";
      break;
    case Ellipse:
      os << "ellipse";
      break;
    case Oval:
      os << "oval";
      break;
    case Circle:
      os << "circle";
      break;
    case Point:
      os << "point";
      break;
    case Egg:
      os << "egg";
      break;
    case Triangle:
      os << "triangle";
      break;
    case Plaintext:
      os << "plaintext";
      break;
    case Plain:
      os << "plain";
      break;
    case Diamond:
      os << "diamond";
      break;
    case Trapezium:
      os << "trapezium";
      break;
    case Parallelogram:
      os << "parallelogram";
      break;
    case House:
      os << "house";
      break;
    case Pentagon:
      os << "pentagon";
      break;
    case Hexagon:
      os << "hexagon";
      break;
    case Septagon:
      os << "septagon";
      break;
    case Octagon:
      os << "octagon";
      break;
    case DoubleCircle:
      os << "doublecircle";
      break;
    case DoubleOctagon:
      os << "doubleoctagon";
      break;
    case TripleOctagon:
      os << "tripleoctagon";
      break;
    case InvTriangle:
      os << "invtriangle";
      break;
    case InvTrapezium:
      os << "invtrapezium";
      break;
    case InvHouse:
      os << "invhouse";
      break;
    case Mdiamond:
      os << "Mdiamond";
      break;
    case Msquare:
      os << "Msquare";
      break;
    case Mcircle:
      os << "Mcircle";
      break;
    case Rect:
      os << "rect";
      break;
    case Rectangle:
      os << "rectangle";
      break;
    case Square:
      os << "square";
      break;
    case Star:
      os << "star";
      break;
    case None:
      os << "none";
      break;
    case Underline:
      os << "underline";
      break;
    case Cylinder:
      os << "cylinder";
      break;
    case Note:
      os << "note";
      break;
    case Tab:
      os << "tab";
      break;
    case Folder:
      os << "folder";
      break;
    case Box3d:
      os << "box3d";
      break;
    case Component:
      os << "component";
      break;
    case Promoter:
      os << "promoter";
      break;
    case Cds:
      os << "cds";
      break;
    case Terminator:
      os << "terminator";
      break;
    case Utr:
      os << "utr";
      break;
    case PrimerSite:
      os << "primersite";
      break;
    case RestrictionSite:
      os << "restrictionsite";
      break;
    case FivePOverhang:
      os << "fivepoverhang";
      break;
    case ThreePOverhang:
      os << "threepoverhang";
      break;
    case NOverhang:
      os << "noverhang";
      break;
    case Assembly:
      os << "assembly";
      break;
    case Signature:
      os << "signature";
      break;
    case Insulator:
      os << "insulator";
      break;
    case Ribosite:
      os << "ribosite";
      break;
    case RNAStab:
      os << "rnastab";
      break;
    case ProteaseSite:
      os << "proteasesite";
      break;
    case ProteinStab:
      os << "proteinstab";
      break;
    case RPromoter:
      os << "rpromoter";
      break;
    case RArrow:
      os << "rarrow";
      break;
    case LArrow:
      os << "larrow";
      break;
    case LPromoter:
      os << "lpromoter";
      break;
    default:
      std::terminate();
  }

  return os;
}

std::ostream& operator<<(std::ostream& os, const Style& style) {
  switch (style) {
    using enum Style;
    case Filled:
      os << "filled";
      break;
    case Invisible:
      os << "invisible";
      break;
    case Diagonals:
      os << "diagonals";
      break;
    case Rounded:
      os << "rounded";
      break;
    case Dashed:
      os << "dashed";
      break;
    case Dotted:
      os << "dotted";
      break;
    case Solid:
      os << "solid";
      break;
    case Bold:
      os << "bold";
      break;
    default:
      os << "unknown";
      break;
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Node& node) {
  if (node.id.empty()) {
    throw std::runtime_error("Node id must not be empty");
  }

  std::string id = node.id;
  std::ranges::replace(id, ' ', '_');
  os << id << " [";

  std::vector<std::string> attributes;

  std::vector<std::string> styles;
  std::ranges::transform(node.style, std::back_inserter(styles),
                         [](const Style& style) {
                           std::stringstream ss;
                           ss << style;
                           return ss.str();
                         });

  if (!node.label.empty()) {
    attributes.push_back("label=<" + node.label + ">");
  }

  std::stringstream ss;
  ss << node.shape;
  attributes.push_back("shape=" + ss.str());

  attributes.push_back("style=" + boost::algorithm::join(styles, ","));

  os << boost::algorithm::join(attributes, ", ");

  os << "];\n";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Edge& node) {
  os << node.from.id << " -> " << node.to.id << " [";

  os << "style=" << node.style;

  os << "];\n";

  return os;
}

}  // namespace Acts::GraphViz
