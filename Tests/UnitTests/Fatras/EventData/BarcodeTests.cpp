// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "ActsFatras/EventData/Barcode.hpp"

using ActsFatras::Barcode;

BOOST_AUTO_TEST_SUITE(FatrasBarcode)

BOOST_AUTO_TEST_CASE(MakeDescendant) {
  // initial barcode with primary particle
  auto p3 = Barcode().setVertexPrimary(1).setVertexSecondary(2).setParticle(3);
  BOOST_CHECK_EQUAL(p3.vertexPrimary(), 1u);
  BOOST_CHECK_EQUAL(p3.vertexSecondary(), 2u);
  BOOST_CHECK_EQUAL(p3.particle(), 3u);
  BOOST_CHECK_EQUAL(p3.generation(), 0u);
  BOOST_CHECK_EQUAL(p3.subParticle(), 0u);

  // generate two first-generation descendants
  auto p30 = p3.makeDescendant(0);
  auto p31 = p3.makeDescendant(1);
  BOOST_CHECK_EQUAL(p30.vertexPrimary(), 1u);
  BOOST_CHECK_EQUAL(p30.vertexSecondary(), 2u);
  BOOST_CHECK_EQUAL(p30.particle(), 3u);
  BOOST_CHECK_EQUAL(p30.generation(), 1u);
  BOOST_CHECK_EQUAL(p30.subParticle(), 0u);
  BOOST_CHECK_EQUAL(p31.vertexPrimary(), 1u);
  BOOST_CHECK_EQUAL(p31.vertexSecondary(), 2u);
  BOOST_CHECK_EQUAL(p31.particle(), 3u);
  BOOST_CHECK_EQUAL(p31.generation(), 1u);
  BOOST_CHECK_EQUAL(p31.subParticle(), 1u);

  // generate two (overlapping) second-generation descendants.
  auto p300 = p30.makeDescendant(0);
  auto p310 = p31.makeDescendant(0);
  BOOST_CHECK_EQUAL(p300.vertexPrimary(), 1u);
  BOOST_CHECK_EQUAL(p300.vertexSecondary(), 2u);
  BOOST_CHECK_EQUAL(p300.particle(), 3u);
  BOOST_CHECK_EQUAL(p300.generation(), 2u);
  BOOST_CHECK_EQUAL(p300.subParticle(), 0u);
  BOOST_CHECK_EQUAL(p310.vertexPrimary(), 1u);
  BOOST_CHECK_EQUAL(p310.vertexSecondary(), 2u);
  BOOST_CHECK_EQUAL(p310.particle(), 3u);
  BOOST_CHECK_EQUAL(p310.generation(), 2u);
  BOOST_CHECK_EQUAL(p310.subParticle(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
