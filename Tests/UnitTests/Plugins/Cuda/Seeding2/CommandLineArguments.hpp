// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

// System include(s).
#include <cstddef>
#include <string>

/// Structure holding the arguments passed to the test executable on the command
/// line
struct CommandLineArguments {
  /// Spacepoint file to use
  std::string spFile = "sp.txt";
  /// Do not print the properties of the reconstructed seeds
  bool quiet = false;
  /// Run the seed finding using only the GPU implementation
  bool onlyGPU = false;
  /// The number of groups to process as a maximum
  std::size_t groupsToIterate = 500;
  /// Look for spacepoint duplicates in the received input file, and remove them
  bool filterDuplicates = false;

  /// The CUDA device to use
  int cudaDevice = 0;
  /// Memory to use on the CUDA device in megabytes (by default it's 80% of the
  /// available)
  std::size_t cudaDeviceMemory = 0;

  /// Interpret the command line arguments of the test executable
  void interpret(int argc, char* argv[]);

};  // struct CommandLineArguments
