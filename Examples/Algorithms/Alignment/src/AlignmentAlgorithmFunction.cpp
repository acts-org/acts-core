// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Propagator/EigenStepper.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/TrackFitting/GainMatrixSmoother.hpp"
#include "Acts/TrackFitting/GainMatrixUpdater.hpp"
#include "ActsAlignment/Kernel/Alignment.hpp"
#include "ActsExamples/Alignment/AlignmentAlgorithm.hpp"
#include "ActsExamples/MagneticField/MagneticField.hpp"

namespace {

using Updater = Acts::GainMatrixUpdater;
using Smoother = Acts::GainMatrixSmoother;
using Stepper = Acts::EigenStepper<>;
using Propagator = Acts::Propagator<Stepper, Acts::Navigator>;
using Fitter = Acts::KalmanFitter<Propagator, Acts::VectorMultiTrajectory>;
using Alignment = ActsAlignment::Alignment<Fitter>;

struct AlignmentFunctionImpl
    : public ActsExamples::AlignmentAlgorithm::AlignmentFunction {
  Alignment align;

  AlignmentFunctionImpl(Alignment&& a) : align(std::move(a)) {}

  ActsExamples::AlignmentAlgorithm::AlignmentResult operator()(
      const std::vector<std::vector<ActsExamples::IndexSourceLink>>&
          sourceLinks,
      const ActsExamples::TrackParametersContainer& initialParameters,
      const ActsAlignment::AlignmentOptions<
          ActsExamples::AlignmentAlgorithm::TrackFitterOptions>& options)
      const override {
    return align.align(sourceLinks, initialParameters, options);
  };
};
}  // namespace

std::shared_ptr<ActsExamples::AlignmentAlgorithm::AlignmentFunction>
ActsExamples::AlignmentAlgorithm::makeAlignmentFunction(
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry,
    std::shared_ptr<const Acts::MagneticFieldProvider> magneticField) {
  Stepper stepper(std::move(magneticField));
  Acts::Navigator::Config cfg{std::move(trackingGeometry)};
  cfg.resolvePassive = false;
  cfg.resolveMaterial = true;
  cfg.resolveSensitive = true;
  Acts::Navigator navigator(cfg);
  Propagator propagator(std::move(stepper), std::move(navigator));
  Fitter trackFitter(std::move(propagator));
  Alignment alignment(std::move(trackFitter));

  // build the alignment functions. owns the alignment object.
  return std::make_shared<AlignmentFunctionImpl>(std::move(alignment));
}
