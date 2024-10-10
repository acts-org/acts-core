// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Utilities/Options.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {
constexpr char s_separator = ':';
}

// interval

std::istream& ActsExamples::Options::operator>>(
    std::istream& is, ActsExamples::Options::Interval& interval) {
  std::string buf;
  is >> buf;

  // default to an unbounded interval
  interval.lower.reset();
  interval.upper.reset();

  // find the limit separator
  auto pos = buf.find_first_of(s_separator);
  // no separator -> invalid input -> unbounded interval
  if (pos == std::string::npos) {
    return is;
  }

  // if it exists, parse limit before separator
  if (0 < pos) {
    auto lowerStr = buf.substr(0, pos);
    interval.lower = std::stod(lowerStr);
  }
  // if it exists, parse limit after separator
  if ((pos + 1) < buf.size()) {
    auto upperStr = buf.substr(pos + 1);
    interval.upper = std::stod(upperStr);
  }

  return is;
}

std::ostream& ActsExamples::Options::operator<<(
    std::ostream& os, const ActsExamples::Options::Interval& interval) {
  if (!interval.lower.has_value() && !interval.upper.has_value()) {
    os << "unbounded";
  } else {
    if (interval.lower.has_value()) {
      os << interval.lower.value();
    }
    os << s_separator;
    if (interval.upper.has_value()) {
      os << interval.upper.value();
    }
  }
  return os;
}

// helper functions to parse and print multiple values

namespace {

template <typename value_t, typename converter_t>
void parseVariable(std::istream& is, std::vector<value_t>& values,
                   converter_t&& convert) {
  values.clear();

  std::string buf;
  is >> buf;
  std::string bufValue;
  std::string::size_type pos = 0;
  std::string::size_type end = std::string::npos;
  do {
    end = buf.find_first_of(s_separator, pos);
    if (end == std::string::npos) {
      // last element; take the rest of the buffer
      bufValue = buf.substr(pos);
    } else {
      bufValue = buf.substr(pos, end - pos);
      pos = end + 1u;
    }
    values.push_back(convert(bufValue));
  } while (end != std::string::npos);
}

template <typename value_t, typename converter_t>
void parseFixed(std::istream& is, std::size_t size, value_t* values,
                converter_t&& convert) {
  // reserve space for the expected number of values
  std::vector<value_t> tmp(size, 0);
  parseVariable(is, tmp, std::forward<converter_t>(convert));
  if (tmp.size() < size) {
    throw std::invalid_argument(
        "Not enough values for fixed-size user option, expected " +
        std::to_string(size) + " received " + std::to_string(tmp.size()));
  }
  if (size < tmp.size()) {
    throw std::invalid_argument(
        "Too many values for fixed-size user option, expected " +
        std::to_string(size) + " received " + std::to_string(tmp.size()));
  }
  std::copy(tmp.begin(), tmp.end(), values);
}

template <typename value_t>
void print(std::ostream& os, std::size_t size, const value_t* values) {
  for (std::size_t i = 0; i < size; ++i) {
    if (0u < i) {
      os << s_separator;
    }
    os << values[i];
  }
}

}  // namespace

// fixed and variable number of generic values

void ActsExamples::Options::detail::parseDoublesFixed(std::istream& is,
                                                      std::size_t size,
                                                      double* values) {
  parseFixed(is, size, values,
             [](const std::string& s) { return std::stod(s); });
}

void ActsExamples::Options::detail::parseDoublesVariable(
    std::istream& is, std::vector<double>& values) {
  parseVariable(is, values, [](const std::string& s) { return std::stod(s); });
}

void ActsExamples::Options::detail::printDoubles(std::ostream& os,
                                                 std::size_t size,
                                                 const double* values) {
  print(os, size, values);
}

// fixed and variable number of integers

void ActsExamples::Options::detail::parseIntegersFixed(std::istream& is,
                                                       std::size_t size,
                                                       int* values) {
  parseFixed(is, size, values,
             [](const std::string& s) { return std::stoi(s); });
}

void ActsExamples::Options::detail::parseIntegersVariable(
    std::istream& is, std::vector<int>& values) {
  parseVariable(is, values, [](const std::string& s) { return std::stoi(s); });
}

void ActsExamples::Options::detail::printIntegers(std::ostream& os,
                                                  std::size_t size,
                                                  const int* values) {
  print(os, size, values);
}
