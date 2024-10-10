// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <exception>
#include <iostream>
#include <string>

#include <TApplication.h>
#include <boost/program_options.hpp>

#include "boundParamResolution.C"

using namespace boost::program_options;

int main(int argc, char** argv) {
  std::cout << "*** ACTS Residual and Pull plotting " << std::endl;

  try {
    options_description description("*** Usage:");

    // Add the program options
    auto ao = description.add_options();
    ao("help,h", "Display this help message");
    ao("silent,s", bool_switch(), "Silent mode (without X-window/display).");
    ao("input,i", value<std::string>()->required(),
       "Input ROOT file containing the input TTree.");
    ao("tree,t", value<std::string>()->default_value("trackstates"),
       "Input TTree name.");
    ao("output,o", value<std::string>()->default_value(""),
       "Output ROOT file with histograms");
    ao("predicted", bool_switch(), "Analyze the predicted parameters.");
    ao("filtered", bool_switch(), "Analyze the filtered parameters.");
    ao("smoothed", bool_switch(), "Analyze the smoothed parameters.");
    ao("fit-predicted", bool_switch(), "Fit the predicted parameters.");
    ao("fit-filtered", bool_switch(), "Fit the filtered parameters.");
    ao("fit-smoothed", bool_switch(), "Fit the smoothed parameters.");
    ao("save", value<std::string>()->default_value("png"),
       "Output save format (to be interpreted by ROOT).");

    // Set up the variables map
    variables_map vm;
    store(command_line_parser(argc, argv).options(description).run(), vm);

    if (vm.contains("help")) {
      std::cout << description;
      return 1;
    }

    notify(vm);

    // Parse the parameters
    auto iFile = vm["input"].as<std::string>();
    auto iTree = vm["tree"].as<std::string>();
    auto oFile = vm["output"].as<std::string>();
    auto saveAs = vm["save"].as<std::string>();

    TApplication* tApp =
        vm["silent"].as<bool>()
            ? nullptr
            : new TApplication("ResidualAndPulls", nullptr, nullptr);

    // Run the actual resolution estimation
    switch (boundParamResolution(
        iFile, iTree, oFile, vm["predicted"].as<bool>(),
        vm["filtered"].as<bool>(), vm["smoothed"].as<bool>(),
        vm["fit-predicted"].as<bool>(), vm["fit-filtered"].as<bool>(),
        vm["fit-smoothed"].as<bool>(), saveAs)) {
      case -1:
        std::cout << "*** Input file could not be opened, check name/path."
                  << std::endl;
        return -1;
      case -2:
        std::cout << "*** Input tree could not be found, check name."
                  << std::endl;
        return -2;
      default:
        std::cout << "*** Successful run." << std::endl;
    }

    if (tApp != nullptr) {
      tApp->Run();
    }

  } catch (std::exception& e) {
    std::cerr << e.what() << "\n";
  }

  std::cout << "*** Done." << std::endl;
  return 0;
}
