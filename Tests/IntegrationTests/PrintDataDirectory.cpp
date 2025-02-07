// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

// This is an example on how to access data from the test data directory.
// It is not really an integration test, but since it does not use the
// unit test framework, placing it into the integration tests directory
// is the least awkward place.

#include "Acts/Tests/CommonHelpers/DataDirectory.hpp"

#include <cstddef>
#include <iostream>

int main(void) {
  std::cout << Acts::Test::getDataPath("") << std::endl;
  std::cout << Acts::Test::getDataPath("missing-dir/does_not_exists.txt")
            << std::endl;
  return EXIT_SUCCESS;
}
