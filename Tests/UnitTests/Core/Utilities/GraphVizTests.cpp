// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/tools/output_test_stream.hpp>
#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/GraphViz.hpp"

namespace Acts::Test {

BOOST_AUTO_TEST_SUITE(GraphViz)

BOOST_AUTO_TEST_CASE(ApiTest) {
  std::stringstream ss;

  using namespace Acts::GraphViz;

  Node node1{.id = "node1",
             .label = "Node 1",
             .shape = Shape::Ellipse,
             .style = {Style::Filled}};

  ss << node1;

  std::string exp = R"(node1 [label=<Node 1>, shape=ellipse, style=filled];
)";

  BOOST_CHECK_EQUAL(ss.str(), exp);

  Node node2{.id = "node2",
             .label = "Node 2",
             .shape = Shape::Rectangle,
             .style = {Style::Dashed}};

  Edge edge = {.from = node1, .to = node2, .style = Style::Dashed};

  ss.str("");

  ss << edge;

  exp = R"(node1 -> node2 [style=dashed];
)";

  BOOST_CHECK_EQUAL(ss.str(), exp);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace Acts::Test
