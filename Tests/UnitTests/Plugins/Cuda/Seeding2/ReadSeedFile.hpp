// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
