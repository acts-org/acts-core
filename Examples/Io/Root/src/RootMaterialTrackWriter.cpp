// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootMaterialTrackWriter.hpp"

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Geometry/Volume.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialInteraction.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceBounds.hpp"
#include "Acts/Utilities/Intersection.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"

#include <algorithm>
#include <cstddef>
#include <ios>
#include <stdexcept>
#include <type_traits>

#include <TFile.h>
#include <TTree.h>

using Acts::VectorHelpers::eta;
using Acts::VectorHelpers::perp;
using Acts::VectorHelpers::phi;

namespace ActsExamples {

RootMaterialTrackWriter::RootMaterialTrackWriter(
    const RootMaterialTrackWriter::Config& config, Acts::Logging::Level level)
    : WriterT(config.inputMaterialTracks, "RootMaterialTrackWriter", level),
      m_cfg(config) {
  // An input collection name and tree name must be specified
  if (m_cfg.inputMaterialTracks.empty()) {
    throw std::invalid_argument("Missing input collection");
  } else if (m_cfg.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  }

  // Setup ROOT I/O
  m_outputFile = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
  if (m_outputFile == nullptr) {
    throw std::ios_base::failure("Could not open '" + m_cfg.filePath + "'");
  }

  m_outputFile->cd();
  m_outputTree =
      new TTree(m_cfg.treeName.c_str(), "TTree from RootMaterialTrackWriter");
  if (m_outputTree == nullptr) {
    throw std::bad_alloc();
  }

  // Set the branches
  m_outputTree->Branch("event_id", &m_eventId);
  m_outputTree->Branch("v_x", &m_v_x);
  m_outputTree->Branch("v_y", &m_v_y);
  m_outputTree->Branch("v_z", &m_v_z);
  m_outputTree->Branch("v_px", &m_v_px);
  m_outputTree->Branch("v_py", &m_v_py);
  m_outputTree->Branch("v_pz", &m_v_pz);
  m_outputTree->Branch("v_phi", &m_v_phi);
  m_outputTree->Branch("v_eta", &m_v_eta);
  m_outputTree->Branch("t_X0", &m_tX0);
  m_outputTree->Branch("t_L0", &m_tL0);
  m_outputTree->Branch("mat_x", &m_step_x);
  m_outputTree->Branch("mat_y", &m_step_y);
  m_outputTree->Branch("mat_z", &m_step_z);
  m_outputTree->Branch("mat_r", &m_step_r);
  m_outputTree->Branch("mat_dx", &m_step_dx);
  m_outputTree->Branch("mat_dy", &m_step_dy);
  m_outputTree->Branch("mat_dz", &m_step_dz);
  m_outputTree->Branch("mat_step_length", &m_step_length);
  m_outputTree->Branch("mat_X0", &m_step_X0);
  m_outputTree->Branch("mat_L0", &m_step_L0);
  m_outputTree->Branch("mat_A", &m_step_A);
  m_outputTree->Branch("mat_Z", &m_step_Z);
  m_outputTree->Branch("mat_rho", &m_step_rho);

  if (m_cfg.prePostStep) {
    m_outputTree->Branch("mat_sx", &m_step_sx);
    m_outputTree->Branch("mat_sy", &m_step_sy);
    m_outputTree->Branch("mat_sz", &m_step_sz);
    m_outputTree->Branch("mat_ex", &m_step_ex);
    m_outputTree->Branch("mat_ey", &m_step_ey);
    m_outputTree->Branch("mat_ez", &m_step_ez);
  }
  if (m_cfg.storeSurface) {
    m_outputTree->Branch("sur_id", &m_sur_id);
    m_outputTree->Branch("sur_type", &m_sur_type);
    m_outputTree->Branch("sur_x", &m_sur_x);
    m_outputTree->Branch("sur_y", &m_sur_y);
    m_outputTree->Branch("sur_z", &m_sur_z);
    m_outputTree->Branch("sur_r", &m_sur_r);
    m_outputTree->Branch("sur_distance", &m_sur_distance);
    m_outputTree->Branch("sur_pathCorrection", &m_sur_pathCorrection);
    m_outputTree->Branch("sur_range_min", &m_sur_range_min);
    m_outputTree->Branch("sur_range_max", &m_sur_range_max);
  }
  if (m_cfg.storeVolume) {
    m_outputTree->Branch("vol_id", &m_vol_id);
  }
}

RootMaterialTrackWriter::~RootMaterialTrackWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

ProcessCode RootMaterialTrackWriter::finalize() {
  // write the tree and close the file
  ACTS_INFO("Writing ROOT output File : " << m_cfg.filePath);

  m_outputFile->cd();
  m_outputTree->Write();
  m_outputFile->Close();

  return ProcessCode::SUCCESS;
}

ProcessCode RootMaterialTrackWriter::writeT(
    const AlgorithmContext& ctx,
    const std::unordered_map<std::size_t, Acts::RecordedMaterialTrack>&
        materialTracks) {
  // Exclusive access to the tree while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  m_eventId = ctx.eventNumber;
  // Loop over the material tracks and write them out
  for (auto& [idTrack, mtrack] : materialTracks) {
    // Clearing the vector first
    m_step_sx.clear();
    m_step_sy.clear();
    m_step_sz.clear();
    m_step_x.clear();
    m_step_y.clear();
    m_step_z.clear();
    m_step_r.clear();
    m_step_ex.clear();
    m_step_ey.clear();
    m_step_ez.clear();
    m_step_dx.clear();
    m_step_dy.clear();
    m_step_dz.clear();
    m_step_length.clear();
    m_step_X0.clear();
    m_step_L0.clear();
    m_step_A.clear();
    m_step_Z.clear();
    m_step_rho.clear();

    m_sur_id.clear();
    m_sur_type.clear();
    m_sur_x.clear();
    m_sur_y.clear();
    m_sur_z.clear();
    m_sur_r.clear();
    m_sur_distance.clear();
    m_sur_pathCorrection.clear();
    m_sur_range_min.clear();
    m_sur_range_max.clear();

    m_vol_id.clear();

    auto materialInteractions = mtrack.second.materialInteractions;
    if (m_cfg.collapseInteractions) {
      std::vector<Acts::MaterialInteraction> collapsed;

      Acts::Vector3 positionSum = Acts::Vector3::Zero();
      double pathCorrectionSum = 0;

      for (std::size_t start = 0, end = 0; end < materialInteractions.size();
           ++end) {
        const auto& mintStart = materialInteractions[start];
        const auto& mintEnd = materialInteractions[end];

        positionSum += mintEnd.position;
        pathCorrectionSum += mintEnd.pathCorrection;

        const bool same = mintStart.materialSlab.material() ==
                          mintEnd.materialSlab.material();
        const bool last = end == materialInteractions.size() - 1;

        if (!same || last) {
          auto mint = mintStart;
          mint.position = positionSum / (end - start);
          mint.pathCorrection = pathCorrectionSum;

          collapsed.push_back(mint);

          start = end;
          positionSum = Acts::Vector3::Zero();
          pathCorrectionSum = 0;
        }
      }

      materialInteractions = std::move(collapsed);
    }

    // Reserve the vector then
    std::size_t mints = materialInteractions.size();
    m_step_sx.reserve(mints);
    m_step_sy.reserve(mints);
    m_step_sz.reserve(mints);
    m_step_x.reserve(mints);
    m_step_y.reserve(mints);
    m_step_z.reserve(mints);
    m_step_r.reserve(mints);
    m_step_ex.reserve(mints);
    m_step_ey.reserve(mints);
    m_step_ez.reserve(mints);
    m_step_dx.reserve(mints);
    m_step_dy.reserve(mints);
    m_step_dz.reserve(mints);
    m_step_length.reserve(mints);
    m_step_X0.reserve(mints);
    m_step_L0.reserve(mints);
    m_step_A.reserve(mints);
    m_step_Z.reserve(mints);
    m_step_rho.reserve(mints);

    m_sur_id.reserve(mints);
    m_sur_type.reserve(mints);
    m_sur_x.reserve(mints);
    m_sur_y.reserve(mints);
    m_sur_z.reserve(mints);
    m_sur_r.reserve(mints);
    m_sur_distance.reserve(mints);
    m_sur_pathCorrection.reserve(mints);
    m_sur_range_min.reserve(mints);
    m_sur_range_max.reserve(mints);

    m_vol_id.reserve(mints);

    // reset the global counter
    if (m_cfg.recalculateTotals) {
      m_tX0 = 0.;
      m_tL0 = 0.;
    } else {
      m_tX0 = mtrack.second.materialInX0;
      m_tL0 = mtrack.second.materialInL0;
    }

    // set the track information at vertex
    m_v_x = mtrack.first.first.x();
    m_v_y = mtrack.first.first.y();
    m_v_z = mtrack.first.first.z();
    m_v_px = mtrack.first.second.x();
    m_v_py = mtrack.first.second.y();
    m_v_pz = mtrack.first.second.z();
    m_v_phi = phi(mtrack.first.second);
    m_v_eta = eta(mtrack.first.second);

    // and now loop over the material
    for (const auto& mint : materialInteractions) {
      auto direction = mint.direction.normalized();

      // The material step position information
      m_step_x.push_back(mint.position.x());
      m_step_y.push_back(mint.position.y());
      m_step_z.push_back(mint.position.z());
      m_step_r.push_back(perp(mint.position));
      m_step_dx.push_back(direction.x());
      m_step_dy.push_back(direction.y());
      m_step_dz.push_back(direction.z());

      if (m_cfg.prePostStep) {
        Acts::Vector3 prePos =
            mint.position - 0.5 * mint.pathCorrection * direction;
        Acts::Vector3 posPos =
            mint.position + 0.5 * mint.pathCorrection * direction;

        m_step_sx.push_back(prePos.x());
        m_step_sy.push_back(prePos.y());
        m_step_sz.push_back(prePos.z());
        m_step_ex.push_back(posPos.x());
        m_step_ey.push_back(posPos.y());
        m_step_ez.push_back(posPos.z());
      }

      // Store surface information
      if (m_cfg.storeSurface) {
        const Acts::Surface* surface = mint.surface;
        if (mint.intersectionID.value() != 0) {
          m_sur_id.push_back(mint.intersectionID.value());
          m_sur_pathCorrection.push_back(mint.pathCorrection);
          m_sur_x.push_back(mint.intersection.x());
          m_sur_y.push_back(mint.intersection.y());
          m_sur_z.push_back(mint.intersection.z());
          m_sur_r.push_back(perp(mint.intersection));
          m_sur_distance.push_back((mint.position - mint.intersection).norm());
        } else if (surface != nullptr) {
          auto sfIntersection =
              surface
                  ->intersect(ctx.geoContext, mint.position, mint.direction,
                              Acts::BoundaryTolerance::None())
                  .closest();
          m_sur_id.push_back(surface->geometryId().value());
          m_sur_pathCorrection.push_back(1.0);
          m_sur_x.push_back(sfIntersection.position().x());
          m_sur_y.push_back(sfIntersection.position().y());
          m_sur_z.push_back(sfIntersection.position().z());
        } else {
          m_sur_id.push_back(Acts::GeometryIdentifier().value());
          m_sur_x.push_back(0);
          m_sur_y.push_back(0);
          m_sur_z.push_back(0);
          m_sur_pathCorrection.push_back(1.0);
        }
        if (surface != nullptr) {
          m_sur_type.push_back(surface->type());
          const Acts::SurfaceBounds& surfaceBounds = surface->bounds();
          const Acts::RadialBounds* radialBounds =
              dynamic_cast<const Acts::RadialBounds*>(&surfaceBounds);
          const Acts::CylinderBounds* cylinderBounds =
              dynamic_cast<const Acts::CylinderBounds*>(&surfaceBounds);
          if (radialBounds != nullptr) {
            m_sur_range_min.push_back(radialBounds->rMin());
            m_sur_range_max.push_back(radialBounds->rMax());
          } else if (cylinderBounds != nullptr) {
            m_sur_range_min.push_back(
                -cylinderBounds->get(Acts::CylinderBounds::eHalfLengthZ));
            m_sur_range_max.push_back(
                cylinderBounds->get(Acts::CylinderBounds::eHalfLengthZ));
          } else {
            m_sur_range_min.push_back(0);
            m_sur_range_max.push_back(0);
          }
        } else {
          m_sur_type.push_back(-1);
          m_sur_range_min.push_back(0);
          m_sur_range_max.push_back(0);
        }
      }

      // store volume information
      if (m_cfg.storeVolume) {
        Acts::GeometryIdentifier vlayerID;
        if (!mint.volume.empty()) {
          vlayerID = mint.volume.geometryId();
          m_vol_id.push_back(vlayerID.value());
        } else {
          vlayerID.setVolume(0);
          vlayerID.setBoundary(0);
          vlayerID.setLayer(0);
          vlayerID.setApproach(0);
          vlayerID.setSensitive(0);
          m_vol_id.push_back(vlayerID.value());
        }
      }

      // the material information
      const auto& mprops = mint.materialSlab;
      m_step_length.push_back(mprops.thickness());
      m_step_X0.push_back(mprops.material().X0());
      m_step_L0.push_back(mprops.material().L0());
      m_step_A.push_back(mprops.material().Ar());
      m_step_Z.push_back(mprops.material().Z());
      m_step_rho.push_back(mprops.material().massDensity());
      // re-calculate if defined to do so
      if (m_cfg.recalculateTotals) {
        m_tX0 += mprops.thicknessInX0();
        m_tL0 += mprops.thicknessInL0();
      }
    }
    // write to
    m_outputTree->Fill();
  }

  // return success
  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
