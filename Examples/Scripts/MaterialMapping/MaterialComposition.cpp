// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Utilities/Options.hpp"

#include <algorithm>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <TApplication.h>
#include <boost/program_options.hpp>
#include <boost/version.hpp>
#include <nlohmann/json.hpp>

#define BOOST_AVAILABLE 1
#if ((BOOST_VERSION / 100) % 1000) <= 71
// Boost <=1.71 and lower do not have progress_display.hpp as a replacement yet
#include <boost/progress.hpp>

using progress_display = boost::progress_display;
#else
// Boost >=1.72 can use this as a replacement
#include <boost/timer/progress_display.hpp>

using progress_display = boost::timer::progress_display;
#endif

#include "materialComposition.C"

using namespace boost::program_options;
using VariableReals = ActsExamples::Options::VariableReals;

int main(int argc, char** argv) {
  std::cout << "*** Material Composition plotting " << std::endl;

  try {
    options_description description("*** Usage:");

    // Add the program options
    auto ao = description.add_options();
    ao("help,h", "Display this help message");
    ao("silent,s", bool_switch(), "Silent mode (without X-window/display).");
    ao("input,i", value<std::string>()->default_value(""),
       "Input ROOT file containing the input TTree.");
    ao("tree,t", value<std::string>()->default_value("material-tracks"),
       "Input TTree name.");
    ao("output,o", value<std::string>()->default_value(""),
       "Output ROOT file with histograms");
    ao("bins,b", value<unsigned int>()->default_value(60),
       "Number of bins in eta/phi");
    ao("eta,e", value<float>()->default_value(4.), "Eta range.");
    ao("sub-names", value<std::vector<std::string>>()->multitoken(),
       "Subdetector names.");
    ao("sub-rmin", value<VariableReals>(), "Minimal radial restrictions.");
    ao("sub-rmax", value<VariableReals>(), "Maximal radial restrictions.");
    ao("sub-zmin", value<VariableReals>(), "Minimal z radial restrictions");
    ao("sub-zmax", value<VariableReals>(), "Maximal z radial restrictions.");
    ao("config,c", value<std::string>(), "Configuration file (json).");

    // Set up the variables map
    variables_map vm;
    store(command_line_parser(argc, argv).options(description).run(), vm);
    notify(vm);

    if (vm.contains("help")) {
      std::cout << description;
    }

    // Parse the parameters
    auto iFile = vm["input"].as<std::string>();
    auto iTree = vm["tree"].as<std::string>();
    auto oFile = vm["output"].as<std::string>();

    // Bins & eta range
    unsigned int bins = vm["bins"].as<unsigned int>();
    float eta = vm["eta"].as<float>();

    // Subdetector configurations
    std::vector<Region> dRegion = {};

    if (vm.contains("config")) {
      std::filesystem::path config = vm["config"].as<std::string>();
      std::cout << "Reading region configuration from JSON: " << config
                << std::endl;

      if (!std::filesystem::exists(config)) {
        std::cerr << "Configuration file does not exist." << std::endl;
        return 1;
      }

      std::ifstream ifs(config.string().c_str());
      nlohmann::ordered_json j = nlohmann::ordered_json::parse(ifs);

      for (const auto& [key, regions] : j.items()) {
        dRegion.push_back(Region{key, {}});
        auto& reg = dRegion.back();
        std::cout << "Region(" << key << ")" << std::endl;
        for (const auto& region : regions) {
          float rmin = region["rmin"].template get<float>();
          float rmax = region["rmax"].template get<float>();
          float zmin = region["zmin"].template get<float>();
          float zmax = region["zmax"].template get<float>();

          reg.boxes.push_back({rmin, rmax, zmin, zmax});
          std::cout << "* " << key << " r/z: " << rmin << "/" << rmax << " "
                    << zmin << "/" << zmax << std::endl;
        }
      }
    } else {
      auto snames = vm["sub-names"].as<std::vector<std::string>>();
      auto rmins = vm["sub-rmin"].as<VariableReals>().values;
      auto rmaxs = vm["sub-rmax"].as<VariableReals>().values;
      auto zmins = vm["sub-zmin"].as<VariableReals>().values;
      auto zmaxs = vm["sub-zmax"].as<VariableReals>().values;

      std::size_t subs = snames.size();

      if (subs != rmins.size() || subs != rmaxs.size() ||
          subs != zmins.size() || subs != zmaxs.size()) {
        std::cerr << "Configuration problem." << std::endl;
        return 1;
      }

      // Create the regions
      for (unsigned int is = 0; is < subs; ++is) {
        dRegion.push_back(Region{
            snames[is],
            {{static_cast<float>(rmins[is]), static_cast<float>(rmaxs[is]),
              static_cast<float>(zmins[is]), static_cast<float>(zmaxs[is])}}});
      }
    }

    TApplication* tApp =
        vm["silent"].as<bool>()
            ? nullptr
            : new TApplication("ResidualAndPulls", nullptr, nullptr);

    materialComposition(iFile, iTree, oFile, bins, eta, dRegion);

    if (tApp != nullptr) {
      tApp->Run();
    }

  } catch (std::exception& e) {
    std::cerr << e.what() << "\n";
  }

  std::cout << "*** Done." << std::endl;
  return 0;
}
