// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/TGeo/TGeoPrimitivesHelper.hpp"

bool Acts::TGeoPrimitivesHelper::match(const char* first, const char* second) {
  // If we reach at the end of both strings, we are done
  if (*first == '\0' && *second == '\0') {
    return true;
  }

  // Make sure that the characters after '*' are present
  // in second string. This function assumes that the first
  // string will not contain two consecutive '*'
  if (*first == '*' && *(first + 1) != '\0' && *second == '\0') {
    return false;
  }

  // If the first string contains '?', or current characters
  // of both strings match
  if (*first == '?' || *first == *second) {
    return match(first + 1, second + 1);
  }

  // If there is *, then there are two possibilities
  // a) We consider current character of second string
  // b) We ignore current character of second string.
  if (*first == '*') {
    return match(first + 1, second) || match(first, second + 1);
  }
  return false;
}

bool Acts::TGeoPrimitivesHelper::match(const std::vector<std::string>& first,
                                       const char* second) {
  for (const auto& f : first) {
    if (match(f.c_str(), second)) {
      return true;
    }
  }
  return false;
}
