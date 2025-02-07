// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace ActsExamples {

/// Ensure that the given directory exists and is writable.
///
/// @return Canonical path to the directory.
///
/// Will create missing directories and throw on any error.
std::string ensureWritableDirectory(const std::string& dir);

/// Join dir and name into one path with correct handling of empty dirs.
std::string joinPaths(const std::string& dir, const std::string& name);

/// Construct a file path of the form `[<dir>/]event<XXXXXXXXX>-<name>`.
///
/// @params dir output directory, current directory if empty
/// @params name basic filename
/// @params event event number
std::string perEventFilepath(const std::string& dir, const std::string& name,
                             std::size_t event);

/// Determine the range of available events in a directory of per-event files.
///
/// @params dir input directory, current directory if empty
/// @params name base filename
/// @return first and last+1 event number
/// @returns {0, 0} when no matching files could be found
///
/// Event files must be named `[<dir>/]event<XXXXXXXXX>-<name>` to be considered
std::pair<std::size_t, std::size_t> determineEventFilesRange(
    const std::string& dir, const std::string& name);

}  // namespace ActsExamples
