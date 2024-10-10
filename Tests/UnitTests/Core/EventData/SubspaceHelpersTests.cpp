// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/EventData/SubspaceHelpers.hpp"

BOOST_AUTO_TEST_SUITE(SubspaceHelpers)

BOOST_AUTO_TEST_CASE(SerializeDeserialize) {
  Acts::SubspaceIndices<3> indices = {1, 2, 3};
  auto serialized = Acts::serializeSubspaceIndices<3>(indices);
  auto deserialized = Acts::deserializeSubspaceIndices<3>(serialized);

  BOOST_CHECK_EQUAL_COLLECTIONS(indices.begin(), indices.end(),
                                deserialized.begin(), deserialized.end());
}

BOOST_AUTO_TEST_SUITE_END()
