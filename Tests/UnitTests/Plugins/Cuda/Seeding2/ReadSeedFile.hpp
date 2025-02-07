// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

// Local include(s).
#include "TestSpacePoint.hpp"

// System include(s).
#include <memory>
#include <string>
#include <vector>

/// Function used to read in text files holding seeds
std::vector<std::unique_ptr<TestSpacePoint> > readSeedFile(
    const std::string& fileName, bool filterDuplicates = false);
