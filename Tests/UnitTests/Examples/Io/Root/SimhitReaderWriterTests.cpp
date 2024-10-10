// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Tests/CommonHelpers/FloatComparisons.hpp"
#include "Acts/Tests/CommonHelpers/WhiteBoardUtilities.hpp"
#include "Acts/Utilities/Zip.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Io/Root/RootSimHitReader.hpp"
#include "ActsExamples/Io/Root/RootSimHitWriter.hpp"

#include <fstream>
#include <iostream>
#include <random>

using namespace ActsExamples;
using namespace Acts::Test;

std::mt19937 gen(23);

auto makeTestSimhits(std::size_t nSimHits) {
  std::uniform_int_distribution<std::uint64_t> distIds(
      1, std::numeric_limits<std::uint64_t>::max());
  std::uniform_int_distribution<std::int32_t> distIndex(1, 20);

  SimHitContainer simhits;
  for (auto i = 0ul; i < nSimHits; ++i) {
    Acts::GeometryIdentifier geoid(distIds(gen));
    SimBarcode pid(distIds(gen));

    Acts::Vector4 pos4 = Acts::Vector4::Random();
    Acts::Vector4 before4 = Acts::Vector4::Random();
    Acts::Vector4 after4 = Acts::Vector4::Random();

    auto index = distIndex(gen);

    simhits.insert(SimHit(geoid, pid, pos4, before4, after4, index));
  }

  return simhits;
}

BOOST_AUTO_TEST_SUITE(RootSimHitReaderWriter)

BOOST_AUTO_TEST_CASE(RoundTripTest) {
  ////////////////////////////
  // Create some dummy data //
  ////////////////////////////
  auto simhits1 = makeTestSimhits(20);
  auto simhits2 = makeTestSimhits(15);

  ///////////
  // Write //
  ///////////
  RootSimHitWriter::Config writerConfig;
  writerConfig.inputSimHits = "hits";
  writerConfig.filePath = "./testhits.root";

  RootSimHitWriter writer(writerConfig, Acts::Logging::WARNING);

  auto readWriteTool =
      GenericReadWriteTool<>().add(writerConfig.inputSimHits, simhits1);

  // Write two different events
  readWriteTool.write(writer, 11);

  std::get<0>(readWriteTool.tuple) = simhits2;
  readWriteTool.write(writer, 22);

  writer.finalize();

  //////////
  // Read //
  //////////
  RootSimHitReader::Config readerConfig;
  readerConfig.outputSimHits = "hits";
  readerConfig.filePath = "./testhits.root";

  RootSimHitReader reader(readerConfig, Acts::Logging::WARNING);
  // Read two different events
  const auto [hitsRead2] = readWriteTool.read(reader, 22);
  const auto [hitsRead1] = readWriteTool.read(reader, 11);
  reader.finalize();

  ///////////
  // Check //
  ///////////

  auto check = [](const auto &testhits, const auto &refhits, auto tol) {
    BOOST_CHECK_EQUAL(testhits.size(), refhits.size());

    for (const auto &[ref, test] : Acts::zip(refhits, testhits)) {
      CHECK_CLOSE_ABS(test.fourPosition(), ref.fourPosition(), tol);
      CHECK_CLOSE_ABS(test.momentum4After(), ref.momentum4After(), tol);
      CHECK_CLOSE_ABS(test.momentum4Before(), ref.momentum4Before(), tol);

      BOOST_CHECK_EQUAL(ref.geometryId(), test.geometryId());
      BOOST_CHECK_EQUAL(ref.particleId(), test.particleId());
      BOOST_CHECK_EQUAL(ref.index(), test.index());
    }
  };

  check(hitsRead1, simhits1, 1.e-6);
  check(hitsRead2, simhits2, 1.e-6);
}

BOOST_AUTO_TEST_SUITE_END()
