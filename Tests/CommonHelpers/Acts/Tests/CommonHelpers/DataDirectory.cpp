// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Tests/CommonHelpers/DataDirectory.hpp"

#include <filesystem>

std::string Acts::Test::getDataPath(const std::string& relativePath) {
  using std::filesystem::path;

  path dataDir(ACTS_TEST_DATA_DIR);
  path absPath = dataDir / path(relativePath);
  // ensure absolute, consistent path
  // weakly_canonical allows non-existing trailing components
  return weakly_canonical(absPath).string();
}
