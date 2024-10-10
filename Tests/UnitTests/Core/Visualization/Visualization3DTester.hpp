// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

namespace Acts::Test {

/// This is a test function that tests the validity of an obj stream
/// It tests for special characters that are not allowed to be contained
///
/// It checks for:
///
/// * Allowed as tags:
///    mtllib, usemtl
///    v, vn, vt, l, f, g, o, s
///    comment lines (starting with #)
/// * v, vn, vt are checked for 3 entries
/// * f is checked for integer > 0 entries only
/// * f is also check for being a triplet if triMesh is true
/// * l is checked for integer > 0 entries only
///
/// @param tString is the test string
/// @param triMesh is the test if only triangular surfaces exist
///
/// @return a vector of failure messages
inline static std::vector<std::string> testObjString(const std::string& tString,
                                                     bool triMesh = false) {
  std::vector<std::string> errorStrings;
  const std::string w = "[ Invalid obj : ";

  std::vector<std::string> passes = {"#", "usemtl", "mtllib", "o", "g", "s"};

  auto ss = std::stringstream{tString};
  for (std::string line; std::getline(ss, line, '\n');) {
    auto fnbc = line.find_first_not_of(" ", 0);
    if (fnbc != std::string::npos) {
      auto snbc = line.find_first_of(" ", fnbc);
      std::string stag = line.substr(fnbc, snbc - fnbc);

      // Ignore comment line, usemtl, mtllib
      bool pass(false);
      for (const auto& ptag : passes) {
        if (ptag == stag) {
          pass = true;
          break;
        }
      }
      if (pass) {
        continue;
      }

      auto tnbc = line.find_first_not_of(" ", snbc);
      std::string body = line.substr(tnbc, line.size() - tnbc);

      // Check if we have triplets
      if (stag.find("v") != std::string::npos ||
          (stag == std::string("f") && triMesh)) {
        std::vector<std::string> bodySplit;
        boost::split(bodySplit, body, boost::is_any_of(" "));
        if (bodySplit.size() != 3 && stag != std::string("f")) {
          errorStrings.push_back(w + line + " ] " + stag +
                                 " must only have three attributes!");
        } else if (bodySplit.size() != 3) {
          errorStrings.push_back("[ not a triangular mesh : " + line + " ]");
        }
        continue;
      }
      // Check if face and line only have positive integer numbers > 1
      // or deliminator " ", " /"
      if (stag == std::string("f") || stag == std::string("l")) {
        bool onlyDigits =
            (body.find_first_not_of("0123456789/ ") == std::string::npos);
        if (!onlyDigits) {
          errorStrings.push_back(w + line + " ] " + stag +
                                 " can only have positive integers!");
        }
        std::vector<std::string> bodySplit;
        boost::split(bodySplit, body, boost::is_any_of(" "));
        for (auto& bs : bodySplit) {
          if (bs == "0") {
            errorStrings.push_back(w + line +
                                   " ] vertex with index 0 detected!");
          }
        }
        continue;
      }
      errorStrings.push_back(w + line + " ] invalid syntax!");
    }
  }
  return errorStrings;
}

/// Ply element struct
struct PlyElement {
  std::string name = "none";
  std::size_t copies = 0;
  int properties = 0;  // -1 for list
};

/// This is a test function that tests the validity of an obj stream
/// It tests for special characters that are not allowed to be contained
///
/// It checks for:
///
///
/// @param tString is the test string
/// @param triMesh is the test if only triangular surfaces exist
///
/// @return a vector of failure messages
inline static std::vector<std::string> testPlyString(const std::string& tString,
                                                     bool /*triMesh*/ = false) {
  std::vector<std::string> errorStrings;
  const std::string w = "[ Invalid ply : ";

  std::vector<std::string> hPasses = {"format", "comment"};

  auto ss = std::stringstream{tString};
  bool inHeader = false;

  std::size_t lNumber = 0;
  std::size_t cElement = 0;
  std::vector<PlyElement> elements;
  PlyElement currentElement;

  for (std::string line; std::getline(ss, line, '\n'); ++lNumber) {
    // Check the "ply" statement at the beginning of the file
    if (lNumber == 0 && line != "ply") {
      errorStrings.push_back(w + line + " ] first line has to be 'ply");
    } else if (line == "ply") {
      inHeader = true;
      cElement = 0;
      elements.clear();
      continue;
    }
    // Process the header
    if (inHeader) {
      auto fnbc = line.find_first_not_of(" ", 0);
      if (fnbc != std::string::npos) {
        auto snbc = line.find_first_of(" ", fnbc);
        std::string stag = line.substr(fnbc, snbc - fnbc);
        if (stag == "comment" || stag == "format") {
          continue;
        }
        if (stag == "end_header") {
          inHeader = false;
          elements.push_back(currentElement);
          currentElement = PlyElement();
          continue;
        }

        auto tnbc = line.find_first_not_of(" ", snbc);
        std::string body = line.substr(tnbc, line.size() - tnbc);

        auto n0nbc = body.find_first_not_of(" ", 0);
        auto n1nbc = body.find_first_of(" ", n0nbc);
        std::string name = body.substr(n0nbc, n1nbc);

        if (stag == "element") {
          // new element write the old one
          if (currentElement.name != "none" && currentElement.copies > 0) {
            elements.push_back(currentElement);
            currentElement = PlyElement();
          }
          currentElement.name = name;
          // get the number of copies
          auto n2nbc = body.find_first_of(" ", n1nbc);
          std::string copies = body.substr(n1nbc, n2nbc);
          currentElement.copies = std::stoi(copies);
        } else if (stag == "property") {
          if (name == "list") {
            currentElement.properties = -1;
            continue;
          }
          if (currentElement.properties >= 0) {
            ++currentElement.properties;
          }
        } else {
          errorStrings.push_back(w + line + " ] Unknown command.");
        }
      }
    } else {
      if (elements[cElement].copies == 0) {
        ++cElement;
      }
      if (cElement < elements.size()) {
        elements[cElement].copies -= 1;
        std::vector<std::string> lineSplit;
        boost::split(lineSplit, line, boost::is_any_of(" "));
        if (elements[cElement].properties == -1) {
          int nprops = std::stoi(lineSplit[0]);
          if (nprops != (static_cast<int>(lineSplit.size()) - 1)) {
            errorStrings.push_back(w + line + std::string(" ] List expected ") +
                                   std::to_string(nprops) +
                                   std::string(" properties, while found ") +
                                   std::to_string(lineSplit.size() - 1) +
                                   std::string("."));
          }
        } else if (lineSplit.size() !=
                   static_cast<std::size_t>(elements[cElement].properties)) {
          errorStrings.push_back(
              w + line + std::string(" ] Element expected ") +
              std::to_string(elements[cElement].properties) +
              std::string(" properties, while found ") +
              std::to_string(lineSplit.size()) + std::string("."));
        }
      }
    }
  }

  return errorStrings;
}

}  // namespace Acts::Test
