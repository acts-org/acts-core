// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
#include "TROOT.h"
#include "TTree.h"

// This script prints the histogram of a magnetic field map.
// To be used with the Output of the RootInterpolatedBFieldWriter.

/// to print out the FCC field map please use
/// @code
/// printBField("FCChhBField.root","bField","printBField_FCC.root",-20.,20.,-30.,30.,400.)
/// @endcode
/// ro print out the ATLAS BField map please use
/// @code
/// printBField("ATLASBField.root","bField","printBField_ATLAS.root",-10.,10.,-15.,15.,200.)
/// @endcode

/// @param inFile The root input file containing the magnetic field values and
/// positions either in cylinder (Branch names: 'r','z','Br','Bz') or cartesian
/// coordinates (Branch names: 'x','y','z','Bx','By','Bz')
/// @param the name of the tree containing the branches
/// @param rMin The minimum value of the position in either r (for cylinder
/// coordinates) or x/y (for cartesian coordinates) to be printed in [m]
/// @param rMin The minimum value of the position in either r (for cylinder
/// coordinates) or x/y (for cartesian coordinates) to be printed in [m]
/// @param rMin The maximum value of the position in either r (for cylinder
/// coordinates) or x/y (for cartesian coordinates) to be printed in [m]
/// @param rMin The minimum value of the position in z in [m]
/// @param rMin The maximum value of the position in z in [m]
/// @param nBins Number of bins which should be used for the histogram (on all
/// axes)
/// @note This script just writes out the values which are read in from the
/// given input file. It does no interpolation in between the values. This means,
/// in case the binning is chosen too high, empty bins will appear.
void
printBField(std::string inFile,
            std::string treeName,
            std::string outFile,
            float       rmin,
            float       rmax,
            float       zmin,
            float       zmax,
            int         nBins)
{
  const Int_t NRGBs        = 5;
  const Int_t NCont        = 255;
  Double_t    stops[NRGBs] = {0.00, 0.34, 0.61, 0.84, 1.00};
  Double_t    red[NRGBs]   = {0.00, 0.00, 0.87, 1.00, 0.51};
  Double_t    green[NRGBs] = {0.00, 0.81, 1.00, 0.20, 0.00};
  Double_t    blue[NRGBs]  = {0.51, 1.00, 0.12, 0.00, 0.00};
  TColor::CreateGradientColorTable(NRGBs, stops, red, green, blue, NCont);
  gStyle->SetNumberContours(NCont);
  gStyle->SetOptStat(0);

  std::cout << "Opening file: " << inFile << std::endl;
  TFile inputFile(inFile.c_str());
  std::cout << "Reading tree: " << treeName << std::endl;
  TTree* tree = (TTree*)inputFile.Get(treeName.c_str());

  TTreeReader reader(treeName.c_str(), &inputFile);

  double x = 0., y = 0., z = 0., r = 0.;
  double Bx = 0., By = 0., Bz = 0., Br = 0.;

  // find out if file is given in cylinder coordinates or cartesian coordinates
  bool cylinderCoordinates = false;
  if (tree->FindBranch("r")) {
    cylinderCoordinates = true;
    tree->SetBranchAddress("r", &r);
    tree->SetBranchAddress("Br", &Br);
  } else {
    tree->SetBranchAddress("x", &x);
    tree->SetBranchAddress("y", &y);
    tree->SetBranchAddress("Bx", &Bx);
    tree->SetBranchAddress("By", &By);
  }
  // should be given for sure
  tree->SetBranchAddress("z", &z);
  tree->SetBranchAddress("Bz", &Bz);

  Int_t entries = tree->GetEntries();
  std::cout << "Creating new output file: " << outFile
            << " and writing out histograms. " << std::endl;
  TFile outputFile(outFile.c_str(), "recreate");

  TProfile2D* bField_rz = new TProfile2D(
      "BField_rz", "Magnetic Field", nBins, zmin, zmax, nBins * 0.5, 0., rmax);
  bField_rz->GetXaxis()->SetTitle("z [m]");
  bField_rz->GetYaxis()->SetTitle("r [m]");
  TProfile2D* bField_xy = new TProfile2D(
      "BField_xy", "Magnetic Field", nBins, rmin, rmax, nBins, rmin, rmax);
  bField_xy->GetXaxis()->SetTitle("x [m]");
  bField_xy->GetYaxis()->SetTitle("y [m]");
  TProfile2D* bField_yz = new TProfile2D(
      "BField_yz", "Magnetic Field", nBins, zmin, zmax, nBins, rmin, rmax);
  bField_yz->GetXaxis()->SetTitle("z [m]");
  bField_yz->GetYaxis()->SetTitle("y [m]");
  TProfile2D* bField_xz = new TProfile2D(
      "BField_xz", "Magnetic Field", nBins, zmin, zmax, nBins, rmin, rmax);
  bField_xz->GetXaxis()->SetTitle("z [m]");
  bField_xz->GetYaxis()->SetTitle("x [m]");

  for (int i = 0; i < entries; i++) {
    tree->GetEvent(i);
    if (cylinderCoordinates) {
      float bFieldValue = std::hypot(Br, Bz);
      bField_rz->Fill(z / 1000., r / 1000., bFieldValue);
    } else {
      float bFieldValue = std::hypot(Bx, By, Bz);

      bField_xy->Fill(x / 1000., y / 1000., bFieldValue);
      bField_yz->Fill(z / 1000., y / 1000., bFieldValue);
      bField_xz->Fill(z / 1000., x / 1000., bFieldValue);
    }
  }
  inputFile.Close();

  if (!cylinderCoordinates) {
    bField_xy->Write();
    bField_yz->Write();
    bField_xz->Write();
  } else
    bField_rz->Write();

  delete bField_rz;
  delete bField_xy;
  delete bField_yz;
  delete bField_xz;

  outputFile.Close();
}
