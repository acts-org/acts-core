// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <ostream>
#include <vector>

namespace Acts::GraphViz {

enum class Shape {
  Box,
  Polygon,
  Ellipse,
  Oval,
  Circle,
  Point,
  Egg,
  Triangle,
  Plaintext,
  Plain,
  Diamond,
  Trapezium,
  Parallelogram,
  House,
  Pentagon,
  Hexagon,
  Septagon,
  Octagon,
  DoubleCircle,
  DoubleOctagon,
  TripleOctagon,
  InvTriangle,
  InvTrapezium,
  InvHouse,
  Mdiamond,
  Msquare,
  Mcircle,
  Rect,
  Rectangle,
  Square,
  Star,
  None,
  Underline,
  Cylinder,
  Note,
  Tab,
  Folder,
  Box3d,
  Component,
  Promoter,
  Cds,
  Terminator,
  Utr,
  PrimerSite,
  RestrictionSite,
  FivePOverhang,
  ThreePOverhang,
  NOverhang,
  Assembly,
  Signature,
  Insulator,
  Ribosite,
  RNAStab,
  ProteaseSite,
  ProteinStab,
  RPromoter,
  RArrow,
  LArrow,
  LPromoter
};

std::ostream& operator<<(std::ostream& os, const Shape& shape);

enum class Style {
  Filled,
  Invisible,
  Diagonals,
  Rounded,
  Dashed,
  Dotted,
  Solid,
  Bold
};

std::ostream& operator<<(std::ostream& os, const Style& style);

struct Node {
  std::string id;
  std::string label = "";
  Shape shape = Shape::Ellipse;
  std::vector<Style> style = {Style::Solid};
};

std::ostream& operator<<(std::ostream& os, const Node& node);

struct Edge {
  Node from;
  Node to;
  Style style = Style::Solid;
};

std::ostream& operator<<(std::ostream& os, const Edge& node);

}  // namespace Acts::GraphViz
