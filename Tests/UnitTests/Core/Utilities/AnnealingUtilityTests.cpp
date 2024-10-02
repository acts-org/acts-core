// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/AnnealingUtility.hpp"

#include <iostream>
#include <vector>

namespace Acts::Test {

BOOST_AUTO_TEST_CASE(annealing_tool_singleChi2_tests) {
  std::vector<double> temperatures{64., 16., 4., 2., 1.5, 1.};
  AnnealingUtility::Config config;
  config.setOfTemperatures = temperatures;
  AnnealingUtility annealingTool(config);

  AnnealingUtility::State state;

  // Test weight decrease when annealing for chi2>cutOff
  // choose a chi2 greater than default config.cutOff (=9.),
  // such that it should decrease with decreasing temperature
  double chi2 = config.cutOff + 3.14;

  // cache last weight
  double previousWeight = 1.;

  std::cout << "Check weight decrease:" << std::endl;
  for (auto temp : temperatures) {
    double weight = annealingTool.getWeight(state, chi2);

    bool hasDecreased = weight < previousWeight;

    BOOST_CHECK(hasDecreased);

    previousWeight = weight;
    annealingTool.anneal(state);

    std::cout << "\tTemperature: " << temp << ", weight: " << weight
              << std::endl;
  }

  // equilibrium should be reached here
  BOOST_CHECK_EQUAL(state.equilibriumReached, true);

  // test reset
  state = AnnealingUtility::State();

  BOOST_CHECK_EQUAL(state.currentTemperatureIndex, 0u);
  BOOST_CHECK_EQUAL(state.equilibriumReached, false);

  // Test weight increase when annealing for chi2<cutOff
  // choose a chi2 smaller than default config.cutOff (=9.),
  // such that it should increase with decreasing temperature
  chi2 = config.cutOff - 3.14;

  // cache last weight
  previousWeight = 0.;

  std::cout << "Check weight increase:" << std::endl;
  for (auto temp : temperatures) {
    double weight = annealingTool.getWeight(state, chi2);

    bool hasIncreased = weight > previousWeight;

    BOOST_CHECK(hasIncreased);

    previousWeight = weight;
    annealingTool.anneal(state);

    std::cout << "\tTemperature: " << temp << ", weight: " << weight
              << std::endl;
  }

  // reset for last test
  state = AnnealingUtility::State();

  // Test weight insensitivity when annealing for chi2==cutOff
  // choose a chi2 equal default config.cutOff (=9.),
  // such that it should be insensitive to decreasing temperature
  chi2 = config.cutOff;

  // cache last weight
  previousWeight = 0.5;

  std::cout << "Check weight insensitivity:" << std::endl;
  for (auto temp : temperatures) {
    double weight = annealingTool.getWeight(state, chi2);

    bool hasNotChanged = weight == previousWeight;

    BOOST_CHECK(hasNotChanged);

    previousWeight = weight;
    annealingTool.anneal(state);

    std::cout << "\tTemperature: " << temp << ", weight: " << weight
              << std::endl;
  }
}

BOOST_AUTO_TEST_CASE(annealing_tool_multiChi2_tests) {
  // vector of different chi2
  std::vector<double> allChi2{1.3, 4.5, 8.4,  0.4, 10.3, 12.3,
                              3.5, 5.8, 11.0, 1.1, 3.5,  6.7};

  std::vector<double> temperatures{64., 16., 4., 2., 1.5, 1.};
  AnnealingUtility::Config config;
  config.setOfTemperatures = {64., 16., 4., 2., 1.5, 1.};
  AnnealingUtility annealingTool(config);

  AnnealingUtility::State state;

  // Test weight decrease when annealing for chi2>cutOff
  // choose a chi2 greater than default config.cutOff (=9.),
  // such that it should decrease with decreasing temperature
  double chi2 = config.cutOff + 5.;

  // cache last weight
  double previousWeight = 1.;

  std::cout << "Check weight decrease:" << std::endl;
  for (auto temp : temperatures) {
    double weight = annealingTool.getWeight(state, chi2, allChi2);

    bool hasDecreased = weight < previousWeight;

    BOOST_CHECK(hasDecreased);

    previousWeight = weight;
    annealingTool.anneal(state);

    std::cout << "\tTemperature: " << temp << ", weight: " << weight
              << std::endl;
  }

  // equilibrium should be reached here
  BOOST_CHECK_EQUAL(state.equilibriumReached, true);

  // test reset
  state = AnnealingUtility::State();

  BOOST_CHECK_EQUAL(state.currentTemperatureIndex, 0u);
  BOOST_CHECK_EQUAL(state.equilibriumReached, false);

  // Test weight increase when annealing for chi2<cutOff
  // choose a chi2 smaller than default config.cutOff (=9.),
  // such that it should increase with decreasing temperature
  chi2 = 1.234;

  // cache last weight
  previousWeight = 0.;

  std::cout << "Check weight increase:" << std::endl;
  for (auto temp : temperatures) {
    double weight = annealingTool.getWeight(state, chi2, allChi2);

    bool hasIncreased = weight > previousWeight;

    BOOST_CHECK(hasIncreased);

    previousWeight = weight;
    annealingTool.anneal(state);

    std::cout << "\tTemperature: " << temp << ", weight: " << weight
              << std::endl;
  }
}

}  // namespace Acts::Test
