// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// TODO We still use some Kalman Fitter functionalities. Check for replacement

#include "Acts/Definitions/Direction.hpp"
#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/TrackContainer.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/EventData/VectorMultiTrajectory.hpp"
#include "Acts/EventData/VectorTrackContainer.hpp"
#include "Acts/EventData/detail/CorrectedTransformationFreeToBound.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Propagator/DirectNavigator.hpp"
#include "Acts/Propagator/Navigator.hpp"
#include "Acts/Propagator/Propagator.hpp"
#include "Acts/Propagator/SympyStepper.hpp"
#include "Acts/TrackFitting/GlobalChiSquareFitter.hpp"
#include "Acts/TrackFitting/KalmanFitter.hpp"
#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/MeasurementCalibration.hpp"
#include "ActsExamples/EventData/Track.hpp"
#include "ActsExamples/TrackFitting/RefittingCalibrator.hpp"
#include "ActsExamples/TrackFitting/TrackFitterFunction.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace Acts {
class MagneticFieldProvider;
class SourceLink;
class Surface;
class TrackingGeometry;
}  // namespace Acts

namespace {

using Stepper = Acts::SympyStepper;
using Propagator = Acts::Propagator<Stepper, Acts::Navigator>;
using Fitter =
    Acts::Experimental::Gx2Fitter<Propagator, Acts::VectorMultiTrajectory>;
using DirectPropagator = Acts::Propagator<Stepper, Acts::DirectNavigator>;
using DirectFitter =
    Acts::KalmanFitter<DirectPropagator, Acts::VectorMultiTrajectory>;

using TrackContainer =
    Acts::TrackContainer<Acts::VectorTrackContainer,
                         Acts::VectorMultiTrajectory, std::shared_ptr>;

using namespace ActsExamples;

struct GlobalChiSquareFitterFunctionImpl final : public TrackFitterFunction {
  Fitter fitter;
  DirectFitter directFitter;

  bool multipleScattering = false;
  bool energyLoss = false;
  Acts::FreeToBoundCorrection freeToBoundCorrection;
  std::size_t nUpdateMax = 5;
  double relChi2changeCutOff = 1e-7;

  IndexSourceLink::SurfaceAccessor m_slSurfaceAccessor;

  GlobalChiSquareFitterFunctionImpl(Fitter&& f, DirectFitter&& df,
                                    const Acts::TrackingGeometry& trkGeo)
      : fitter(std::move(f)),
        directFitter(std::move(df)),
        m_slSurfaceAccessor{trkGeo} {}

  template <typename calibrator_t>
  auto makeGx2fOptions(const GeneralFitterOptions& options,
                       const calibrator_t& calibrator) const {
    Acts::Experimental::Gx2FitterExtensions<Acts::VectorMultiTrajectory>
        extensions;
    extensions.calibrator.connect<&calibrator_t::calibrate>(&calibrator);

    if (options.doRefit) {
      extensions.surfaceAccessor.connect<&RefittingCalibrator::accessSurface>();
    } else {
      extensions.surfaceAccessor
          .connect<&IndexSourceLink::SurfaceAccessor::operator()>(
              &m_slSurfaceAccessor);
    }

    const Acts::Experimental::Gx2FitterOptions gx2fOptions(
        options.geoContext, options.magFieldContext, options.calibrationContext,
        extensions, options.propOptions, &(*options.referenceSurface),
        multipleScattering, energyLoss, freeToBoundCorrection, nUpdateMax,
        relChi2changeCutOff);

    return gx2fOptions;
  }

  TrackFitterResult operator()(const std::vector<Acts::SourceLink>& sourceLinks,
                               const TrackParameters& initialParameters,
                               const GeneralFitterOptions& options,
                               const MeasurementCalibratorAdapter& calibrator,
                               TrackContainer& tracks) const override {
    const auto gx2fOptions = makeGx2fOptions(options, calibrator);
    return fitter.fit(sourceLinks.begin(), sourceLinks.end(), initialParameters,
                      gx2fOptions, tracks);
  }

  // We need a placeholder for the directNavigator overload. Otherwise, we would
  // have an unimplemented pure virtual method in a final class.
  TrackFitterResult operator()(
      const std::vector<Acts::SourceLink>& /*sourceLinks*/,
      const TrackParameters& /*initialParameters*/,
      const GeneralFitterOptions& /*options*/,
      const RefittingCalibrator& /*calibrator*/,
      const std::vector<const Acts::Surface*>& /*surfaceSequence*/,
      TrackContainer& /*tracks*/) const override {
    throw std::runtime_error(
        "direct navigation with GX2 fitter is not implemented");
  }
};

}  // namespace

std::shared_ptr<ActsExamples::TrackFitterFunction>
ActsExamples::makeGlobalChiSquareFitterFunction(
    std::shared_ptr<const Acts::TrackingGeometry> trackingGeometry,
    std::shared_ptr<const Acts::MagneticFieldProvider> magneticField,
    bool multipleScattering, bool energyLoss,
    Acts::FreeToBoundCorrection freeToBoundCorrection, std::size_t nUpdateMax,
    double relChi2changeCutOff, const Acts::Logger& logger) {
  // Stepper should be copied into the fitters
  const Stepper stepper(std::move(magneticField));

  // Standard fitter
  const auto& geo = *trackingGeometry;
  Acts::Navigator::Config cfg{std::move(trackingGeometry)};
  cfg.resolvePassive = false;
  cfg.resolveMaterial = true;
  cfg.resolveSensitive = true;
  Acts::Navigator navigator(cfg, logger.cloneWithSuffix("Navigator"));
  Propagator propagator(stepper, std::move(navigator),
                        logger.cloneWithSuffix("Propagator"));
  Fitter trackFitter(std::move(propagator), logger.cloneWithSuffix("Fitter"));

  // Direct fitter
  Acts::DirectNavigator directNavigator{
      logger.cloneWithSuffix("DirectNavigator")};
  DirectPropagator directPropagator(stepper, std::move(directNavigator),
                                    logger.cloneWithSuffix("DirectPropagator"));
  DirectFitter directTrackFitter(std::move(directPropagator),
                                 logger.cloneWithSuffix("DirectFitter"));

  // build the fitter function. owns the fitter object.
  auto fitterFunction = std::make_shared<GlobalChiSquareFitterFunctionImpl>(
      std::move(trackFitter), std::move(directTrackFitter), geo);
  fitterFunction->multipleScattering = multipleScattering;
  fitterFunction->energyLoss = energyLoss;
  fitterFunction->freeToBoundCorrection = freeToBoundCorrection;
  fitterFunction->nUpdateMax = nUpdateMax;
  fitterFunction->relChi2changeCutOff = relChi2changeCutOff;

  return fitterFunction;
}
