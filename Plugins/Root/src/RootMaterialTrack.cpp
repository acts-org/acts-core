// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Root/RootMaterialTrack.hpp"

#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <ROOT/RField.hxx>
#include <ROOT/RNTupleModel.hxx>
#include <TChain.h>
#include <TTree.h>

Acts::RootMaterialTrack::RootMaterialTrack(const Config& cfg) : m_cfg(cfg) {}

void Acts::RootMaterialTrack::initializeWrite(
    ROOT::Experimental::RNTupleModel& rntModel) {
  m_payload.eventId = rntModel.MakeField<std::uint32_t>("event_id");

  m_payload.vX = rntModel.MakeField<float>("v_x");
  m_payload.vY = rntModel.MakeField<float>("v_y");
  m_payload.vZ = rntModel.MakeField<float>("v_z");
  m_payload.vPX = rntModel.MakeField<float>("v_px");
  m_payload.vPY = rntModel.MakeField<float>("v_py");
  m_payload.vPZ = rntModel.MakeField<float>("v_pz");
  m_payload.vPPhi = rntModel.MakeField<float>("v_phi");
  m_payload.vPEta = rntModel.MakeField<float>("v_eta");
  m_payload.tX0 = rntModel.MakeField<float>("t_X0");
  m_payload.tL0 = rntModel.MakeField<float>("t_L0");

  m_payload.stepX = rntModel.MakeField<std::vector<float>>("mat_x");
  m_payload.stepY = rntModel.MakeField<std::vector<float>>("mat_y");
  m_payload.stepZ = rntModel.MakeField<std::vector<float>>("mat_z");
  m_payload.stepR = rntModel.MakeField<std::vector<float>>("mat_r");
  if (m_cfg.prePostStep) {
    m_payload.stepXs = rntModel.MakeField<std::vector<float>>("mat_sx");
    m_payload.stepYs = rntModel.MakeField<std::vector<float>>("mat_sy");
    m_payload.stepZs = rntModel.MakeField<std::vector<float>>("mat_sz");
    m_payload.stepXe = rntModel.MakeField<std::vector<float>>("mat_ex");
    m_payload.stepYe = rntModel.MakeField<std::vector<float>>("mat_ey");
    m_payload.stepZe = rntModel.MakeField<std::vector<float>>("mat_ez");
  }
  m_payload.stepDX = rntModel.MakeField<std::vector<float>>("mat_dx");
  m_payload.stepDY = rntModel.MakeField<std::vector<float>>("mat_dy");
  m_payload.stepDZ = rntModel.MakeField<std::vector<float>>("mat_dz");
  m_payload.stepLength =
      rntModel.MakeField<std::vector<float>>("mat_step_length");
  m_payload.matX0 = rntModel.MakeField<std::vector<float>>("mat_X0");
  m_payload.matL0 = rntModel.MakeField<std::vector<float>>("mat_L0");
  m_payload.matA = rntModel.MakeField<std::vector<float>>("mat_A");
  m_payload.matZ = rntModel.MakeField<std::vector<float>>("mat_Z");
  m_payload.matRho = rntModel.MakeField<std::vector<float>>("mat_rho");

  m_payload.surfaceId =
      rntModel.MakeField<std::vector<std::uint64_t>>("sur_id");
  m_payload.surfaceType =
      rntModel.MakeField<std::vector<std::int32_t>>("sur_type");
  m_payload.surfaceX = rntModel.MakeField<std::vector<float>>("sur_x");
  m_payload.surfaceY = rntModel.MakeField<std::vector<float>>("sur_y");
  m_payload.surfaceZ = rntModel.MakeField<std::vector<float>>("sur_z");
  m_payload.surfaceR = rntModel.MakeField<std::vector<float>>("sur_r");
  m_payload.surfaceDistance =
      rntModel.MakeField<std::vector<float>>("sur_distance");
  m_payload.surfacePathCorrection =
      rntModel.MakeField<std::vector<float>>("sur_pathCorrection");
  m_payload.surfaceRangeMin =
      rntModel.MakeField<std::vector<float>>("sur_range_min");
  m_payload.surfaceRangeMax =
      rntModel.MakeField<std::vector<float>>("sur_range_max");

  m_payload.volumeId = rntModel.MakeField<std::vector<std::uint64_t>>("vol_id");
}

void Acts::RootMaterialTrack::initializeRead(TChain& chain) {
  // Set the branches
  chain.SetBranchAddress("event_id", &m_payload.eventId);
  chain.SetBranchAddress("v_x", &m_payload.vX);
  chain.SetBranchAddress("v_y", &m_payload.vY);
  chain.SetBranchAddress("v_z", &m_payload.vZ);
  chain.SetBranchAddress("v_px", &m_payload.vPX);
  chain.SetBranchAddress("v_py", &m_payload.vPY);
  chain.SetBranchAddress("v_pz", &m_payload.vPZ);
  chain.SetBranchAddress("v_phi", &m_payload.vPPhi);
  chain.SetBranchAddress("v_eta", &m_payload.vPEta);
  chain.SetBranchAddress("t_X0", &m_payload.tX0);
  chain.SetBranchAddress("t_L0", &m_payload.tL0);
  chain.SetBranchAddress("mat_x", &m_payload.stepX);
  chain.SetBranchAddress("mat_y", &m_payload.stepY);
  chain.SetBranchAddress("mat_z", &m_payload.stepZ);
  chain.SetBranchAddress("mat_dx", &m_payload.stepDX);
  chain.SetBranchAddress("mat_dy", &m_payload.stepDY);
  chain.SetBranchAddress("mat_dz", &m_payload.stepDZ);
  chain.SetBranchAddress("mat_step_length", &m_payload.stepLength);
  chain.SetBranchAddress("mat_X0", &m_payload.matX0);
  chain.SetBranchAddress("mat_L0", &m_payload.matL0);
  chain.SetBranchAddress("mat_A", &m_payload.matA);
  chain.SetBranchAddress("mat_Z", &m_payload.matZ);
  chain.SetBranchAddress("mat_rho", &m_payload.matRho);

  if (m_cfg.readCachedSurfaceInformation) {
    chain.SetBranchAddress("sur_id", &m_payload.surfaceId);
    chain.SetBranchAddress("sur_x", &m_payload.surfaceX);
    chain.SetBranchAddress("sur_y", &m_payload.surfaceY);
    chain.SetBranchAddress("sur_z", &m_payload.surfaceZ);
    chain.SetBranchAddress("sur_pathCorrection",
                           &m_payload.surfacePathCorrection);
  }
}

void Acts::RootMaterialTrack::initializeWrite(TTree& tree) {
  // Set the branches
  tree.Branch("event_id", m_payload.eventId.get());
  tree.Branch("v_x", m_payload.vX.get());
  tree.Branch("v_y", m_payload.vY.get());
  tree.Branch("v_z", m_payload.vZ.get());
  tree.Branch("v_px", m_payload.vPX.get());
  tree.Branch("v_py", m_payload.vPY.get());
  tree.Branch("v_pz", m_payload.vPZ.get());
  tree.Branch("v_phi", m_payload.vPPhi.get());
  tree.Branch("v_eta", m_payload.vPEta.get());
  tree.Branch("t_X0", m_payload.tX0.get());
  tree.Branch("t_L0", m_payload.tL0.get());
  tree.Branch("mat_x", m_payload.stepX.get());
  tree.Branch("mat_y", m_payload.stepY.get());
  tree.Branch("mat_z", m_payload.stepZ.get());
  tree.Branch("mat_r", m_payload.stepR.get());
  tree.Branch("mat_dx", m_payload.stepDX.get());
  tree.Branch("mat_dy", m_payload.stepDY.get());
  tree.Branch("mat_dz", m_payload.stepDZ.get());
  tree.Branch("mat_step_length", m_payload.stepLength.get());
  tree.Branch("mat_X0", m_payload.matX0.get());
  tree.Branch("mat_L0", m_payload.matL0.get());
  tree.Branch("mat_A", m_payload.matA.get());
  tree.Branch("mat_Z", m_payload.matZ.get());
  tree.Branch("mat_rho", m_payload.matRho.get());

  if (m_cfg.prePostStep) {
    tree.Branch("mat_sx", m_payload.stepXs.get());
    tree.Branch("mat_sy", m_payload.stepYs.get());
    tree.Branch("mat_sz", m_payload.stepZs.get());
    tree.Branch("mat_ex", m_payload.stepXe.get());
    tree.Branch("mat_ey", m_payload.stepYe.get());
    tree.Branch("mat_ez", m_payload.stepZe.get());
  }
  if (m_cfg.storeSurface) {
    tree.Branch("sur_id", m_payload.surfaceId.get());
    tree.Branch("sur_type", m_payload.surfaceType.get());
    tree.Branch("sur_x", m_payload.surfaceX.get());
    tree.Branch("sur_y", m_payload.surfaceY.get());
    tree.Branch("sur_z", m_payload.surfaceZ.get());
    tree.Branch("sur_r", m_payload.surfaceR.get());
    tree.Branch("sur_distance", m_payload.surfaceDistance.get());
    tree.Branch("sur_pathCorrection", m_payload.surfacePathCorrection.get());
    tree.Branch("sur_range_min", m_payload.surfaceRangeMin.get());
    tree.Branch("sur_range_max", m_payload.surfaceRangeMax.get());
  }
  if (m_cfg.storeVolume) {
    tree.Branch("vol_id", m_payload.volumeId.get());
  }
}

void Acts::RootMaterialTrack::clear() {
  m_payload.stepXs->clear();
  m_payload.stepYs->clear();
  m_payload.stepZs->clear();
  m_payload.stepX->clear();
  m_payload.stepY->clear();
  m_payload.stepZ->clear();
  m_payload.stepR->clear();
  m_payload.stepXe->clear();
  m_payload.stepYe->clear();
  m_payload.stepZe->clear();

  m_payload.stepDX->clear();
  m_payload.stepDY->clear();
  m_payload.stepDZ->clear();
  m_payload.stepLength->clear();
  m_payload.matX0->clear();
  m_payload.matL0->clear();
  m_payload.matA->clear();
  m_payload.matZ->clear();
  m_payload.matRho->clear();

  m_payload.surfaceId->clear();
  m_payload.surfaceType->clear();
  m_payload.surfaceX->clear();
  m_payload.surfaceY->clear();
  m_payload.surfaceZ->clear();
  m_payload.surfaceR->clear();
  m_payload.surfaceDistance->clear();
  m_payload.surfacePathCorrection->clear();
  m_payload.surfaceRangeMin->clear();
  m_payload.surfaceRangeMax->clear();

  m_payload.volumeId->clear();
}

void Acts::RootMaterialTrack::fill(const GeometryContext& gctx,
                                   const RecordedMaterialTrack& rmTrack,
                                   const Auxiliaries& aux) {
  clear();
  // Prepare in case we want to collapse single interactions per surface to one
  auto materialInteractions = rmTrack.second.materialInteractions;
  if (m_cfg.collapseInteractions) {
    std::vector<MaterialInteraction> collapsed;

    Vector3 positionSum = Vector3::Zero();
    double pathCorrectionSum = 0;

    for (std::size_t start = 0, end = 0; end < materialInteractions.size();
         ++end) {
      const auto& mintStart = materialInteractions[start];
      const auto& mintEnd = materialInteractions[end];

      positionSum += mintEnd.position;
      pathCorrectionSum += mintEnd.pathCorrection;

      const bool same =
          mintStart.materialSlab.material() == mintEnd.materialSlab.material();
      const bool last = end == materialInteractions.size() - 1;

      if (!same || last) {
        auto mint = mintStart;
        mint.position = positionSum / (end - start);
        mint.pathCorrection = pathCorrectionSum;

        collapsed.emplace_back(mint);

        start = end;
        positionSum = Vector3::Zero();
        pathCorrectionSum = 0;
      }
    }
    materialInteractions = std::move(collapsed);
  }

  // Reserve the vector then
  std::size_t mints = materialInteractions.size();
  m_payload.stepXs->reserve(mints);
  m_payload.stepYs->reserve(mints);
  m_payload.stepZs->reserve(mints);
  m_payload.stepX->reserve(mints);
  m_payload.stepY->reserve(mints);
  m_payload.stepZ->reserve(mints);
  m_payload.stepR->reserve(mints);
  m_payload.stepXe->reserve(mints);
  m_payload.stepYe->reserve(mints);
  m_payload.stepZe->reserve(mints);

  m_payload.stepDX->reserve(mints);
  m_payload.stepDY->reserve(mints);
  m_payload.stepDZ->reserve(mints);
  m_payload.stepLength->reserve(mints);
  m_payload.matX0->reserve(mints);
  m_payload.matL0->reserve(mints);
  m_payload.matA->reserve(mints);
  m_payload.matZ->reserve(mints);
  m_payload.matRho->reserve(mints);

  m_payload.surfaceId->reserve(mints);
  m_payload.surfaceType->reserve(mints);
  m_payload.surfaceX->reserve(mints);
  m_payload.surfaceY->reserve(mints);
  m_payload.surfaceZ->reserve(mints);
  m_payload.surfaceR->reserve(mints);
  m_payload.surfaceDistance->reserve(mints);
  m_payload.surfacePathCorrection->reserve(mints);
  m_payload.surfaceRangeMin->reserve(mints);
  m_payload.surfaceRangeMax->reserve(mints);

  m_payload.volumeId->reserve(mints);

  // set the event ID
  (*m_payload.eventId) = aux.eventId;

  // set the track information at vertex
  (*m_payload.vX) = rmTrack.first.first.x();
  (*m_payload.vY) = rmTrack.first.first.y();
  (*m_payload.vZ) = rmTrack.first.first.z();
  (*m_payload.vPX) = rmTrack.first.second.x();
  (*m_payload.vPY) = rmTrack.first.second.y();
  (*m_payload.vPZ) = rmTrack.first.second.z();
  (*m_payload.vPPhi) = VectorHelpers::phi(rmTrack.first.second);
  (*m_payload.vPEta) = VectorHelpers::eta(rmTrack.first.second);

  // reset the global counter
  if (m_cfg.recalculateTotals) {
    (*m_payload.tX0) = 0.;
    (*m_payload.tL0) = 0.;
  } else {
    (*m_payload.tX0) = rmTrack.second.materialInX0;
    (*m_payload.tL0) = rmTrack.second.materialInL0;
  }

  // Loop over the material
  for (const auto& mint : materialInteractions) {
    auto direction = mint.direction.normalized();

    // The material step position information
    m_payload.stepX->emplace_back(mint.position.x());
    m_payload.stepY->emplace_back(mint.position.y());
    m_payload.stepZ->emplace_back(mint.position.z());
    m_payload.stepR->emplace_back(VectorHelpers::perp(mint.position));
    m_payload.stepDX->emplace_back(direction.x());
    m_payload.stepDY->emplace_back(direction.y());
    m_payload.stepDZ->emplace_back(direction.z());

    // Record pre/post step
    if (m_cfg.prePostStep) {
      Vector3 prePos = mint.position - 0.5 * mint.pathCorrection * direction;
      Vector3 posPos = mint.position + 0.5 * mint.pathCorrection * direction;

      m_payload.stepXs->emplace_back(prePos.x());
      m_payload.stepYs->emplace_back(prePos.y());
      m_payload.stepZs->emplace_back(prePos.z());
      m_payload.stepXe->emplace_back(posPos.x());
      m_payload.stepYe->emplace_back(posPos.y());
      m_payload.stepZe->emplace_back(posPos.z());
    }

    // Store surface information
    if (m_cfg.storeSurface) {
      const Surface* surface = mint.surface;
      if (mint.intersectionID.value() != 0) {
        m_payload.surfaceId->emplace_back(mint.intersectionID.value());
        m_payload.surfacePathCorrection->emplace_back(mint.pathCorrection);
        m_payload.surfaceX->emplace_back(mint.intersection.x());
        m_payload.surfaceY->emplace_back(mint.intersection.y());
        m_payload.surfaceZ->emplace_back(mint.intersection.z());
        m_payload.surfaceR->emplace_back(
            VectorHelpers::perp(mint.intersection));
        m_payload.surfaceDistance->emplace_back(
            (mint.position - mint.intersection).norm());
      } else if (surface != nullptr) {
        auto sfIntersection =
            surface
                ->intersect(gctx, mint.position, mint.direction,
                            BoundaryTolerance::None())
                .closest();
        m_payload.surfaceId->emplace_back(surface->geometryId().value());
        m_payload.surfacePathCorrection->emplace_back(1.0);
        m_payload.surfaceX->emplace_back(sfIntersection.position().x());
        m_payload.surfaceY->emplace_back(sfIntersection.position().y());
        m_payload.surfaceZ->emplace_back(sfIntersection.position().z());
      } else {
        m_payload.surfaceId->emplace_back(GeometryIdentifier().value());
        m_payload.surfaceX->emplace_back(0);
        m_payload.surfaceY->emplace_back(0);
        m_payload.surfaceZ->emplace_back(0);
        m_payload.surfacePathCorrection->emplace_back(1.0);
      }
      if (surface != nullptr) {
        m_payload.surfaceType->emplace_back(surface->type());
        const SurfaceBounds& surfaceBounds = surface->bounds();
        auto radialBounds = dynamic_cast<const RadialBounds*>(&surfaceBounds);
        auto cylinderBounds =
            dynamic_cast<const CylinderBounds*>(&surfaceBounds);
        if (radialBounds != nullptr) {
          m_payload.surfaceRangeMin->emplace_back(radialBounds->rMin());
          m_payload.surfaceRangeMax->emplace_back(radialBounds->rMax());
        } else if (cylinderBounds != nullptr) {
          m_payload.surfaceRangeMin->emplace_back(
              -cylinderBounds->get(CylinderBounds::eHalfLengthZ));
          m_payload.surfaceRangeMax->emplace_back(
              cylinderBounds->get(CylinderBounds::eHalfLengthZ));
        } else {
          m_payload.surfaceRangeMin->emplace_back(0);
          m_payload.surfaceRangeMax->emplace_back(0);
        }
      } else {
        m_payload.surfaceType->emplace_back(-1);
        m_payload.surfaceRangeMin->emplace_back(0);
        m_payload.surfaceRangeMax->emplace_back(0);
      }
    }

    // store volume information
    if (m_cfg.storeVolume) {
      GeometryIdentifier vlayerID;
      if (!mint.volume.empty()) {
        vlayerID = mint.volume.geometryId();
        m_payload.volumeId->emplace_back(vlayerID.value());
      } else {
        vlayerID.setVolume(0);
        vlayerID.setBoundary(0);
        vlayerID.setLayer(0);
        vlayerID.setApproach(0);
        vlayerID.setSensitive(0);
        m_payload.volumeId->emplace_back(vlayerID.value());
      }
    }

    // the material information
    const auto& mprops = mint.materialSlab;
    m_payload.stepLength->emplace_back(mprops.thickness());
    m_payload.matX0->emplace_back(mprops.material().X0());
    m_payload.matL0->emplace_back(mprops.material().L0());
    m_payload.matA->emplace_back(mprops.material().Ar());
    m_payload.matZ->emplace_back(mprops.material().Z());
    m_payload.matRho->emplace_back(mprops.material().massDensity());
    // re-calculate if defined to do so
    if (m_cfg.recalculateTotals) {
      (*m_payload.tX0) += mprops.thicknessInX0();
      (*m_payload.tL0) += mprops.thicknessInL0();
    }
  }
}

Acts::RecordedMaterialTrack Acts::RootMaterialTrack::read() const {
  RecordedMaterialTrack rmTrack;
  // Fill the position and momentum
  rmTrack.first.first = Vector3(*m_payload.vX, *m_payload.vY, *m_payload.vZ);
  rmTrack.first.second =
      Vector3(*m_payload.vPX, *m_payload.vPY, *m_payload.vPZ);

  // Fill the individual steps
  std::size_t msteps = m_payload.stepLength->size();

  rmTrack.second.materialInteractions.reserve(msteps);
  rmTrack.second.materialInX0 = 0.;
  rmTrack.second.materialInL0 = 0.;

  for (std::size_t is = 0; is < msteps; ++is) {
    double s = m_payload.stepLength->at(is);
    if (s == 0) {
      continue;
    }

    double mX0 = m_payload.matX0->at(is);
    double mL0 = m_payload.matL0->at(is);
    rmTrack.second.materialInX0 += s / mX0;
    rmTrack.second.materialInL0 += s / mL0;

    /// Fill the position & the material
    MaterialInteraction mInteraction;
    mInteraction.position =
        Vector3(m_payload.stepX->at(is), m_payload.stepY->at(is),
                m_payload.stepZ->at(is));
    mInteraction.direction =
        Vector3(m_payload.stepDX->at(is), m_payload.stepDY->at(is),
                m_payload.stepDZ->at(is));
    mInteraction.materialSlab =
        MaterialSlab(Material::fromMassDensity(mX0, mL0, m_payload.matA->at(is),
                                               m_payload.matZ->at(is),
                                               m_payload.matRho->at(is)),
                     s);
    if (m_cfg.readCachedSurfaceInformation) {
      // add the surface information to the interaction this allows the
      // mapping to be speed up
      mInteraction.intersectionID =
          GeometryIdentifier(m_payload.surfaceId->at(is));
      mInteraction.intersection =
          Vector3(m_payload.surfaceX->at(is), m_payload.surfaceY->at(is),
                  m_payload.surfaceZ->at(is));
      mInteraction.pathCorrection = m_payload.surfacePathCorrection->at(is);
    } else {
      mInteraction.intersectionID = GeometryIdentifier();
      mInteraction.intersection = Vector3(0, 0, 0);
    }
    rmTrack.second.materialInteractions.push_back(std::move(mInteraction));
  }
  return rmTrack;
}
