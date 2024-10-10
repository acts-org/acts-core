// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/EDM4hep/EDM4hepUtil.hpp"

#include "Acts/Definitions/Common.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/EventData/Charge.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/MultiTrajectoryHelpers.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/EDM4hep/EDM4hepUtil.hpp"
#include "ActsExamples/Digitization/MeasurementCreation.hpp"
#include "ActsExamples/EventData/Index.hpp"
#include "ActsExamples/EventData/IndexSourceLink.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimHit.hpp"
#include "ActsExamples/Validation/TrackClassification.hpp"

#include "edm4hep/TrackState.h"

using namespace Acts::UnitLiterals;

namespace ActsExamples {

ActsFatras::Particle EDM4hepUtil::readParticle(
    const edm4hep::MCParticle& from, const MapParticleIdFrom& particleMapper) {
  ActsFatras::Barcode particleId = particleMapper(from);

  ActsFatras::Particle to(particleId,
                          static_cast<Acts::PdgParticle>(from.getPDG()),
                          from.getCharge() * Acts::UnitConstants::e,
                          from.getMass() * Acts::UnitConstants::GeV);

  // TODO do we have that in EDM4hep?
  // particle.setProcess(static_cast<ActsFatras::ProcessType>(data.process));

  to.setPosition4(from.getVertex()[0] * Acts::UnitConstants::mm,
                  from.getVertex()[1] * Acts::UnitConstants::mm,
                  from.getVertex()[2] * Acts::UnitConstants::mm,
                  from.getTime() * Acts::UnitConstants::ns);

  // Only used for direction; normalization/units do not matter
  Acts::Vector3 momentum = {from.getMomentum()[0], from.getMomentum()[1],
                            from.getMomentum()[2]};
  to.setDirection(momentum.normalized());

  to.setAbsoluteMomentum(momentum.norm() * 1_GeV);

  return to;
}

void EDM4hepUtil::writeParticle(const ActsFatras::Particle& from,
                                edm4hep::MutableMCParticle to) {
  // TODO what about particleId?

  to.setPDG(from.pdg());
  to.setCharge(from.charge() / Acts::UnitConstants::e);
  to.setMass(from.mass() / Acts::UnitConstants::GeV);
  to.setVertex({from.position().x(), from.position().y(), from.position().z()});
  to.setMomentum({static_cast<float>(from.fourMomentum().x()),
                  static_cast<float>(from.fourMomentum().y()),
                  static_cast<float>(from.fourMomentum().z())});
}

ActsFatras::Hit EDM4hepUtil::readSimHit(
    const edm4hep::SimTrackerHit& from, const MapParticleIdFrom& particleMapper,
    const MapGeometryIdFrom& geometryMapper) {
  ActsFatras::Barcode particleId = particleMapper(from.getMCParticle());

  const auto mass = from.getMCParticle().getMass() * 1_GeV;
  const Acts::Vector3 momentum{
      from.getMomentum().x * 1_GeV,
      from.getMomentum().y * 1_GeV,
      from.getMomentum().z * 1_GeV,
  };
  const auto energy = std::hypot(momentum.norm(), mass);

  Acts::Vector4 pos4{
      from.getPosition().x * 1_mm,
      from.getPosition().y * 1_mm,
      from.getPosition().z * 1_mm,
      from.getTime() * 1_ns,
  };

  Acts::Vector4 mom4{
      momentum.x(),
      momentum.y(),
      momentum.z(),
      energy,
  };

  Acts::Vector4 delta4 = Acts::Vector4::Zero();
  delta4[Acts::eEnergy] = -from.getEDep() * Acts::UnitConstants::GeV;

  Acts::GeometryIdentifier geometryId = geometryMapper(from.getCellID());

  // Can extract from time, but we need a complete picture of the trajectory
  // first
  std::int32_t index = -1;

  return ActsFatras::Hit(geometryId, particleId, pos4, mom4, mom4 + delta4,
                         index);
}

void EDM4hepUtil::writeSimHit(const ActsFatras::Hit& from,
                              edm4hep::MutableSimTrackerHit to,
                              const MapParticleIdTo& particleMapper,
                              const MapGeometryIdTo& geometryMapper) {
  const Acts::Vector4& globalPos4 = from.fourPosition();
  const Acts::Vector4& momentum4Before = from.momentum4Before();
  const auto delta4 = from.momentum4After() - momentum4Before;

  if (particleMapper) {
    to.setMCParticle(particleMapper(from.particleId()));
  }

  if (geometryMapper) {
    // TODO what about the digitization?
    to.setCellID(geometryMapper(from.geometryId()));
  }

  to.setTime(globalPos4[Acts::eTime] / Acts::UnitConstants::ns);

  to.setPosition({
      globalPos4[Acts::ePos0] / Acts::UnitConstants::mm,
      globalPos4[Acts::ePos1] / Acts::UnitConstants::mm,
      globalPos4[Acts::ePos2] / Acts::UnitConstants::mm,
  });

  to.setMomentum({
      static_cast<float>(momentum4Before[Acts::eMom0] /
                         Acts::UnitConstants::GeV),
      static_cast<float>(momentum4Before[Acts::eMom1] /
                         Acts::UnitConstants::GeV),
      static_cast<float>(momentum4Before[Acts::eMom2] /
                         Acts::UnitConstants::GeV),
  });

  to.setEDep(-delta4[Acts::eEnergy] / Acts::UnitConstants::GeV);
}

VariableBoundMeasurementProxy EDM4hepUtil::readMeasurement(
    MeasurementContainer& container, const edm4hep::TrackerHitPlane& from,
    const edm4hep::TrackerHitCollection* fromClusters, Cluster* toCluster,
    const MapGeometryIdFrom& geometryMapper) {
  // no need for digitization as we only want to identify the sensor
  Acts::GeometryIdentifier geometryId = geometryMapper(from.getCellID());

  IndexSourceLink sourceLink{
      geometryId, static_cast<Index>(podioObjectIDToInteger(from.id()))};

  auto pos = from.getPosition();
  auto cov = from.getCovMatrix();

  DigitizedParameters dParameters;

  dParameters.indices.push_back(Acts::eBoundLoc0);
  dParameters.values.push_back(pos.x);
  dParameters.variances.push_back(cov[0]);

  // TODO cut this out for 1D
  dParameters.indices.push_back(Acts::eBoundLoc1);
  dParameters.values.push_back(pos.y);
  dParameters.variances.push_back(cov[2]);

  dParameters.indices.push_back(Acts::eBoundTime);
  dParameters.values.push_back(pos.z);
  dParameters.variances.push_back(cov[5]);

  auto to = createMeasurement(container, dParameters, sourceLink);

  if (fromClusters != nullptr) {
    for (const auto objectId : from.getRawHits()) {
      const auto& c = fromClusters->at(objectId.index);

      // TODO get EDM4hep fixed
      // misusing some fields to store ACTS specific information
      // don't ask ...
      ActsFatras::Segmentizer::Bin2D bin{
          static_cast<unsigned int>(c.getType()),
          static_cast<unsigned int>(c.getQuality())};
      ActsFatras::Segmentizer::Segment2D path2D{
          {Acts::Vector2::Zero(), Acts::Vector2::Zero()}};
      double activation = c.getTime();
      ActsFatras::Segmentizer::ChannelSegment cell{bin, path2D, activation};

      toCluster->channels.push_back(cell);
    }
  }

  return to;
}

void EDM4hepUtil::writeMeasurement(
    const ConstVariableBoundMeasurementProxy& from,
    edm4hep::MutableTrackerHitPlane to, const Cluster* fromCluster,
    edm4hep::TrackerHitCollection& toClusters,
    const MapGeometryIdTo& geometryMapper) {
  Acts::GeometryIdentifier geoId =
      from.sourceLink().template get<IndexSourceLink>().geometryId();

  if (geometryMapper) {
    // no need for digitization as we only want to identify the sensor
    to.setCellID(geometryMapper(geoId));
  }

  const auto& parameters = from.fullParameters();
  const auto& covariance = from.fullCovariance();

  to.setTime(parameters[Acts::eBoundTime] / Acts::UnitConstants::ns);

  to.setType(Acts::EDM4hepUtil::EDM4HEP_ACTS_POSITION_TYPE);
  // TODO set uv (which are in global spherical coordinates with r=1)
  to.setPosition({parameters[Acts::eBoundLoc0], parameters[Acts::eBoundLoc1],
                  parameters[Acts::eBoundTime]});

  to.setCovMatrix({
      static_cast<float>(covariance(Acts::eBoundLoc0, Acts::eBoundLoc0)),
      static_cast<float>(covariance(Acts::eBoundLoc1, Acts::eBoundLoc0)),
      static_cast<float>(covariance(Acts::eBoundLoc1, Acts::eBoundLoc1)),
      0,
      0,
      0,
  });

  if (fromCluster != nullptr) {
    for (const auto& c : fromCluster->channels) {
      auto toChannel = toClusters.create();
      to.addToRawHits(toChannel.getObjectID());

      // TODO digitization channel

      // TODO get EDM4hep fixed
      // misusing some fields to store ACTS specific information
      // don't ask ...
      toChannel.setType(c.bin[0]);
      toChannel.setQuality(c.bin[1]);
      toChannel.setTime(c.activation);
    }
  }
}

void EDM4hepUtil::writeTrajectory(
    const Acts::GeometryContext& gctx, double Bz, const Trajectories& from,
    edm4hep::MutableTrack to, std::size_t fromIndex,
    const Acts::ParticleHypothesis& particleHypothesis,
    const IndexMultimap<ActsFatras::Barcode>& hitParticlesMap) {
  const auto& multiTrajectory = from.multiTrajectory();
  auto trajectoryState =
      Acts::MultiTrajectoryHelpers::trajectoryState(multiTrajectory, fromIndex);

  std::vector<ParticleHitCount> particleHitCount;
  identifyContributingParticles(hitParticlesMap, from, fromIndex,
                                particleHitCount);
  // TODO use particles

  // TODO write track params
  // auto trackParameters = from.trackParameters(fromIndex);

  to.setChi2(trajectoryState.chi2Sum / trajectoryState.NDF);
  to.setNdf(trajectoryState.NDF);

  multiTrajectory.visitBackwards(fromIndex, [&](const auto& state) {
    // we only fill the track states with non-outlier measurement
    auto typeFlags = state.typeFlags();
    if (!typeFlags.test(Acts::TrackStateFlag::MeasurementFlag)) {
      return true;
    }

    edm4hep::TrackState trackState;

    Acts::BoundTrackParameters parObj{state.referenceSurface().getSharedPtr(),
                                      state.parameters(), state.covariance(),
                                      particleHypothesis};

    // Convert to LCIO track parametrization expected by EDM4hep
    // This will create an ad-hoc perigee surface if the input parameters are
    // not bound on a perigee surface already
    Acts::EDM4hepUtil::detail::Parameters converted =
        Acts::EDM4hepUtil::detail::convertTrackParametersToEdm4hep(gctx, Bz,
                                                                   parObj);

    trackState.D0 = converted.values[0];
    trackState.Z0 = converted.values[1];
    trackState.phi = converted.values[2];
    trackState.tanLambda = converted.values[3];
    trackState.omega = converted.values[4];
    trackState.time = converted.values[5];

    // Converted parameters are relative to an ad-hoc perigee surface created at
    // the hit location
    auto center = converted.surface->center(gctx);
    trackState.referencePoint.x = center.x();
    trackState.referencePoint.y = center.y();
    trackState.referencePoint.z = center.z();

    if (converted.covariance) {
      const auto& c = converted.covariance.value();

      trackState.covMatrix = {
          static_cast<float>(c(0, 0)), static_cast<float>(c(1, 0)),
          static_cast<float>(c(1, 1)), static_cast<float>(c(2, 0)),
          static_cast<float>(c(2, 1)), static_cast<float>(c(2, 2)),
          static_cast<float>(c(3, 0)), static_cast<float>(c(3, 1)),
          static_cast<float>(c(3, 2)), static_cast<float>(c(3, 3)),
          static_cast<float>(c(4, 0)), static_cast<float>(c(4, 1)),
          static_cast<float>(c(4, 2)), static_cast<float>(c(4, 3)),
          static_cast<float>(c(4, 4))};
    }

    to.addToTrackStates(trackState);

    return true;
  });
}

}  // namespace ActsExamples
