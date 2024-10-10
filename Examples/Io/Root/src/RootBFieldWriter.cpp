// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/Root/RootBFieldWriter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Units.hpp"
#include "Acts/MagneticField/InterpolatedBFieldMap.hpp"
#include "Acts/MagneticField/MagneticFieldContext.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <cassert>
#include <ios>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include <TFile.h>
#include <TTree.h>

namespace ActsExamples {

/// Write down an interpolated magnetic field map
void RootBFieldWriter::run(const Config& config,
                           std::unique_ptr<const Acts::Logger> p_logger) {
  // Set up (local) logging
  // @todo Remove dangerous using declaration once the logger macro
  // tolerates it
  using namespace Acts;
  ACTS_LOCAL_LOGGER(std::move(p_logger))

  Acts::MagneticFieldContext bFieldContext;

  // Check basic configuration
  if (config.treeName.empty()) {
    throw std::invalid_argument("Missing tree name");
  } else if (config.fileName.empty()) {
    throw std::invalid_argument("Missing file name");
  } else if (config.bField == nullptr) {
    throw std::invalid_argument("Missing interpolated magnetic field");
  }

  // Setup ROOT I/O
  ACTS_INFO("Registering new ROOT output File : " << config.fileName);
  TFile* outputFile =
      TFile::Open(config.fileName.c_str(), config.fileMode.c_str());
  if (outputFile == nullptr) {
    throw std::ios_base::failure("Could not open '" + config.fileName + "'");
  }
  TTree* outputTree = new TTree(config.treeName.c_str(),
                                config.treeName.c_str(), 99, outputFile);
  if (outputTree == nullptr) {
    throw std::bad_alloc();
  }

  // Access the minima and maxima of all axes
  auto minima = config.bField->getMin();
  auto maxima = config.bField->getMax();
  auto nBins = config.bField->getNBins();

  if (logger().doPrint(Acts::Logging::VERBOSE)) {
    std::stringstream ss;
    ss << "Minimum:";
    for (auto m : minima) {
      ss << " " << m;
    }
    ACTS_VERBOSE(ss.str());
    ss.str("");
    ss << "Maximum:";
    for (auto m : maxima) {
      ss << " " << m;
    }
    ACTS_VERBOSE(ss.str());
    ss.str("");
    ss << "nBins:";
    for (auto m : nBins) {
      ss << " " << m;
    }
    ACTS_VERBOSE(ss.str());
  }

  if (config.gridType == GridType::xyz) {
    ACTS_INFO("Map will be written out in cartesian coordinates (x,y,z).");

    // Write out the interpolated magnetic field map
    double minX = 0., minY = 0., minZ = 0.;
    double maxX = 0., maxY = 0., maxZ = 0.;
    std::size_t nBinsX = 0, nBinsY = 0, nBinsZ = 0;

    // The position values in xyz
    double x = 0;
    outputTree->Branch("x", &x);
    double y = 0;
    outputTree->Branch("y", &y);
    double z = 0;
    outputTree->Branch("z", &z);

    // The BField values in xyz
    double Bx = 0;
    outputTree->Branch("Bx", &Bx);
    double By = 0;
    outputTree->Branch("By", &By);
    double Bz = 0;
    outputTree->Branch("Bz", &Bz);

    // check if range is user defined
    if (config.rBounds && config.zBounds) {
      ACTS_INFO("User defined ranges handed over.");

      // print out map in user defined range
      minX = config.rBounds->at(0);
      minY = config.rBounds->at(0);
      minZ = config.zBounds->at(0);

      maxX = config.rBounds->at(1);
      maxY = config.rBounds->at(1);
      maxZ = config.zBounds->at(1);

      nBinsX = config.rBins;
      nBinsY = config.rBins;
      nBinsZ = config.zBins;

    } else {
      ACTS_INFO("No user defined ranges handed over - write out whole map.");
      // write out whole map

      // check dimension of Bfieldmap
      if (minima.size() == 3 && maxima.size() == 3) {
        minX = minima.at(0);
        minY = minima.at(1);
        minZ = minima.at(2);

        maxX = maxima.at(0);
        maxY = maxima.at(1);
        maxZ = maxima.at(2);

        nBinsX = nBins.at(0);
        nBinsY = nBins.at(1);
        nBinsZ = nBins.at(2);

      } else if (minima.size() == 2 && maxima.size() == 2) {
        minX = -maxima.at(0);
        minY = -maxima.at(0);
        minZ = minima.at(1);

        maxX = maxima.at(0);
        maxY = maxima.at(0);
        maxZ = maxima.at(1);

        nBinsX = nBins.at(0);
        nBinsY = nBins.at(0);
        nBinsZ = nBins.at(1);
      } else {
        throw std::invalid_argument(
            "BField has wrong dimension. The dimension needs to be "
            "either 2 (r,z,Br,Bz) or 3(x,y,z,Bx,By,Bz) in order to be "
            "written out by this writer.");
      }
    }

    assert(maxX > minX);
    assert(maxY > minY);
    assert(maxZ > minZ);

    double stepX = (maxX - minX) / (nBinsX - 1);
    double stepY = (maxY - minY) / (nBinsY - 1);
    double stepZ = (maxZ - minZ) / (nBinsZ - 1);

    for (std::size_t i = 0; i < nBinsX; i++) {
      double raw_x = minX + i * stepX;
      for (std::size_t j = 0; j < nBinsY; j++) {
        double raw_y = minY + j * stepY;
        for (std::size_t k = 0; k < nBinsZ; k++) {
          double raw_z = minZ + k * stepZ;
          Acts::Vector3 position(raw_x, raw_y, raw_z);
          Vector3 bField = config.bField->getFieldUnchecked(position);

          x = raw_x / Acts::UnitConstants::mm;
          y = raw_y / Acts::UnitConstants::mm;
          z = raw_z / Acts::UnitConstants::mm;
          Bx = bField.x() / Acts::UnitConstants::T;
          By = bField.y() / Acts::UnitConstants::T;
          Bz = bField.z() / Acts::UnitConstants::T;
          outputTree->Fill();
        }  // for z
      }  // for y
    }  // for x

  } else {
    ACTS_INFO("Map will be written out in cylinder coordinates (r,z).");

    // The position value in rz
    double r = 0;
    outputTree->Branch("r", &r);
    double z = 0;
    outputTree->Branch("z", &z);
    // The BField value in rz
    double Br = 0;
    outputTree->Branch("Br", &Br);
    double Bz = 0;
    outputTree->Branch("Bz", &Bz);

    double minR = 0, maxR = 0;
    double minZ = 0, maxZ = 0;
    std::size_t nBinsR = 0, nBinsZ = 0;

    if (config.rBounds && config.zBounds) {
      ACTS_INFO("User defined ranges handed over.");

      minR = config.rBounds->at(0);
      minZ = config.zBounds->at(0);

      maxR = config.rBounds->at(1);
      maxZ = config.zBounds->at(1);

      nBinsR = config.rBins;
      nBinsZ = config.zBins;
    } else {
      ACTS_INFO("No user defined ranges handed over - printing out whole map.");

      if (minima.size() == 3 && maxima.size() == 3) {
        minR = 0.;
        minZ = minima.at(2);

        maxR = maxima.at(0);
        maxZ = maxima.at(2);

        nBinsR = nBins.at(0);
        nBinsZ = nBins.at(2);

      } else if (minima.size() == 2 || maxima.size() == 2) {
        minR = minima.at(0);
        minZ = minima.at(1);

        maxR = maxima.at(0);
        maxZ = maxima.at(1);

        nBinsR = nBins.at(0);
        nBinsZ = nBins.at(1);

      } else {
        throw std::invalid_argument(
            "BField has wrong dimension. The dimension needs to be "
            "either 2 (r,z,Br,Bz) or 3(x,y,z,Bx,By,Bz) in order to be "
            "written out by this writer.");
      }
    }

    assert(maxR > minR);
    assert(maxZ > minZ);

    double stepR = (maxR - minR) / (nBinsR - 1);
    double stepZ = (maxZ - minZ) / (nBinsZ - 1);

    for (std::size_t k = 0; k < nBinsZ; k++) {
      double raw_z = minZ + k * stepZ;
      for (std::size_t j = 0; j < nBinsR; j++) {
        double raw_r = minR + j * stepR;
        Acts::Vector3 position(raw_r, 0.0, raw_z);  // position at phi=0
        ACTS_VERBOSE("Requesting position: " << position.transpose());
        auto bField = config.bField->getFieldUnchecked(position);
        z = raw_z / Acts::UnitConstants::mm;
        r = raw_r / Acts::UnitConstants::mm;
        Bz = bField.z() / Acts::UnitConstants::T;
        Br = VectorHelpers::perp(bField) / Acts::UnitConstants::T;
        outputTree->Fill();
      }  // for R
    }  // for z
  }

  // Tear down ROOT I/O
  ACTS_INFO("Closing and Writing ROOT output File : " << config.fileName);
  outputTree->Write();
  delete outputFile;
}
}  // namespace ActsExamples
