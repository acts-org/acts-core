// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <string>

namespace Acts::Test {

/// Get the full path to a file in the test data directory.
///
/// @param relativePath file path relative to the data directory
std::string getDataPath(const std::string& relativePath);

}  // namespace Acts::Test
