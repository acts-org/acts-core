// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
