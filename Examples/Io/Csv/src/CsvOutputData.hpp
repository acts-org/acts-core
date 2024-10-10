// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <ActsExamples/EventData/Index.hpp>

#include <cstdint>

namespace ActsExamples {

struct ParticleData {
  /// Event-unique particle identifier a.k.a barcode.
  std::uint64_t particle_id = 0;
  /// Particle type number a.k.a. PDG particle number.
  std::int32_t particle_type = 0;
  /// Production process type. Not available in the TrackML datasets.
  std::uint32_t process = 0u;
  /// Production position components in mm.
  float vx = 0, vy = 0, vz = 0;
  // Production time in ns. Not available in the TrackML datasets.
  float vt = 0.0f;
  /// Momentum components in GeV.
  float px = 0, py = 0, pz = 0;
  /// Mass in GeV. Not available in the TrackML datasets
  float m = 0.0f;
  /// Charge in e.
  float q = 0;

  DFE_NAMEDTUPLE(ParticleData, particle_id, particle_type, process, vx, vy, vz,
                 vt, px, py, pz, m, q);
};

// Write out simhits before digitization (no hi_id associated)
struct SimHitData {
  /// Hit surface identifier. Not available in the TrackML datasets.
  std::uint64_t geometry_id = 0u;
  /// Event-unique particle identifier of the generating particle.
  std::uint64_t particle_id = 0;
  /// True global hit position components in mm.
  float tx = 0, ty = 0, tz = 0;
  // True global hit time in ns. Not available in the TrackML datasets.
  float tt = 0.0f;
  /// True particle momentum in GeV before interaction.
  float tpx = 0, tpy = 0, tpz = 0;
  /// True particle energy in GeV before interaction.
  /// Not available in the TrackML datasets.
  float te = 0.0f;
  /// True four-momentum change in GeV due to interaction.
  /// Not available in the TrackML datasets.
  float deltapx = 0.0f;
  float deltapy = 0.0f;
  float deltapz = 0.0f;
  float deltae = 0.0f;
  // Hit index along the trajectory. Not available in the TrackML datasets.
  std::int32_t index = -1;

  DFE_NAMEDTUPLE(SimHitData, particle_id, geometry_id, tx, ty, tz, tt, tpx, tpy,
                 tpz, te, deltapx, deltapy, deltapz, deltae, index);
};

// Write out muon simhits before digitization
struct MuonSimHitData {
  /// Hit surface identifier. Not available in the TrackML datasets.
  int pdgId = 0;
  /// three components of the muon station identifier
  int StationName = 0;
  int StationEta = 0;
  int StationPhi = 0;
  // True hit location in station frame, in mm.
  float LocalPositionExtrx = 0.0f, LocalPositionExtry = 0.0f,
        LocalPositionExtrz = 0.0f;
  /// True particle momentum in GeV before interaction.
  float LocalDirectionx = 0.0f, LocalDirectiony = 0.0f, LocalDirectionz = 0.0f;
  DFE_NAMEDTUPLE(MuonSimHitData, pdgId, StationName, StationEta, StationPhi,
                 LocalPositionExtrx, LocalPositionExtry, LocalPositionExtrz,
                 LocalDirectionx, LocalDirectiony, LocalDirectionz);
};

// Write out muon simhits before digitization
struct MuonDriftCircleData {
  /// Drift radius, in mm.
  float driftRadius = 0.0f;
  /// Drift tube center location in the station frame
  float tubePositionx = 0.0f, tubePositiony = 0.0f, tubePositionz = 0.0f;
  /// three components of the muon station identifier
  int stationName = 0;
  int stationEta = 0;
  int stationPhi = 0;
  // components of the tube identifier within the station
  int multilayer = 0;
  int tubelayer = 0;
  int tube = 0;

  DFE_NAMEDTUPLE(MuonDriftCircleData, driftRadius, tubePositionx, tubePositiony,
                 tubePositionz, stationName, stationEta, stationPhi, multilayer,
                 tubelayer, tube);
};

struct TruthHitData {
  /// Event-unique hit identifier. As defined for the simulated hit below and
  /// used to link back to it; same value can appear multiple times here due to
  /// shared hits in dense environments.
  std::uint64_t hit_id = 0;
  /// Hit surface identifier. Not available in the TrackML datasets.
  std::uint64_t geometry_id = 0u;
  /// Event-unique particle identifier of the generating particle.
  std::uint64_t particle_id = 0;
  /// True global hit position components in mm.
  float tx = 0, ty = 0, tz = 0;
  // True global hit time in ns. Not available in the TrackML datasets.
  float tt = 0.0f;
  /// True particle momentum in GeV before interaction.
  float tpx = 0, tpy = 0, tpz = 0;
  /// True particle energy in GeV before interaction.
  /// Not available in the TrackML datasets.
  float te = 0.0f;
  /// True four-momentum change in GeV due to interaction.
  /// Not available in the TrackML datasets.
  float deltapx = 0.0f;
  float deltapy = 0.0f;
  float deltapz = 0.0f;
  float deltae = 0.0f;
  // Hit index along the trajectory. Not available in the TrackML datasets.
  std::int32_t index = -1;

  DFE_NAMEDTUPLE(TruthHitData, hit_id, particle_id, geometry_id, tx, ty, tz, tt,
                 tpx, tpy, tpz, te, deltapx, deltapy, deltapz, deltae, index);
};

struct HitData {
  /// Event-unique hit identifier. Each value can appear at most once.
  std::uint64_t hit_id = 0;
  /// Hit surface identifier. Not available in the TrackML datasets.
  std::uint64_t geometry_id = 0u;
  /// Global hit position components in mm.
  float x = 0, y = 0, z = 0;
  /// Global hit time in ns. Not available in the TrackML datasets.
  float t = 0.0f;

  DFE_NAMEDTUPLE(HitData, hit_id, geometry_id, x, y, z, t);
};

struct MeasurementSimHitLink {
  /// Event-unique measurement identifier. Each value can appear at most once.
  std::uint64_t measurement_id = 0;
  /// Event-unique measurement sim hit identifier.
  std::uint64_t hit_id = 0;

  DFE_NAMEDTUPLE(MeasurementSimHitLink, measurement_id, hit_id);
};

struct MeasurementData {
  /// Event-unique measurement identifier. Each value can appear at most once.
  std::uint64_t measurement_id = 0;
  /// Hit surface identifier.
  std::uint64_t geometry_id = 0u;
  /// Local hit information - bit identification what's measured
  std::uint8_t local_key = 0;
  float local0 = 0, local1 = 0, phi = 0, theta = 0, time = 0;
  float var_local0 = 0, var_local1 = 0, var_phi = 0, var_theta = 0,
        var_time = 0;

  DFE_NAMEDTUPLE(MeasurementData, measurement_id, geometry_id, local_key,
                 local0, local1, phi, theta, time, var_local0, var_local1,
                 var_phi, var_theta, var_time);
};

struct CellData {
  /// Hit surface identifier.
  std::uint64_t geometry_id = 0u;
  /// Event-unique measurement identifier. As defined for the measurement above
  /// and used to link back to it; same value can appear multiple times for
  /// clusters with more than one active cell.
  std::uint64_t measurement_id = 0;
  /// Digital cell address/ channel
  std::int32_t channel0 = 0, channel1 = 0;
  /// Digital cell timestamp. Not available in the TrackML datasets.
  float timestamp = 0;
  /// (Digital) measured cell value, e.g. amplitude or time-over-threshold.
  float value = 0;

  DFE_NAMEDTUPLE(CellData, geometry_id, measurement_id, channel0, channel1,
                 timestamp, value);
};

// uses hit id
struct CellDataLegacy {
  /// Hit surface identifier.
  std::uint64_t geometry_id = 0u;
  /// Event-unique measurement identifier. As defined for the measurement above
  /// and used to link back to it; same value can appear multiple times for
  /// clusters with more than one active cell.
  std::uint64_t hit_id = 0;
  /// Digital cell address/ channel
  std::int32_t channel0 = 0, channel1 = 0;
  /// Digital cell timestamp. Not available in the TrackML datasets.
  float timestamp = 0;
  /// (Digital) measured cell value, e.g. amplitude or time-over-threshold.
  float value = 0;

  DFE_NAMEDTUPLE(CellDataLegacy, geometry_id, hit_id, channel0, channel1,
                 timestamp, value);
};

struct SurfaceData {
  /// Surface identifier. Not available in the TrackML datasets.
  std::uint64_t geometry_id = 0;
  /// Partially decoded surface identifier components.
  std::uint32_t volume_id = 0, boundary_id = 0, layer_id = 0, module_id = 0;
  /// Center position components in mm.
  float cx = 0, cy = 0, cz = 0;
  /// Rotation matrix components.
  float rot_xu = 0, rot_xv = 0, rot_xw = 0;
  float rot_yu = 0, rot_yv = 0, rot_yw = 0;
  float rot_zu = 0, rot_zv = 0, rot_zw = 0;
  /// The type of the surface bpounds object, determines the parameters filled
  int bounds_type = 0;
  float bound_param0 = -1.f;
  float bound_param1 = -1.f;
  float bound_param2 = -1.f;
  float bound_param3 = -1.f;
  float bound_param4 = -1.f;
  float bound_param5 = -1.f;
  float bound_param6 = -1.f;

  float module_t = -1.f;
  float pitch_u = -1.f;
  float pitch_v = -1.f;

  DFE_NAMEDTUPLE(SurfaceData, geometry_id, volume_id, boundary_id, layer_id,
                 module_id, cx, cy, cz, rot_xu, rot_xv, rot_xw, rot_yu, rot_yv,
                 rot_yw, rot_zu, rot_zv, rot_zw, bounds_type, bound_param0,
                 bound_param1, bound_param2, bound_param3, bound_param4,
                 bound_param5, bound_param6, module_t, pitch_u, pitch_v);
};

struct LayerVolumeData {
  /// Surface identifier. Not available in the TrackML datasets.
  std::uint64_t geometry_id = 0;
  /// Partially decoded surface identifier components.
  std::uint32_t volume_id = 0, layer_id = 0;
  /// The type of the surface bpounds object, determines the parameters filled
  int volume_type = 0;
  float min_v0 = -1.f;
  float max_v0 = -1.f;
  float min_v1 = -1.f;
  float max_v1 = -1.f;
  float min_v2 = -1.f;
  float max_v2 = -1.f;

  DFE_NAMEDTUPLE(LayerVolumeData, geometry_id, volume_id, layer_id, min_v0,
                 max_v0, min_v1, max_v1, min_v2, max_v2);
};

struct SpacePointData {
  /// Event-unique measurement identifier. Each value can appear at most once.
  std::uint64_t measurement_id = 0;
  /// Space point information
  float sp_x = 0, sp_y = 0, sp_z = 0, sp_radius = 0;
  float sp_covr = 0, sp_covz = 0;

  // half of the length of the top strip
  float sp_topHalfStripLength = 0;
  // half of the length of the bottom strip
  float sp_bottomHalfStripLength = 0;
  // direction of the top strip
  Acts::Vector3 sp_topStripDirection;
  // direction of the bottom strip
  Acts::Vector3 sp_bottomStripDirection;
  // distance between the center of the two strips
  Acts::Vector3 sp_stripCenterDistance;
  // position of the center of the bottom strip
  Acts::Vector3 sp_topStripCenterPosition;

  DFE_NAMEDTUPLE(SpacePointData, measurement_id, sp_x, sp_y, sp_z, sp_radius,
                 sp_covr, sp_covz, sp_topHalfStripLength,
                 sp_bottomHalfStripLength, sp_topStripDirection[0],
                 sp_topStripDirection[1], sp_topStripDirection[2],
                 sp_bottomStripDirection[0], sp_bottomStripDirection[1],
                 sp_bottomStripDirection[2], sp_stripCenterDistance[0],
                 sp_stripCenterDistance[1], sp_stripCenterDistance[2],
                 sp_topStripCenterPosition[0], sp_topStripCenterPosition[1],
                 sp_topStripCenterPosition[2]);
};

struct SurfaceGridData {
  /// Surface identifier. Not available in the TrackML datasets.
  std::uint64_t geometry_id = 0;
  /// Partially decoded surface identifier components.
  std::uint32_t volume_id = 0, layer_id = 0, surface_id = 0;
  /// The number of bins in loc 0 / 1
  int type_loc0 = -1;
  int nbins_loc0 = -1;
  float min_loc0 = 0, max_loc0 = 0;
  int type_loc1 = -1;
  int nbins_loc1 = -1;
  float min_loc1 = 0, max_loc1 = 0;

  DFE_NAMEDTUPLE(SurfaceGridData, geometry_id, volume_id, layer_id, surface_id,
                 type_loc0, nbins_loc0, min_loc0, max_loc0, type_loc1,
                 nbins_loc1, min_loc1, max_loc1);
};

struct SpacepointData {
  std::uint64_t measurement_id;
  std::uint64_t geometry_id;
  float x, y, z;
  float var_r, var_z;
  DFE_NAMEDTUPLE(SpacepointData, measurement_id, geometry_id, x, y, z, var_r,
                 var_z);
};

struct TrackParameterData {
  double d0;
  double z0;
  double phi;
  double theta;
  double qop;

  double var_d0, var_z0, var_phi, var_theta, var_qop;

  double cov_d0z0, cov_d0phi, cov_d0theta, cov_d0qop;
  double cov_z0d0, cov_z0phi, cov_z0theta, cov_z0qop;
  double cov_phid0, cov_phiz0, cov_phitheta, cov_phiqop;
  double cov_thetad0, cov_thetaz0, cov_thetaphi, cov_thetaqop;
  double cov_qopd0, cov_qopz0, cov_qopphi, cov_qoptheta;

  DFE_NAMEDTUPLE(TrackParameterData, d0, z0, phi, theta, qop, var_d0, var_z0,
                 var_phi, var_theta, var_qop, cov_d0z0, cov_d0phi, cov_d0theta,
                 cov_d0qop, cov_z0d0, cov_z0phi, cov_z0theta, cov_z0qop,
                 cov_phid0, cov_phiz0, cov_phitheta, cov_phiqop, cov_thetad0,
                 cov_thetaz0, cov_thetaphi, cov_thetaqop, cov_qopd0, cov_qopz0,
                 cov_qopphi, cov_qoptheta);
};

struct ProtoTrackData {
  std::size_t trackId;
  Index measurementId;
  double x, y, z;

  DFE_NAMEDTUPLE(ProtoTrackData, trackId, measurementId, x, y, z);
};

struct GraphData {
  std::int64_t edge0 = 0;
  std::int64_t edge1 = 0;
  float weight = 0.0;
  DFE_NAMEDTUPLE(GraphData, edge0, edge1, weight);
};

struct SpacePointBucketData {
  /// @brief Data structure for space point buckets
  /// @details A bucket is a collection of space points
  ///        Different buckets can contain the same space point
  ///        Measurement IDs are used to uniquely identify space points

  /// Bucket index
  std::uint64_t bucketIdx;
  /// Bucket size (number of space points)
  std::uint64_t bucketSize;

  /// Measurement IDs of the space points in the bucket
  /// To allow for variable size, the bucket data is split into several lines
  /// A line can contain up to 20 space points (arbitrary number)
  std::array<std::uint64_t, 20> measurement_id;

  DFE_NAMEDTUPLE(SpacePointBucketData, bucketIdx, bucketSize, measurement_id[0],
                 measurement_id[1], measurement_id[2], measurement_id[3],
                 measurement_id[4], measurement_id[5], measurement_id[6],
                 measurement_id[7], measurement_id[8], measurement_id[9],
                 measurement_id[10], measurement_id[11], measurement_id[12],
                 measurement_id[13], measurement_id[14], measurement_id[15],
                 measurement_id[16], measurement_id[17], measurement_id[18],
                 measurement_id[19]);
};

}  // namespace ActsExamples
