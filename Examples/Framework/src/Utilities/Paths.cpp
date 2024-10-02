// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Utilities/Paths.hpp"

#include "Acts/Utilities/Logger.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <map>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <type_traits>
#include <vector>

std::string ActsExamples::ensureWritableDirectory(const std::string& dir) {
  using std::filesystem::current_path;
  using std::filesystem::path;

  auto dir_path = dir.empty() ? current_path() : path(dir);
  if (exists(dir_path) && !is_directory(dir_path)) {
    throw std::runtime_error("'" + dir +
                             "' already exists but is not a directory");
  }
  create_directories(dir_path);
  return canonical(dir_path).native();
}

std::string ActsExamples::joinPaths(const std::string& dir,
                                    const std::string& name) {
  if (dir.empty()) {
    return name;
  } else {
    return dir + '/' + name;
  }
}

std::string ActsExamples::perEventFilepath(const std::string& dir,
                                           const std::string& name,
                                           std::size_t event) {
  char prefix[64];

  snprintf(prefix, sizeof(prefix), "event%09zu-", event);

  if (dir.empty()) {
    return prefix + name;
  } else {
    return dir + '/' + prefix + name;
  }
}

std::pair<std::size_t, std::size_t> ActsExamples::determineEventFilesRange(
    const std::string& dir, const std::string& name) {
  using std::filesystem::current_path;
  using std::filesystem::directory_iterator;
  using std::filesystem::path;

  ACTS_LOCAL_LOGGER(
      Acts::getDefaultLogger("EventFilesRange", Acts::Logging::INFO));

  // ensure directory path is valid
  auto dir_path = dir.empty() ? current_path() : path(dir);
  if (!exists(dir_path)) {
    throw std::runtime_error("'" + dir_path.native() + "' does not exists");
  }
  if (!is_directory(dir_path)) {
    throw std::runtime_error("'" + dir_path.native() + "' is not a directory");
  }

  // invalid default range that allows simple restriction later on
  std::size_t eventMin = std::numeric_limits<std::size_t>::max();
  std::size_t eventMax = 0;

  // filter matching event files from the directory listing
  std::string filename;
  std::regex re("^event([0-9]+)-" + name + "$");
  std::cmatch match;

  for (const auto& f : directory_iterator(dir_path)) {
    if ((!exists(f.status())) || (!is_regular_file(f.status()))) {
      continue;
    }
    // keep a copy so the match can refer to the underlying const char*
    filename = f.path().filename().native();
    if (std::regex_match(filename.c_str(), match, re)) {
      ACTS_VERBOSE("Matching file " << filename);

      // first sub_match is the whole string, second should be the event number
      std::size_t event = 0;
      auto ret = std::from_chars(match[1].first, match[1].second, event);
      if (ret.ptr == match[1].first) {
        throw std::runtime_error(
            "Could not extract event number from filename");
      }
      // enlarge available event range
      eventMin = std::min(eventMin, event);
      eventMax = std::max(eventMax, event);
    }
  }
  ACTS_VERBOSE("Detected event range [" << eventMin << "," << eventMax << "]");

  // should only occur if no files matched and the initial values persisted.
  if (eventMax < eventMin) {
    return std::make_pair(0u, 0u);
  }
  return std::make_pair(eventMin, eventMax + 1);
}
