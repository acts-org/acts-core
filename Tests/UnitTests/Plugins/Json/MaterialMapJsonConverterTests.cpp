// This file is part of the Acts project.
//
// Copyright (C) 2019-2020 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Plugins/Json/MapJsonConverter.hpp"
#include "Acts/Tests/CommonHelpers/DataDirectory.hpp"

#include <fstream>

BOOST_AUTO_TEST_SUITE(MaterialMapJsonConverter)

BOOST_AUTO_TEST_CASE(RoundtripFromFile) {
  // read reference map from file
  std::ifstream refFile(Acts::Test::getDataPath("material-map.json"));
  nlohmann::json refJson;
  refFile >> refJson;

  // convert to the material map and back again
  Acts::MapJsonConverter::Config converterCfg;
  Acts::MapJsonConverter converter(converterCfg);
  auto materialMap = converter.jsonToMaterialMaps(refJson);
  auto encodedJson = converter.materialMapsToJson(materialMap);

  // verify identical encoded JSON values
  BOOST_CHECK_EQUAL(refJson, encodedJson);
}

BOOST_AUTO_TEST_SUITE_END()
