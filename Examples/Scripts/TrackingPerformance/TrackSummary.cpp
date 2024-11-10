// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Utilities/Options.hpp"

#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

#include <TApplication.h>
#include <boost/program_options.hpp>
#include <boost/timer/progress_display.hpp>
#include <nlohmann/json.hpp>

#define NLOHMANN_AVAILABLE 1
#include "trackSummaryAnalysis.C"

using namespace boost::program_options;

using Interval = ActsExamples::Options::Interval;
using VariableReals = ActsExamples::Options::VariableReals;

int main(int argc, char** argv) {
  std::cout << "*** ACTS Perigee parameters and Track summary plotting "
            << std::endl;

  try {
    options_description description("*** Usage:");

    // Add the program options
    auto ao = description.add_options();
    ao("help,h", "Display this help message");
    ao("silent,s", bool_switch(), "Silent mode (without X-window/display).");
    ao("events,n", value<unsigned long>()->default_value(0),
       "(Optionally) limit number of events to be processed.");
    ao("peak-events,p", value<unsigned long>()->default_value(0),
       "(Optionally) limit number of events for the range peaking.");
    ao("input,i", value<std::vector<std::string>>()->required(),
       "Input ROOT file(s) containing the input TTree.");
    ao("tree,t", value<std::string>()->default_value("tracksummary"),
       "Input TTree/TChain name.");
    ao("output,o", value<std::string>()->default_value(""),
       "Output ROOT file with histograms");
    ao("hist-bins", value<unsigned int>()->default_value(61),
       "Number of bins for the residual/pull histograms");
    ao("pull-range", value<float>()->default_value(5.),
       "Number of sigmas for the pull range.");
    ao("eta-bins", value<unsigned int>()->default_value(10),
       "Number of bins in eta.");
    ao("eta-range",
       value<Interval>()->value_name("MIN:MAX")->default_value({-3.0, 3.0}),
       "Range for the eta bins.");
    ao("phi-bins", value<unsigned int>()->default_value(10),
       "Number of bins in phi.");
    ao("phi-range",
       value<Interval>()->value_name("MIN:MAX")->default_value({-M_PI, M_PI}),
       "Range for the phi bins.");
    ao("pt-borders", value<VariableReals>()->required(),
       "Transverse momentum borders.");
    ao("config-output", value<std::string>()->default_value(""),
       "(Optional) output histogram configuration json file.");
    ao("config-input", value<std::string>()->default_value(""),
       "(Optional) input histogram configuration json file.");
    // Define all parameters (overwrites individual parameters)
    ao("all", bool_switch(),
       "Process all residual/pull and auxiliary parameters");
    // Define the parameters for the residual/pull analysis
    std::vector<std::string> resPullPars = {"d0",  "z0",   "phi0", "theta0",
                                            "qop", "time", "pt"};
    for (const auto& rp : resPullPars) {
      ao(rp.c_str(), bool_switch(),
         (std::string("Residual/pulls for ") + rp).c_str());
    }
    // Define the auxiliary track information
    std::vector<std::string> auxPars = {"chi2ndf", "measurements", "holes",
                                        "outliers", "shared"};
    for (const auto& aux : auxPars) {
      ao(aux.c_str(), bool_switch(),
         (std::string("Auxiliary information for ") + aux).c_str());
    }

    // Set up the variables map
    variables_map vm;
    store(command_line_parser(argc, argv).options(description).run(), vm);

    if (vm.contains("help")) {
      std::cout << description;
      return 1;
    }

    notify(vm);

    // Events
    unsigned long nEntries = vm["events"].as<unsigned long>();
    unsigned long nPeakEntries = vm["peak-events"].as<unsigned long>();

    // Parse the parameters
    auto iFiles = vm["input"].as<std::vector<std::string>>();
    auto iTree = vm["tree"].as<std::string>();
    auto oFile = vm["output"].as<std::string>();

    // Configuration JSON files
    auto configInput = vm["config-input"].as<std::string>();
    auto configOutput = vm["config-output"].as<std::string>();

    float pullRange = vm["pull-range"].as<float>();
    unsigned int nHistBins = vm["hist-bins"].as<unsigned int>();
    unsigned int nEtaBins = vm["eta-bins"].as<unsigned int>();

    auto etaInterval = vm["eta-range"].as<Interval>();
    std::array<float, 2> etaRange = {
        static_cast<float>(etaInterval.lower.value_or(-3)),
        static_cast<float>(etaInterval.upper.value_or(3.))};

    unsigned int nPhiBins = vm["phi-bins"].as<unsigned int>();
    auto phiInterval = vm["phi-range"].as<Interval>();
    std::array<float, 2> phiRange = {
        static_cast<float>(phiInterval.lower.value_or(-M_PI)),
        static_cast<float>(phiInterval.upper.value_or(M_PI))};

    auto ptBorders = vm["pt-borders"].as<VariableReals>().values;
    if (ptBorders.empty()) {
      ptBorders = {0., std::numeric_limits<double>::infinity()};
    }

    TApplication* tApp =
        vm["silent"].as<bool>()
            ? nullptr
            : new TApplication("TrackSummary", nullptr, nullptr);

    std::bitset<7> residualPulls;
    std::bitset<5> auxiliaries;
    if (vm["all"].as<bool>()) {
      residualPulls = std::bitset<7>{"1111111"};
      auxiliaries = std::bitset<5>{"11111"};
    } else {
      // Set the bit for the chosen parameters(s)
      for (unsigned int iresp = 0; iresp < resPullPars.size(); ++iresp) {
        if (vm[resPullPars[iresp]].as<bool>()) {
          residualPulls.set(iresp);
        }
      }
      // Set the bit for the chosen auxiliaries
      for (unsigned int iaux = 0; iaux < auxPars.size(); ++iaux) {
        if (vm[auxPars[iaux]].as<bool>()) {
          auxiliaries.set(iaux);
        }
      }
    }

    // Run the actual resolution estimation
    switch (trackSummaryAnalysis(
        iFiles, iTree, oFile, configInput, configOutput, nEntries, nPeakEntries,
        pullRange, nHistBins, nPhiBins, phiRange, nEtaBins, etaRange, ptBorders,
        residualPulls, auxiliaries)) {
      case -1: {
        std::cout << "*** Input file could not be opened, check name/path."
                  << std::endl;
      } break;
      case -2: {
        std::cout << "*** Input tree could not be found, check name."
                  << std::endl;
      } break;
      default: {
        std::cout << "*** Successful run." << std::endl;
      };
    }

    if (tApp != nullptr) {
      tApp->Run();
    }

  } catch (std::exception& e) {
    std::cerr << e.what() << "\n";
  }

  std::cout << "*** Done." << std::endl;
  return 1;
}
