// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Digitization/MeasurementCreation.hpp"

#include "Acts/EventData/MeasurementHelpers.hpp"
#include "Acts/EventData/SourceLink.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"

#include <stdexcept>
#include <string>
#include <utility>

ActsExamples::VariableBoundMeasurementProxy ActsExamples::createMeasurement(
    MeasurementContainer& container, const DigitizedParameters& dParams,
    const IndexSourceLink& isl) {
  Acts::SourceLink sl{isl};

  if (dParams.indices.size() > 4u) {
    std::string errorMsg = "Invalid/mismatching measurement dimension: " +
                           std::to_string(dParams.indices.size());
    throw std::runtime_error(errorMsg.c_str());
  }

  return Acts::visit_measurement(
      dParams.indices.size(), [&](auto dim) -> VariableBoundMeasurementProxy {
        auto [indices, par, cov] = measurementConstituents<dim>(dParams);
        return container.emplaceMeasurement<dim>(sl, indices, par, cov);
      });
}
