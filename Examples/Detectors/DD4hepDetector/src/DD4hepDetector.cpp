// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/DD4hepDetector/DD4hepDetector.hpp"

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"
#include "Acts/Plugins/DD4hep/DD4hepFieldAdapter.hpp"
#include "ActsExamples/DD4hepDetector/DD4hepGeometryService.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>

#include <DD4hep/DetElement.h>
#include <DD4hep/Detector.h>
#include <DD4hep/Fields.h>
#include <boost/program_options.hpp>

namespace ActsExamples::DD4hep {

DD4hepDetector::DD4hepDetector(
    std::shared_ptr<DD4hepGeometryService> _geometryService)
    : geometryService(std::move(_geometryService)) {}

auto DD4hepDetector::finalize(
    ActsExamples::DD4hep::DD4hepGeometryService::Config config,
    std::shared_ptr<const Acts::IMaterialDecorator> mdecorator)
    -> std::pair<TrackingGeometryPtr, ContextDecorators> {
  Acts::GeometryContext dd4HepContext;
  config.matDecorator = std::move(mdecorator);
  geometryService =
      std::make_shared<ActsExamples::DD4hep::DD4hepGeometryService>(config);
  TrackingGeometryPtr dd4tGeometry =
      geometryService->trackingGeometry(dd4HepContext);
  if (!dd4tGeometry) {
    throw std::runtime_error{
        "Did not receive tracking geometry from DD4hep geometry service"};
  }
  ContextDecorators dd4ContextDecorators = {};
  // return the pair of geometry and empty decorators
  return std::make_pair<TrackingGeometryPtr, ContextDecorators>(
      std::move(dd4tGeometry), std::move(dd4ContextDecorators));
}

auto DD4hepDetector::finalize(
    const Acts::GeometryContext& gctx,
    const Acts::Experimental::DD4hepDetectorStructure::Options& options)
    -> std::tuple<DetectorPtr, ContextDecorators,
                  Acts::DD4hepDetectorElement::Store> {
  if (geometryService == nullptr) {
    throw std::runtime_error{
        "No DD4hep geometry service configured, can not build "
        "TrackingGeometry."};
  }

  auto world = geometryService->geometry();
  // Build the detector structure
  Acts::Experimental::DD4hepDetectorStructure dd4hepStructure(
      Acts::getDefaultLogger("DD4hepDetectorStructure", options.logLevel));

  /// @return a detector and the detector store
  auto [detector, detectorElements] =
      dd4hepStructure.construct(gctx, world, options);

  // Prepare the return objects
  ContextDecorators contextDecorators = {};

  return {detector, contextDecorators, detectorElements};
}

void DD4hepDetector::drop() {
  geometryService->drop();
}

std::shared_ptr<Acts::DD4hepFieldAdapter> DD4hepDetector::field() const {
  const auto& detector = geometryService->detector();

  return std::make_shared<Acts::DD4hepFieldAdapter>(detector.field());
}

}  // namespace ActsExamples::DD4hep
