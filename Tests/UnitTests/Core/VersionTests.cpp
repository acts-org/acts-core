// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

// the purpose of these tests is to ensure that the version header is valid and
// the exported parameters are accessible. otherwise, there would is no code
// that actually includes the version header.

#include <boost/test/unit_test.hpp>

#include "Acts/ActsVersion.hpp"

#include <string_view>

BOOST_AUTO_TEST_CASE(Version) {
  // the only way to get a zero version would be zero for all components
  BOOST_CHECK_LT(0u, Acts::Version);
  // these tests are not really useful as the version components can be any
  // value. they are there so we touch all variables and ensure that they are
  // accessible.
  BOOST_CHECK_LE(0u, Acts::VersionMajor);
  BOOST_CHECK_LE(0u, Acts::VersionMinor);
  BOOST_CHECK_LE(0u, Acts::VersionPatch);
}

BOOST_AUTO_TEST_CASE(CommitHash) {
  BOOST_CHECK(!std::string_view(Acts::CommitHash).empty());
  BOOST_CHECK(!std::string_view(Acts::CommitHashShort).empty());
}
