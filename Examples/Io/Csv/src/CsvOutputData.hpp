// This file is part of the Acts project.
//
// Copyright (C) 2019 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

/// @file
/// @brief Plain structs that each define one row in a TrackML csv file

#pragma once

#include <cstdint>

#include <dfe/dfe_namedtuple.hpp>

namespace ActsExamples {

struct ParticleData {
  /// Event-unique particle identifier a.k.a barcode.
  uint64_t particle_id;
  /// Particle type number a.k.a. PDG particle number.
  int32_t particle_type;
  /// Production process type. Not available in the TrackML datasets.
  uint32_t process = 0u;
  /// Production position components in mm.
  float vx, vy, vz;
  // Production time in ns. Not available in the TrackML datasets.
  float vt = 0.0f;
  /// Momentum components in GeV.
  float px, py, pz;
  /// Mass in GeV. Not available in the TrackML datasets
  float m = 0.0f;
  /// Charge in e.
  float q;

  DFE_NAMEDTUPLE(ParticleData, particle_id, particle_type, process, vx, vy, vz,
                 vt, px, py, pz, m, q);
};

// Write out simhits before digitization (no hi_id associated)
struct SimHitData {
  /// Hit surface identifier. Not available in the TrackML datasets.
  uint64_t geometry_id = 0u;
  /// Event-unique particle identifier of the generating particle.
  uint64_t particle_id;
  /// True global hit position components in mm.
  float tx, ty, tz;
  // True global hit time in ns. Not available in the TrackML datasets.
  float tt = 0.0f;
  /// True particle momentum in GeV before interaction.
  float tpx, tpy, tpz;
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
  int32_t index = -1;

  DFE_NAMEDTUPLE(SimHitData, particle_id, geometry_id, tx, ty, tz, tt, tpx, tpy,
                 tpz, te, deltapx, deltapy, deltapz, deltae, index);
};

struct TruthHitData {
  /// Event-unique hit identifier. As defined for the simulated hit below and
  /// used to link back to it; same value can appear multiple times here due to
  /// shared hits in dense environments.
  uint64_t hit_id;
  /// Hit surface identifier. Not available in the TrackML datasets.
  uint64_t geometry_id = 0u;
  /// Event-unique particle identifier of the generating particle.
  uint64_t particle_id;
  /// True global hit position components in mm.
  float tx, ty, tz;
  // True global hit time in ns. Not available in the TrackML datasets.
  float tt = 0.0f;
  /// True particle momentum in GeV before interaction.
  float tpx, tpy, tpz;
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
  int32_t index = -1;

  DFE_NAMEDTUPLE(TruthHitData, hit_id, particle_id, geometry_id, tx, ty, tz, tt,
                 tpx, tpy, tpz, te, deltapx, deltapy, deltapz, deltae, index);
};

struct HitData {
  /// Event-unique hit identifier. Each value can appear at most once.
  uint64_t hit_id;
  /// Hit surface identifier. Not available in the TrackML datasets.
  uint64_t geometry_id = 0u;
  /// Global hit position components in mm.
  float x, y, z;
  /// Global hit time in ns. Not available in the TrackML datasets.
  float t = 0.0f;

  DFE_NAMEDTUPLE(HitData, hit_id, geometry_id, x, y, z, t);
};

struct MeasurementSimHitLink {
  /// Event-unique measurement identifier. Each value can appear at most once.
  uint64_t measurement_id;
  /// Event-unique measurement sim hit identifier.
  uint64_t hit_id;

  DFE_NAMEDTUPLE(MeasurementSimHitLink, measurement_id, hit_id);
};

struct MeasurementData {
  /// Event-unique measurement identifier. Each value can appear at most once.
  uint64_t measurement_id;
  /// Hit surface identifier.
  uint64_t geometry_id = 0u;
  /// Local hit information - bit identification what's measured
  uint8_t local_key;
  float local0, local1, phi, theta, time;
  float var_local0, var_local1, var_phi, var_theta, var_time;

  DFE_NAMEDTUPLE(MeasurementData, measurement_id, geometry_id, local_key,
                 local0, local1, phi, theta, time, var_local0, var_local1,
                 var_phi, var_theta, var_time);
};

struct CellData {
  /// Hit surface identifier.
  uint64_t geometry_id = 0u;
  /// Event-unique hit identifier. As defined for the simulated hit above and
  /// used to link back to it; same value can appear multiple times for clusters
  /// with more than one active cell.
  uint64_t hit_id;
  /// Digital cell address/ channel
  int32_t channel0, channel1;
  /// Digital cell timestamp. Not available in the TrackML datasets.
  float timestamp = 0;
  /// (Digital) measured cell value, e.g. amplitude or time-over-threshold.
  float value;

  DFE_NAMEDTUPLE(CellData, geometry_id, hit_id, channel0, channel1, timestamp,
                 value);
};

struct SurfaceData {
  /// Surface identifier. Not available in the TrackML datasets.
  uint64_t geometry_id;
  /// Partially decoded surface identifier components.
  uint32_t volume_id, boundary_id, layer_id, module_id;
  /// Center position components in mm.
  float cx, cy, cz;
  /// Rotation matrix components.
  float rot_xu, rot_xv, rot_xw;
  float rot_yu, rot_yv, rot_yw;
  float rot_zu, rot_zv, rot_zw;
  /// The type of the surface bpounds object, determines the parameters filled
  int bounds_type;
  float bound_param0 = -1.f;
  float bound_param1 = -1.f;
  float bound_param2 = -1.f;
  float bound_param3 = -1.f;
  float bound_param4 = -1.f;
  float bound_param5 = -1.f;
  float bound_param6 = -1.f;

  DFE_NAMEDTUPLE(SurfaceData, geometry_id, volume_id, boundary_id, layer_id,
                 module_id, cx, cy, cz, rot_xu, rot_xv, rot_xw, rot_yu, rot_yv,
                 rot_yw, rot_zu, rot_zv, rot_zw, bounds_type, bound_param0,
                 bound_param1, bound_param2, bound_param3, bound_param4,
                 bound_param5, bound_param6);
};

struct LayerVolumeData {
  /// Surface identifier. Not available in the TrackML datasets.
  uint64_t geometry_id;
  /// Partially decoded surface identifier components.
  uint32_t volume_id, layer_id;
  /// The type of the surface bpounds object, determines the parameters filled
  int volume_type;
  float min_v0 = -1.f;
  float max_v0 = -1.f;
  float min_v1 = -1.f;
  float max_v1 = -1.f;
  float min_v2 = -1.f;
  float max_v2 = -1.f;

  DFE_NAMEDTUPLE(LayerVolumeData, geometry_id, volume_id, layer_id, min_v0,
                 max_v0, min_v1, max_v1, min_v2, max_v2);
};

struct SurfaceGridData {
  /// Surface identifier. Not available in the TrackML datasets.
  uint64_t geometry_id;
  /// Partially decoded surface identifier components.
  uint32_t volume_id, layer_id, surface_id;
  /// The number of bins in loc 0 / 1
  int type_loc0 = -1;
  int nbins_loc0 = -1;
  float min_loc0, max_loc0;
  int type_loc1 = -1;
  int nbins_loc1 = -1;
  float min_loc1, max_loc1;

  DFE_NAMEDTUPLE(SurfaceGridData, geometry_id, volume_id, layer_id, surface_id,
                 type_loc0, nbins_loc0, min_loc0, max_loc0, type_loc1,
                 nbins_loc1, min_loc1, max_loc1);
};

struct SpacepointData {
  uint64_t measurement_id;
  float x, y, z;
  float var_r, var_z;
  DFE_NAMEDTUPLE(SpacepointData, measurement_id, x, y, z, var_r, var_z);
};

}  // namespace ActsExamples
