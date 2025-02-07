// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
