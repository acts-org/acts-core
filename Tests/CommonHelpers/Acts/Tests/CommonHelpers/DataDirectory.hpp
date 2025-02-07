// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <string>

namespace Acts::Test {

/// Get the full path to a file in the test data directory.
///
/// @param relativePath file path relative to the data directory
std::string getDataPath(const std::string& relativePath);

}  // namespace Acts::Test
