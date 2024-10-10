// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#ifdef __CLING__

// clang-format off

#pragma link C++ class std::vector<std::vector<int>>+;
#pragma link C++ class std::vector<std::vector<long>>+;

#pragma link C++ class std::vector<std::vector<unsigned int>>+;
#pragma link C++ class std::vector<std::vector<unsigned long>>+;

#pragma link C++ class std::vector<std::vector<std::int32_t>>+;
#pragma link C++ class std::vector<std::vector<std::int64_t>>+;

#pragma link C++ class std::vector<std::vector<std::uint32_t>>+;
#pragma link C++ class std::vector<std::vector<std::uint64_t>>+;

#pragma link C++ class std::vector<std::vector<std::size_t>>+;

#pragma link C++ class std::vector<std::vector<float>>+;
#pragma link C++ class std::vector<std::vector<double>>+;
#pragma link C++ class std::vector<std::vector<bool>>+;

// clang-format on

#endif
