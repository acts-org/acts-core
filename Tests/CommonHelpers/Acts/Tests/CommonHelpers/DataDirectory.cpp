// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
