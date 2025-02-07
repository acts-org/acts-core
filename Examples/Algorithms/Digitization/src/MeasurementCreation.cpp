// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Digitization/MeasurementCreation.hpp"

#include "Acts/EventData/MeasurementHelpers.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "ActsExamples/EventData/Measurement.hpp"

#include <stdexcept>
#include <string>

ActsExamples::VariableBoundMeasurementProxy ActsExamples::createMeasurement(
    MeasurementContainer& container, Acts::GeometryIdentifier geometryId,
    const DigitizedParameters& dParams) {
  if (dParams.indices.size() > 4u) {
    std::string errorMsg = "Invalid/mismatching measurement dimension: " +
                           std::to_string(dParams.indices.size());
    throw std::runtime_error(errorMsg.c_str());
  }

  return Acts::visit_measurement(
      dParams.indices.size(), [&](auto dim) -> VariableBoundMeasurementProxy {
        auto [indices, par, cov] = measurementConstituents<dim>(dParams);
        return VariableBoundMeasurementProxy{
            container.emplaceMeasurement<dim>(geometryId, indices, par, cov)};
      });
}
