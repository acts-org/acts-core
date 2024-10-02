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

// This root script creates different histograms displaying the sensitive
// material, the boundaries and the material of the detector in different views.
// The in file needs to be in the format created form the ExtrapolationTest.
// To plot two or three histograms of this kind  in one canvas the root script
// "compareHistogram.C" can be used.

void
printHits(std::string inFile,
          std::string treeName,
          std::string outFile,
          float       rmin,
          float       rmax,
          float       zmin,
          float       zmax,
          int         nBins)
{
  std::cout << "Opening file: " << inFile << std::endl;
  TFile inputFile(inFile.c_str());
  std::cout << "Reading tree: " << treeName << std::endl;
  TTree* tree = (TTree*)inputFile.Get(treeName.c_str());

  TTreeReader reader(treeName.c_str(), &inputFile);

  int   nHits;
  float eta;
  float theta;
  float zDir;

  std::vector<float>* x      = new std::vector<float>;
  std::vector<float>* y      = new std::vector<float>;
  std::vector<float>* z      = new std::vector<float>;
  std::vector<float>* sens   = new std::vector<float>;
  std::vector<float>* mat    = new std::vector<float>;
  std::vector<float>* bounds = new std::vector<float>;

  tree->SetBranchAddress("step_x", &x);
  tree->SetBranchAddress("step_y", &y);
  tree->SetBranchAddress("step_z", &z);
  tree->SetBranchAddress("sensitive", &sens);

  if (tree->FindBranch("boundary")) {
    std::cout << "No BoundarySteps are given." << std::endl;
    tree->SetBranchAddress("boundary", &bounds);
  }
  if (tree->FindBranch("material")) {
    std::cout << "No MaterialSteps are given." << std::endl;
    tree->SetBranchAddress("material", &mat);
  }
  Int_t entries = tree->GetEntries();
  std::cout << "Creating new output file: " << outFile
            << " and writing out histograms. " << std::endl;
  TFile outputFile(outFile.c_str(), "recreate");

  // full
  TH2F* Full_xy = new TH2F(
      "Full_xy", "Full material", nBins, rmin, rmax, nBins, rmin, rmax);
  TH2F* Full_zr = new TH2F(
      "Full_zr", "Full material", nBins, zmin, zmax, nBins, 0., rmax);

  // sensitive
  TH2F* Sens_xy = new TH2F(
      "Sens_xy", "Sensitive material", nBins, rmin, rmax, nBins, rmin, rmax);
  Sens_xy->GetXaxis()->SetTitle("x [mm]");
  Sens_xy->GetYaxis()->SetTitle("y [mm]");
  TH2F* Sens_zx = new TH2F(
      "Sens_zx", "Sensitive material", nBins, zmin, zmax, nBins, rmin, rmax);
  Sens_zx->GetXaxis()->SetTitle("z [mm]");
  Sens_zx->GetYaxis()->SetTitle("x [mm]");
  TH2F* Sens_zy = new TH2F(
      "Sens_zy", "Sensitive material", nBins, zmin, zmax, nBins, rmin, rmax);
  Sens_zy->GetXaxis()->SetTitle("z [mm]");
  Sens_zy->GetYaxis()->SetTitle("y [mm]");
  TH2F* Sens_zr = new TH2F(
      "Sens_zr", "Sensitive material", nBins, zmin, zmax, nBins, 0., rmax);
  Sens_zr->GetXaxis()->SetTitle("z [mm]");
  Sens_zr->GetYaxis()->SetTitle("r [mm]");

  // boundaries
  TH2F* Bounds_xy = new TH2F(
      "Bounds_xy", "Boundaries", nBins, rmin, rmax, nBins, rmin, rmax);
  Bounds_xy->GetXaxis()->SetTitle("x [mm]");
  Bounds_xy->GetYaxis()->SetTitle("y [mm]");
  TH2F* Bounds_zx = new TH2F(
      "Bounds_zx", "Boundaries", nBins, zmin, zmax, nBins, rmin, rmax);
  Bounds_zx->GetXaxis()->SetTitle("z [mm]");
  Bounds_zx->GetYaxis()->SetTitle("x [mm]");
  TH2F* Bounds_zy = new TH2F(
      "Bounds_zy", "Boundaries", nBins, zmin, zmax, nBins, rmin, rmax);
  Bounds_zy->GetXaxis()->SetTitle("z [mm]");
  Bounds_zy->GetYaxis()->SetTitle("y [mm]");
  TH2F* Bounds_zr
      = new TH2F("Bounds_zr", "Boundaries", nBins, zmin, zmax, nBins, 0., rmax);
  Bounds_zr->GetXaxis()->SetTitle("z [mm]");
  Bounds_zr->GetYaxis()->SetTitle("r [mm]");

  // material
  TH2F* Mat_xy
      = new TH2F("Mat_xy", "Material", nBins, rmin, rmax, nBins, rmin, rmax);
  Mat_xy->GetXaxis()->SetTitle("x [mm]");
  Mat_xy->GetYaxis()->SetTitle("y [mm]");
  TH2F* Mat_zx
      = new TH2F("Mat_zx", "Material", nBins, zmin, zmax, nBins, rmin, rmax);
  Mat_zx->GetXaxis()->SetTitle("z [mm]");
  Mat_zx->GetYaxis()->SetTitle("x [mm]");
  TH2F* Mat_zy
      = new TH2F("Mat_zy", "Material", nBins, zmin, zmax, nBins, rmin, rmax);
  Mat_zy->GetXaxis()->SetTitle("z [mm]");
  Mat_zy->GetYaxis()->SetTitle("y [mm]");
  TH2F* Mat_zr
      = new TH2F("Mat_zr", "Material", nBins, zmin, zmax, nBins, 0., rmax);
  Mat_zr->GetXaxis()->SetTitle("z [mm]");
  Mat_zr->GetYaxis()->SetTitle("r [mm]");

  for (int i = 0; i < entries; i++) {
    tree->GetEvent(i);

    for (int j = 0; j < x->size(); j++) {
      // sensitive
      if (z->at(j) >= zmin && z->at(j) <= zmax) {
        Sens_xy->Fill(x->at(j), y->at(j), sens->at(j));
        Full_xy->Fill(x->at(j), y->at(j));
      }
      Sens_zx->Fill(z->at(j), x->at(j), sens->at(j));
      Sens_zy->Fill(z->at(j), y->at(j), sens->at(j));
      Sens_zr->Fill(z->at(j), std::hypot(x->at(j), y->at(j)));
      Full_zr->Fill(z->at(j), std::hypot(x->at(j), y->at(j)));

      // boundaries
      if (tree->FindBranch("boundary")) {
        if (z->at(j) >= zmin && z->at(j) <= zmax)
          Bounds_xy->Fill(x->at(j), y->at(j), bounds->at(j));
        Bounds_zx->Fill(z->at(j), x->at(j), bounds->at(j));
        Bounds_zy->Fill(z->at(j), y->at(j), bounds->at(j));
        Bounds_zr->Fill(z->at(j),
                        std::hypot(x->at(j), y->at(j)),
                        bounds->at(j));
      }
      // material
      if (tree->FindBranch("material")) {
        if (z->at(j) >= zmin && z->at(j) <= zmax)
          Mat_xy->Fill(x->at(j), y->at(j), mat->at(j));
        Mat_zx->Fill(z->at(j), x->at(j), mat->at(j));
        Mat_zy->Fill(z->at(j), y->at(j), mat->at(j));
        Mat_zr->Fill(z->at(j),
                     std::hypot(x->at(j), y->at(j)),
                     mat->at(j));
      }
    }
  }
  inputFile.Close();

  // full
  Full_xy->Write();
  delete Full_xy;
  Full_zr->Write();
  delete Full_zr;

  // sensitive
  Sens_xy->Write();
  delete Sens_xy;
  Sens_zx->Write();
  delete Sens_zx;
  Sens_zy->Write();
  delete Sens_zy;
  Sens_zr->Write();
  delete Sens_zr;

  // boundaries
  Bounds_xy->Write();
  delete Bounds_xy;
  Bounds_zx->Write();
  delete Bounds_zx;
  Bounds_zy->Write();
  delete Bounds_zy;
  Bounds_zr->Write();
  delete Bounds_zr;

  // material
  Mat_xy->Write();
  delete Mat_xy;
  Mat_zx->Write();
  delete Mat_zx;
  Mat_zy->Write();
  delete Mat_zy;
  Mat_zr->Write();
  delete Mat_zr;

  delete x;
  delete y;
  delete z;
  delete sens;
  delete bounds;
  delete mat;

  outputFile.Close();
}
