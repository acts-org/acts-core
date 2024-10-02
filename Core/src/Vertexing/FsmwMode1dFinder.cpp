// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Vertexing/FsmwMode1dFinder.hpp"

#include "Acts/Vertexing/VertexingError.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

Acts::FsmwMode1dFinder::FsmwMode1dFinder(double firstFraction, double fraction)
    : m_firstFraction(firstFraction), m_fraction(fraction) {}

Acts::Result<double> Acts::FsmwMode1dFinder::getMode(
    std::vector<std::pair<double, double>> inputVector) const {
  if (inputVector.empty()) {
    return VertexingError::EmptyInput;
  }
  if (inputVector.size() == 1) {
    return inputVector.begin()->first;
  }

  // first of all order the vector according to the double value

  std::ranges::sort(inputVector, {}, [](const auto& i) { return i.first; });

  // begin to consider a certain number of elements according to the fraction
  auto begin = inputVector.begin();
  auto end = inputVector.end();

  double overallweight(0.);
  auto best_begin = begin;
  auto best_end = end;

  double last_value = std::numeric_limits<double>::max();

  bool isthelast = false;

  int counter = 0;
  double fraction = m_firstFraction;
  while (!isthelast) {
    counter += 1;
    if (counter == 2) {
      fraction = m_fraction;
    }
    int step = static_cast<int>(std::floor(fraction * (end - begin + 1)));
    overallweight = 0.;
    {
      auto i = begin;
      if (step > 0) {
        auto j_end = i + step - 1;
        for (auto j = i; j != j_end; j++) {
          overallweight += j->second;
        }
      }
    }
    auto i_last = begin + step - 1;

    for (auto i = begin; i != (end - step + 1); ++i, ++i_last) {
      // calculate the weight the interval should be divided into
      overallweight += i_last->second;

      double new_value = ((i + step - 1)->first - i->first) / overallweight;
      if (new_value < last_value) {
        last_value = ((i + step - 1)->first - i->first) / overallweight;
        best_begin = i;
        best_end = i + step - 1;
      }
      overallweight -= i->second;
    }

    // assign the new begin and end...
    begin = best_begin;
    end = best_end;
    last_value = std::numeric_limits<double>::max();

    // Now it should have returned the value with smallest (x2-x1)/weight
    if (best_end - best_begin <= 2) {
      isthelast = true;
    }
  }

  if (best_end - best_begin == 2) {
    auto medium = begin;
    medium++;
    return (begin->first * begin->second + medium->first * medium->second +
            end->first * end->second) /
           (begin->second + medium->second + end->second);
  }

  return (begin->first * begin->second + end->first * end->second) /
         (begin->second + end->second);
}
