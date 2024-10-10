// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// Local include(s).
#include "CommandLineArguments.hpp"

// Boost include(s).
#include <boost/program_options.hpp>

// System include(s).
#include <cstdlib>
#include <iostream>
#include <string>

/// Convenience declaration for using the boost::program_options namespace
namespace po = boost::program_options;

void CommandLineArguments::interpret(int argc, char* argv[]) {
  // Declare the supported options.
  po::options_description desc("Acts::Cuda::SeedFinder Test");
  desc.add_options()("help,h", "Produce a help message")(
      "spFile,f", po::value<std::string>()->default_value("sp.txt"),
      "SpacePoint text file name")(
      "quiet,q", po::bool_switch(),
      "Do not print the properties of the reconstructed seeds")(
      "onlyGPU,g", po::bool_switch(),
      "Run the seed finding using only the GPU implementation")(
      "groupsToIterate,n", po::value<unsigned int>()->default_value(500),
      "The number of groups to process as a maximum")(
      "filterDuplicates,d", po::bool_switch(),
      "Look for spacepoint duplicates in the input file, and remove them "
      "(slow!)")("cudaDevice", po::value<int>()->default_value(0),
                 "The CUDA device to use in the test")(
      "cudaDeviceMemory", po::value<unsigned int>()->default_value(0),
      "The amount of CUDA device memory to use, in megabytes. By default it is"
      " 80% of the available amount.");

  // Parse the command line arguments.
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  // Handle the --help flag.
  if (vm.count("help")) {
    std::cout << desc << std::endl;
    exit(0);
  }

  // Store the arguments in the member variables.
  spFile = vm["spFile"].as<std::string>();
  quiet = vm["quiet"].as<bool>();
  onlyGPU = vm["onlyGPU"].as<bool>();
  groupsToIterate = vm["groupsToIterate"].as<unsigned int>();
  filterDuplicates = vm["filterDuplicates"].as<bool>();
  cudaDevice = vm["cudaDevice"].as<int>();
  cudaDeviceMemory = vm["cudaDeviceMemory"].as<unsigned int>();
  return;
}
