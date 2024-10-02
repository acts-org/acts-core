// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/Detray/DetrayConverter.hpp"
#include "Acts/Tests/CommonHelpers/CylindricalDetector.hpp"

#include <vecmem/memory/host_memory_resource.hpp>
#include <vecmem/memory/memory_resource.hpp>

using namespace Acts;
using namespace Acts::Experimental;
using namespace Acts::Test;

GeometryContext tContext;

auto logger =
    Acts::getDefaultLogger("DetrayConverterTests", Acts::Logging::INFO);

BOOST_AUTO_TEST_SUITE(DetrayConversion)

BOOST_AUTO_TEST_CASE(DetrayConversion) {
  // Load the detector from the Test utilities
  auto detector = buildCylindricalDetector(tContext);

  DetrayConverter::Options options;

  vecmem::host_memory_resource memoryResource;

  auto detrayDetector =
      DetrayConverter(std::move(logger))
          .convert<>(tContext, *detector, memoryResource, options);

  BOOST_CHECK_EQUAL(detrayDetector.volumes().size(), 6u);
  // Beampipe : original 3 -> split into 5
  // Nec:       original 4 -> split into 6
  // Layer0:    original 4 -> left  at   4
  // Layer1:    original 4 -> left  at   4
  // Layer2:    original 4 -> left  at   4
  // Pec:       original 4 -> split into 6
  // + portals to itself, one per volume 6
  BOOST_CHECK_EQUAL(detrayDetector.portals().size(), 35u);
}

BOOST_AUTO_TEST_SUITE_END()
