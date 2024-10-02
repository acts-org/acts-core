// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/MagneticField/FieldMapRootIo.hpp"

#include "Acts/MagneticField/BFieldMapUtils.hpp"

#include <stdexcept>
#include <vector>

#include <RtypesCore.h>
#include <TFile.h>
#include <TTree.h>

ActsExamples::detail::InterpolatedMagneticField2
ActsExamples::makeMagneticFieldMapRzFromRoot(
    const std::function<std::size_t(std::array<std::size_t, 2> binsRZ,
                                    std::array<std::size_t, 2> nBinsRZ)>&
        localToGlobalBin,
    const std::string& fieldMapFile, const std::string& treeName,
    Acts::ActsScalar lengthUnit, Acts::ActsScalar BFieldUnit,
    bool firstQuadrant) {
  /// [1] Read in field map file
  // Grid position points in r and z
  std::vector<double> rPos;
  std::vector<double> zPos;
  // components of magnetic field on grid points
  std::vector<Acts::Vector2> bField;
  // [1] Read in file and fill values
  TFile* inputFile = TFile::Open(fieldMapFile.c_str());
  if (inputFile == nullptr) {
    throw std::runtime_error("file does not exist");
  }
  TTree* tree = inputFile->Get<TTree>(treeName.c_str());
  if (tree == nullptr) {
    throw std::runtime_error("object not found in file");
  }
  Int_t entries = tree->GetEntries();

  double r = 0, z = 0;
  double Br = 0, Bz = 0;

  tree->SetBranchAddress("r", &r);
  tree->SetBranchAddress("z", &z);

  tree->SetBranchAddress("Br", &Br);
  tree->SetBranchAddress("Bz", &Bz);

  // reserve size
  rPos.reserve(entries);
  zPos.reserve(entries);
  bField.reserve(entries);

  for (int i = 0; i < entries; i++) {
    tree->GetEvent(i);
    rPos.push_back(r);
    zPos.push_back(z);
    bField.push_back(Acts::Vector2(Br, Bz));
  }
  delete inputFile;
  /// [2] use helper function in core
  return Acts::fieldMapRZ(localToGlobalBin, rPos, zPos, bField, lengthUnit,
                          BFieldUnit, firstQuadrant);
}

ActsExamples::detail::InterpolatedMagneticField3
ActsExamples::makeMagneticFieldMapXyzFromRoot(
    const std::function<std::size_t(std::array<std::size_t, 3> binsXYZ,
                                    std::array<std::size_t, 3> nBinsXYZ)>&
        localToGlobalBin,
    const std::string& fieldMapFile, const std::string& treeName,
    Acts::ActsScalar lengthUnit, Acts::ActsScalar BFieldUnit,
    bool firstOctant) {
  /// [1] Read in field map file
  // Grid position points in x, y and z
  std::vector<double> xPos;
  std::vector<double> yPos;
  std::vector<double> zPos;
  // components of magnetic field on grid points
  std::vector<Acts::Vector3> bField;
  // [1] Read in file and fill values
  TFile* inputFile = TFile::Open(fieldMapFile.c_str());
  if (inputFile == nullptr) {
    throw std::runtime_error("file does not exist");
  }
  TTree* tree = inputFile->Get<TTree>(treeName.c_str());
  if (tree == nullptr) {
    throw std::runtime_error("object not found in file");
  }
  Int_t entries = tree->GetEntries();

  double x = 0, y = 0, z = 0;
  double Bx = 0, By = 0, Bz = 0;

  tree->SetBranchAddress("x", &x);
  tree->SetBranchAddress("y", &y);
  tree->SetBranchAddress("z", &z);

  tree->SetBranchAddress("Bx", &Bx);
  tree->SetBranchAddress("By", &By);
  tree->SetBranchAddress("Bz", &Bz);

  // reserve size
  xPos.reserve(entries);
  yPos.reserve(entries);
  zPos.reserve(entries);
  bField.reserve(entries);

  for (int i = 0; i < entries; i++) {
    tree->GetEvent(i);
    xPos.push_back(x);
    yPos.push_back(y);
    zPos.push_back(z);
    bField.push_back(Acts::Vector3(Bx, By, Bz));
  }
  delete inputFile;

  return Acts::fieldMapXYZ(localToGlobalBin, xPos, yPos, zPos, bField,
                           lengthUnit, BFieldUnit, firstOctant);
}
