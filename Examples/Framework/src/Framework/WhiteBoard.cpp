// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <algorithm>
#include <array>
#include <string_view>

#include <Eigen/Core>
#include <boost/core/demangle.hpp>

namespace {

/// Compute similarity between two words (see wikipedia)
inline int levenshteinDistance(const std::string_view &a,
                               const std::string_view &b) {
  // for all i and j, d[i,j] will hold the Levenshtein distance between
  // the first i characters of s and the first j characters of t
  Eigen::MatrixXi d = Eigen::MatrixXi::Zero(a.size() + 1, b.size() + 1);

  // source prefixes can be transformed into empty string by
  // dropping all characters
  for (std::size_t i = 1; i < a.size() + 1; ++i) {
    d(i, 0) = i;
  }

  // target prefixes can be reached from empty source prefix
  // by inserting every character
  for (std::size_t j = 1; j < b.size() + 1; ++j) {
    d(0, j) = j;
  }

  // Fill matrix
  for (std::size_t j = 1; j < b.size() + 1; ++j) {
    for (std::size_t i = 1; i < a.size() + 1; ++i) {
      const auto substitutionCost = a[i] == b[j] ? 0 : 1;

      std::array<int, 3> possibilities = {{
          d(i - 1, j) + 1,                    // deletion
          d(i, j - 1) + 1,                    // insertion
          d(i - 1, j - 1) + substitutionCost  // substitution
      }};

      d(i, j) = *std::min_element(possibilities.begin(), possibilities.end());
    }
  }

  // std::cout << "\n" << d << "\n";

  return d(a.size(), b.size());
}

}  // namespace

std::vector<std::string_view> ActsExamples::WhiteBoard::similarNames(
    const std::string_view &name, int distThreshold,
    std::size_t maxNumber) const {
  std::vector<std::pair<int, std::string_view>> names;
  for (const auto &[n, h] : m_store) {
    if (const auto d = levenshteinDistance(n, name); d < distThreshold) {
      names.push_back({d, n});
    }
  }

  std::ranges::sort(names, {}, [](const auto &n) { return n.first; });

  std::vector<std::string_view> selected_names;
  for (std::size_t i = 0; i < std::min(names.size(), maxNumber); ++i) {
    selected_names.push_back(names[i].second);
  }

  return selected_names;
}

std::string ActsExamples::WhiteBoard::typeMismatchMessage(
    const std::string &name, const char *req, const char *act) {
  return std::string{"Type mismatch for '" + name + "'. Requested " +
                     boost::core::demangle(req) + " but actually " +
                     boost::core::demangle(act)};
}
