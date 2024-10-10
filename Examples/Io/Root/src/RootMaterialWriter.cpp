// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootMaterialWriter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Geometry/ApproachDescriptor.hpp"
#include "Acts/Geometry/BoundarySurfaceT.hpp"
#include "Acts/Geometry/Layer.hpp"
#include "Acts/Geometry/TrackingGeometry.hpp"
#include "Acts/Geometry/TrackingVolume.hpp"
#include "Acts/Material/ISurfaceMaterial.hpp"
#include "Acts/Material/IVolumeMaterial.hpp"
#include "Acts/Material/InterpolatedMaterialMap.hpp"
#include "Acts/Material/Material.hpp"
#include "Acts/Material/MaterialGridHelper.hpp"
#include "Acts/Material/MaterialSlab.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/SurfaceArray.hpp"
#include "Acts/Utilities/BinUtility.hpp"
#include "Acts/Utilities/BinnedArray.hpp"
#include "Acts/Utilities/BinningData.hpp"
#include "Acts/Utilities/Enumerate.hpp"
#include "Acts/Utilities/Logger.hpp"
#include <Acts/Geometry/GeometryIdentifier.hpp>
#include <Acts/Material/BinnedSurfaceMaterial.hpp>

#include <cstddef>
#include <ios>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <TFile.h>
#include <TH1.h>
#include <TH2.h>

ActsExamples::RootMaterialWriter::RootMaterialWriter(
    const ActsExamples::RootMaterialWriter::Config& config,
    Acts::Logging::Level level)
    : m_cfg(config),
      m_logger{Acts::getDefaultLogger("RootMaterialWriter", level)} {
  // Validate the configuration
  if (m_cfg.folderSurfaceNameBase.empty()) {
    throw std::invalid_argument("Missing surface folder name base");
  } else if (m_cfg.folderVolumeNameBase.empty()) {
    throw std::invalid_argument("Missing volume folder name base");
  } else if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing file name");
  }

  // Setup ROOT I/O
  m_outputFile = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
  if (m_outputFile == nullptr) {
    throw std::ios_base::failure("Could not open '" + m_cfg.filePath + "'");
  }
}

void ActsExamples::RootMaterialWriter::writeMaterial(
    const Acts::DetectorMaterialMaps& detMaterial) {
  // Change to the output file
  m_outputFile->cd();

  auto& surfaceMaps = detMaterial.first;
  for (auto& [key, value] : surfaceMaps) {
    // Get the Surface material
    const Acts::ISurfaceMaterial* sMaterial = value.get();

    // get the geometry ID
    Acts::GeometryIdentifier geoID = key;
    // decode the geometryID
    const auto gvolID = geoID.volume();
    const auto gbouID = geoID.boundary();
    const auto glayID = geoID.layer();
    const auto gappID = geoID.approach();
    const auto gsenID = geoID.sensitive();
    // create the directory
    std::string tdName = m_cfg.folderSurfaceNameBase.c_str();
    tdName += m_cfg.voltag + std::to_string(gvolID);
    tdName += m_cfg.boutag + std::to_string(gbouID);
    tdName += m_cfg.laytag + std::to_string(glayID);
    tdName += m_cfg.apptag + std::to_string(gappID);
    tdName += m_cfg.sentag + std::to_string(gsenID);
    // create a new directory
    m_outputFile->mkdir(tdName.c_str());
    m_outputFile->cd(tdName.c_str());

    ACTS_VERBOSE("Writing out map at " << tdName);

    std::size_t bins0 = 1, bins1 = 1;
    // understand what sort of material you have in mind
    const Acts::BinnedSurfaceMaterial* bsm =
        dynamic_cast<const Acts::BinnedSurfaceMaterial*>(sMaterial);
    if (bsm != nullptr) {
      // overwrite the bin numbers
      bins0 = bsm->binUtility().bins(0);
      bins1 = bsm->binUtility().bins(1);

      // Get the binning data
      auto& binningData = bsm->binUtility().binningData();
      // 1-D or 2-D maps
      std::size_t binningBins = binningData.size();

      // The bin number information
      TH1F* n = new TH1F(m_cfg.ntag.c_str(), "bins; bin", binningBins, -0.5,
                         binningBins - 0.5);

      // The binning value information
      TH1F* v = new TH1F(m_cfg.vtag.c_str(), "binning values; bin", binningBins,
                         -0.5, binningBins - 0.5);

      // The binning option information
      TH1F* o = new TH1F(m_cfg.otag.c_str(), "binning options; bin",
                         binningBins, -0.5, binningBins - 0.5);

      // The binning option information
      TH1F* min = new TH1F(m_cfg.mintag.c_str(), "min; bin", binningBins, -0.5,
                           binningBins - 0.5);

      // The binning option information
      TH1F* max = new TH1F(m_cfg.maxtag.c_str(), "max; bin", binningBins, -0.5,
                           binningBins - 0.5);

      // Now fill the histogram content
      std::size_t b = 1;
      for (auto bData : binningData) {
        // Fill: nbins, value, option, min, max
        n->SetBinContent(b, static_cast<int>(binningData[b - 1].bins()));
        v->SetBinContent(b, static_cast<int>(binningData[b - 1].binvalue));
        o->SetBinContent(b, static_cast<int>(binningData[b - 1].option));
        min->SetBinContent(b, binningData[b - 1].min);
        max->SetBinContent(b, binningData[b - 1].max);
        ++b;
      }
      n->Write();
      v->Write();
      o->Write();
      min->Write();
      max->Write();
    }

    TH2F* t = new TH2F(m_cfg.ttag.c_str(), "thickness [mm] ;b0 ;b1", bins0,
                       -0.5, bins0 - 0.5, bins1, -0.5, bins1 - 0.5);
    TH2F* x0 = new TH2F(m_cfg.x0tag.c_str(), "X_{0} [mm] ;b0 ;b1", bins0, -0.5,
                        bins0 - 0.5, bins1, -0.5, bins1 - 0.5);
    TH2F* l0 = new TH2F(m_cfg.l0tag.c_str(), "#Lambda_{0} [mm] ;b0 ;b1", bins0,
                        -0.5, bins0 - 0.5, bins1, -0.5, bins1 - 0.5);
    TH2F* A = new TH2F(m_cfg.atag.c_str(), "X_{0} [mm] ;b0 ;b1", bins0, -0.5,
                       bins0 - 0.5, bins1, -0.5, bins1 - 0.5);
    TH2F* Z = new TH2F(m_cfg.ztag.c_str(), "#Lambda_{0} [mm] ;b0 ;b1", bins0,
                       -0.5, bins0 - 0.5, bins1, -0.5, bins1 - 0.5);
    TH2F* rho = new TH2F(m_cfg.rhotag.c_str(), "#rho [g/mm^3] ;b0 ;b1", bins0,
                         -0.5, bins0 - 0.5, bins1, -0.5, bins1 - 0.5);

    // loop over the material and fill
    if (bsm != nullptr) {
      const auto& materialMatrix = bsm->fullMaterial();
      for (auto [b1, materialVector] : Acts::enumerate(materialMatrix)) {
        for (auto [b0, mat] : Acts::enumerate(materialVector)) {
          t->SetBinContent(b0 + 1, b1 + 1, mat.thickness());
          x0->SetBinContent(b0 + 1, b1 + 1, mat.material().X0());
          l0->SetBinContent(b0 + 1, b1 + 1, mat.material().L0());
          A->SetBinContent(b0 + 1, b1 + 1, mat.material().Ar());
          Z->SetBinContent(b0 + 1, b1 + 1, mat.material().Z());
          rho->SetBinContent(b0 + 1, b1 + 1, mat.material().massDensity());
        }
      }
    } else if (bins1 == 1 && bins0 == 1) {
      // homogeneous surface
      auto mat = sMaterial->materialSlab(Acts::Vector3{0, 0, 0});
      t->SetBinContent(1, 1, mat.thickness());
      x0->SetBinContent(1, 1, mat.material().X0());
      l0->SetBinContent(1, 1, mat.material().L0());
      A->SetBinContent(1, 1, mat.material().Ar());
      Z->SetBinContent(1, 1, mat.material().Z());
      rho->SetBinContent(1, 1, mat.material().massDensity());
    }
    t->Write();
    x0->Write();
    l0->Write();
    A->Write();
    Z->Write();
    rho->Write();
  }

  auto& volumeMaps = detMaterial.second;
  for (auto& [key, value] : volumeMaps) {
    // Get the Volume material
    const Acts::IVolumeMaterial* vMaterial = value.get();
    if (vMaterial == nullptr) {
      ACTS_WARNING("No material for volume " << key << " skipping");
      continue;
    }

    // get the geometry ID
    Acts::GeometryIdentifier geoID = key;
    // decode the geometryID
    const auto gvolID = geoID.volume();

    // create the directory
    std::string tdName = m_cfg.folderVolumeNameBase.c_str();
    tdName += m_cfg.voltag + std::to_string(gvolID);

    // create a new directory
    m_outputFile->mkdir(tdName.c_str());
    m_outputFile->cd(tdName.c_str());

    ACTS_VERBOSE("Writing out map at " << tdName);

    // understand what sort of material you have in mind
    auto bvMaterial3D = dynamic_cast<const Acts::InterpolatedMaterialMap<
        Acts::MaterialMapper<Acts::MaterialGrid3D>>*>(vMaterial);
    auto bvMaterial2D = dynamic_cast<const Acts::InterpolatedMaterialMap<
        Acts::MaterialMapper<Acts::MaterialGrid2D>>*>(vMaterial);

    std::size_t points = 1;
    if (bvMaterial3D != nullptr || bvMaterial2D != nullptr) {
      // Get the binning data
      std::vector<Acts::BinningData> binningData;
      if (bvMaterial3D != nullptr) {
        binningData = bvMaterial3D->binUtility().binningData();
        Acts::MaterialGrid3D grid = bvMaterial3D->getMapper().getGrid();
        points = grid.size();
      } else {
        binningData = bvMaterial2D->binUtility().binningData();
        Acts::MaterialGrid2D grid = bvMaterial2D->getMapper().getGrid();
        points = grid.size();
      }

      // 2-D or 3-D maps
      std::size_t binningBins = binningData.size();

      // The bin number information
      TH1F* n = new TH1F(m_cfg.ntag.c_str(), "bins; bin", binningBins, -0.5,
                         binningBins - 0.5);

      // The binning value information
      TH1F* v = new TH1F(m_cfg.vtag.c_str(), "binning values; bin", binningBins,
                         -0.5, binningBins - 0.5);

      // The binning option information
      TH1F* o = new TH1F(m_cfg.otag.c_str(), "binning options; bin",
                         binningBins, -0.5, binningBins - 0.5);

      // The binning option information
      TH1F* min = new TH1F(m_cfg.mintag.c_str(), "min; bin", binningBins, -0.5,
                           binningBins - 0.5);

      // The binning option information
      TH1F* max = new TH1F(m_cfg.maxtag.c_str(), "max; bin", binningBins, -0.5,
                           binningBins - 0.5);

      // Now fill the histogram content
      std::size_t b = 1;
      for (auto bData : binningData) {
        // Fill: nbins, value, option, min, max
        n->SetBinContent(b, static_cast<int>(binningData[b - 1].bins()));
        v->SetBinContent(b, static_cast<int>(binningData[b - 1].binvalue));
        o->SetBinContent(b, static_cast<int>(binningData[b - 1].option));
        min->SetBinContent(b, binningData[b - 1].min);
        max->SetBinContent(b, binningData[b - 1].max);
        ++b;
      }
      n->Write();
      v->Write();
      o->Write();
      min->Write();
      max->Write();
    }

    TH1F* x0 = new TH1F(m_cfg.x0tag.c_str(), "X_{0} [mm] ;gridPoint", points,
                        -0.5, points - 0.5);
    TH1F* l0 = new TH1F(m_cfg.l0tag.c_str(), "#Lambda_{0} [mm] ;gridPoint",
                        points, -0.5, points - 0.5);
    TH1F* A = new TH1F(m_cfg.atag.c_str(), "X_{0} [mm] ;gridPoint", points,
                       -0.5, points - 0.5);
    TH1F* Z = new TH1F(m_cfg.ztag.c_str(), "#Lambda_{0} [mm] ;gridPoint",
                       points, -0.5, points - 0.5);
    TH1F* rho = new TH1F(m_cfg.rhotag.c_str(), "#rho [g/mm^3] ;gridPoint",
                         points, -0.5, points - 0.5);
    // homogeneous volume
    if (points == 1) {
      auto mat = vMaterial->material({0, 0, 0});
      x0->SetBinContent(1, mat.X0());
      l0->SetBinContent(1, mat.L0());
      A->SetBinContent(1, mat.Ar());
      Z->SetBinContent(1, mat.Z());
      rho->SetBinContent(1, mat.massDensity());
    } else {
      // 3d grid volume
      if (bvMaterial3D != nullptr) {
        Acts::MaterialGrid3D grid = bvMaterial3D->getMapper().getGrid();
        for (std::size_t point = 0; point < points; point++) {
          auto mat = Acts::Material(grid.at(point));
          if (mat.isValid()) {
            x0->SetBinContent(point + 1, mat.X0());
            l0->SetBinContent(point + 1, mat.L0());
            A->SetBinContent(point + 1, mat.Ar());
            Z->SetBinContent(point + 1, mat.Z());
            rho->SetBinContent(point + 1, mat.massDensity());
          }
        }
      }
      // 2d grid volume
      else if (bvMaterial2D != nullptr) {
        Acts::MaterialGrid2D grid = bvMaterial2D->getMapper().getGrid();
        for (std::size_t point = 0; point < points; point++) {
          auto mat = Acts::Material(grid.at(point));
          if (mat.isValid()) {
            x0->SetBinContent(point + 1, mat.X0());
            l0->SetBinContent(point + 1, mat.L0());
            A->SetBinContent(point + 1, mat.Ar());
            Z->SetBinContent(point + 1, mat.Z());
            rho->SetBinContent(point + 1, mat.massDensity());
          }
        }
      }
    }
    x0->Write();
    l0->Write();
    A->Write();
    Z->Write();
    rho->Write();
  }
}

ActsExamples::RootMaterialWriter::~RootMaterialWriter() {
  if (m_outputFile != nullptr) {
    m_outputFile->Close();
  }
}

void ActsExamples::RootMaterialWriter::write(
    const Acts::TrackingGeometry& tGeometry) {
  // Create a detector material map and loop recursively through it
  Acts::DetectorMaterialMaps detMatMap;
  auto hVolume = tGeometry.highestTrackingVolume();
  if (hVolume != nullptr) {
    collectMaterial(*hVolume, detMatMap);
  }
  // Write the resulting map to the file
  writeMaterial(detMatMap);
}

void ActsExamples::RootMaterialWriter::collectMaterial(
    const Acts::TrackingVolume& tVolume,
    Acts::DetectorMaterialMaps& detMatMap) {
  // If the volume has volume material, write that
  if (tVolume.volumeMaterialPtr() != nullptr && m_cfg.processVolumes) {
    detMatMap.second[tVolume.geometryId()] = tVolume.volumeMaterialPtr();
  }

  // If confined layers exist, loop over them and collect the layer material
  if (tVolume.confinedLayers() != nullptr) {
    ACTS_VERBOSE("Collecting material for " << tVolume.volumeName()
                                            << " layers");
    for (auto& lay : tVolume.confinedLayers()->arrayObjects()) {
      collectMaterial(*lay, detMatMap);
    }
  }

  // If any of the boundary surfaces has material collect that
  if (m_cfg.processBoundaries) {
    for (auto& bou : tVolume.boundarySurfaces()) {
      const auto& bSurface = bou->surfaceRepresentation();
      if (bSurface.surfaceMaterialSharedPtr() != nullptr) {
        detMatMap.first[bSurface.geometryId()] =
            bSurface.surfaceMaterialSharedPtr();
      }
    }
  }

  // If the volume has sub volumes, step down
  if (tVolume.confinedVolumes() != nullptr) {
    for (auto& tvol : tVolume.confinedVolumes()->arrayObjects()) {
      collectMaterial(*tvol, detMatMap);
    }
  }
}

void ActsExamples::RootMaterialWriter::collectMaterial(
    const Acts::Layer& tLayer, Acts::DetectorMaterialMaps& detMatMap) {
  // If the representing surface has material, collect it
  const auto& rSurface = tLayer.surfaceRepresentation();
  if (rSurface.surfaceMaterialSharedPtr() != nullptr &&
      m_cfg.processRepresenting) {
    detMatMap.first[rSurface.geometryId()] =
        rSurface.surfaceMaterialSharedPtr();
  }

  // Check the approach surfaces
  if (tLayer.approachDescriptor() != nullptr && m_cfg.processApproaches) {
    for (auto& aSurface : tLayer.approachDescriptor()->containedSurfaces()) {
      if (aSurface->surfaceMaterialSharedPtr() != nullptr) {
        detMatMap.first[aSurface->geometryId()] =
            aSurface->surfaceMaterialSharedPtr();
      }
    }
  }

  // Check the sensitive surfaces
  if (tLayer.surfaceArray() != nullptr && m_cfg.processSensitives) {
    // sensitive surface loop
    for (auto& sSurface : tLayer.surfaceArray()->surfaces()) {
      if (sSurface->surfaceMaterialSharedPtr() != nullptr) {
        detMatMap.first[sSurface->geometryId()] =
            sSurface->surfaceMaterialSharedPtr();
      }
    }
  }
}
