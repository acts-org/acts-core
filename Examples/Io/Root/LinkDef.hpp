// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
