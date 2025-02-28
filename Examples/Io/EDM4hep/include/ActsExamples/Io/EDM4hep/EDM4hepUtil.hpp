// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
#pragma once

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Plugins/EDM4hep/TrackerHitCompatibility.hpp"
#include "ActsExamples/EventData/Cluster.hpp"
#include "ActsExamples/EventData/Measurement.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/EventData/Trajectories.hpp"
#include "ActsFatras/EventData/Hit.hpp"

#include <functional>

#include <edm4hep/MCParticle.h>
#include <edm4hep/MutableMCParticle.h>
#include <edm4hep/MutableSimTrackerHit.h>
#include <edm4hep/MutableTrack.h>
#include <edm4hep/MutableTrackerHitPlane.h>
#include <edm4hep/SimTrackerHit.h>
#include <edm4hep/TrackerHitPlane.h>

namespace ActsExamples::EDM4hepUtil {

using MapParticleIdFrom =
    std::function<ActsFatras::Barcode(const edm4hep::MCParticle& particle)>;
using MapParticleIdTo =
    std::function<edm4hep::MCParticle(ActsFatras::Barcode particleId)>;

inline ActsFatras::Barcode zeroParticleMapper(
    const edm4hep::MCParticle& /*particle*/) {
  return ActsFatras::Barcode();
}

using MapGeometryIdFrom =
    std::function<Acts::GeometryIdentifier(std::uint64_t cellId)>;
using MapGeometryIdTo =
    std::function<std::uint64_t(Acts::GeometryIdentifier geometryId)>;

/// Reads a Fatras particle from EDM4hep.
///
/// Inpersistent information:
/// - particle ID
/// - process
SimParticle readParticle(
    const edm4hep::MCParticle& from,
    const MapParticleIdFrom& particleMapper = zeroParticleMapper);

/// Write a Fatras particle into EDM4hep.
///
/// Inpersistent information:
/// - particle ID
/// - process
void writeParticle(const SimParticle& from, edm4hep::MutableMCParticle to);

/// Reads a Fatras hit from EDM4hep.
///
/// Inpersistent information:
/// - after4 momentum
/// - hit index
/// - digitization channel
ActsFatras::Hit readSimHit(const edm4hep::SimTrackerHit& from,
                           const MapParticleIdFrom& particleMapper,
                           const MapGeometryIdFrom& geometryMapper);

/// Writes a Fatras hit to EDM4hep.
///
/// Inpersistent information:
/// - after4 momentum
/// - hit index
/// - digitization channel
void writeSimHit(const ActsFatras::Hit& from, edm4hep::MutableSimTrackerHit to,
                 const MapParticleIdTo& particleMapper,
                 const MapGeometryIdTo& geometryMapper);

/// Reads a measurement cluster from EDM4hep.
///
/// Inpersistent information:
/// - hit index
/// - 1D local coords?
/// - segment path
///
/// Known issues:
/// - cluster channels are read from inappropriate fields
/// - local 2D coordinates and time are read from position
VariableBoundMeasurementProxy readMeasurement(
    MeasurementContainer& container, const edm4hep::TrackerHitPlane& from,
    const edm4hep::TrackerHit3DCollection* fromClusters, Cluster* toCluster,
    const MapGeometryIdFrom& geometryMapper);

/// Writes a measurement cluster to EDM4hep.
///
/// Inpersistent information:
/// - hit index
/// - 1D local coords?
/// - segment path
///
/// Known issues:
/// - cluster channels are written to inappropriate fields
/// - local 2D coordinates and time are written to position
void writeMeasurement(const ConstVariableBoundMeasurementProxy& from,
                      edm4hep::MutableTrackerHitPlane to,
                      const Cluster* fromCluster,
                      edm4hep::TrackerHit3DCollection& toClusters,
                      const MapGeometryIdTo& geometryMapper);

/// Writes a trajectory to EDM4hep.
///
/// Inpersistent information:
/// - trajectory state incomplete
/// - relation to the particles
void writeTrajectory(const Acts::GeometryContext& gctx, double Bz,
                     const Trajectories& from, edm4hep::MutableTrack to,
                     std::size_t fromIndex,
                     const Acts::ParticleHypothesis& particleHypothesis,
                     const IndexMultimap<ActsFatras::Barcode>& hitParticlesMap);

/// Helper function to either return an id as is, or unpack an index from it
/// if it is a podio::ObjectID.
/// @tparam T The type of the id.
/// @param o The id to convert.
/// @return The id as an unsigned integer.
template <typename T>
std::uint64_t podioObjectIDToInteger(T&& o) {
  if constexpr (!std::is_same_v<T, podio::ObjectID>) {
    return o;
  } else {
    return o.index;
  }
}

}  // namespace ActsExamples::EDM4hepUtil
