/*
 * SPDX-PackageName: "ACTS"
 * SPDX-FileCopyrightText: 2016 CERN
 * SPDX-License-Identifier: MPL-2.0
 */

#include "Acts/Utilities/AngleHelpers.hpp"

#include <numbers>

#include "TFile.h"
#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
#include "TROOT.h"
#include "TTree.h"

// This root script creates different momentum distributions with the inFile
// created from the ExtrapolationTest.
// To plot two momentum distributions of this kind  in one canvas the root
// script "compareDistributions.C" can be used.

void momentumDistributions(std::string inFile, std::string treeName,
                           std::string outFile, int nBins, float r, float zMin,
                           float zMax, float etaMin, float etaMax,
                           float thetaMin = 0., float thetaMax = std::numbers::pi_v<float>) {
  std::cout << "Opening file: " << inFile << std::endl;
  TFile inputFile(inFile.c_str());
  std::cout << "Reading tree: " << treeName << std::endl;
  TTree* tree = (TTree*)inputFile.Get(treeName.c_str());

  int nHits;
  float eta;

  std::vector<float>* x = new std::vector<float>;
  std::vector<float>* y = new std::vector<float>;
  std::vector<float>* z = new std::vector<float>;

  tree->SetBranchAddress("nHits", &nHits);
  tree->SetBranchAddress("Eta", &eta);
  tree->SetBranchAddress("StepX", &x);
  tree->SetBranchAddress("StepY", &y);
  tree->SetBranchAddress("StepZ", &z);

  Int_t entries = tree->GetEntries();
  std::cout << "Creating new output file: " << outFile
            << " and writing "
               "material maps"
            << std::endl;
  TFile outputFile(outFile.c_str(), "recreate");

  // distributions of the number of hits versus momentum coordinates
  TProfile* nHits_eta = new TProfile("nHits_eta", "Hits in sensitive Material",
                                     nBins, etaMin, etaMax);
  nHits_eta->GetXaxis()->SetTitle("#eta");
  nHits_eta->GetYaxis()->SetTitle("#hits");
  TProfile* nHits_theta = new TProfile(
      "nHits_theta", "Hits in sensitive Material", nBins, thetaMin, thetaMax);
  nHits_theta->GetXaxis()->SetTitle("#theta [rad]");
  nHits_theta->GetYaxis()->SetTitle("#hits");
  TProfile* nHits_z =
      new TProfile("nHits_z", "Hits in sensitive Material", nBins, zMin, zMax);
  nHits_z->GetXaxis()->SetTitle("z coordinate of momentum [mm]");
  nHits_z->GetYaxis()->SetTitle("#hits");

  // distributions of the momentum coordinates
  TH1F* Eta = new TH1F("eta", "Distribution of #eta", nBins, etaMin, etaMax);
  Eta->GetXaxis()->SetTitle("#eta");
  Eta->GetYaxis()->SetTitle("#events");
  // distributions of the momentum coordinates calculated from eta - since in
  // the extrapolation test eta is flat randomly generated and theta and z are
  // calculated from eta.
  TH1F* Theta =
      new TH1F("theta", "Distribution of #theta", nBins, thetaMin, thetaMax);
  Theta->GetXaxis()->SetTitle("#theta [rad]");
  Theta->GetYaxis()->SetTitle("#events");
  TH1F* Z = new TH1F("z", "Distribution of z coordinate of the momentum", nBins,
                     zMin, zMax);
  Z->GetXaxis()->SetTitle("z coordinate of momentum [mm]");
  Z->GetYaxis()->SetTitle("#events");

  // hit distributions
  TH1F* hitsEta =
      new TH1F("hitsEta", "Sensitive Hit Distribution", nBins, etaMin, etaMax);
  hitsEta->GetXaxis()->SetTitle("#eta");
  hitsEta->GetYaxis()->SetTitle("#hits");
  TH1F* hitsTheta = new TH1F("hitsTheta", "Sensitive Hit Distribution", nBins,
                             thetaMin, thetaMax);
  hitsTheta->GetXaxis()->SetTitle("#theta");
  hitsTheta->GetYaxis()->SetTitle("#hits");
  TH1F* hitsZ =
      new TH1F("hitsZ", "Sensitive Hit Distribution", nBins, zMin, zMax);
  hitsZ->GetXaxis()->SetTitle("z [mm]");
  hitsZ->GetYaxis()->SetTitle("#hits");

  for (int i = 0; i < entries; i++) {
    tree->GetEvent(i);
    double theta = 2. * atan(exp(-eta));
    double zDir = r / tan(theta);

    nHits_eta->Fill(eta, nHits);
    nHits_theta->Fill(theta, nHits);
    nHits_z->Fill(zDir, nHits);

    Eta->Fill(eta, 1);
    Theta->Fill(theta);
    Z->Fill(zDir, 1);

    for (int j = 0; j < x->size(); j++) {
      float hitTheta = std::atan2(std::hypot(x->at(j), y->at(j)), z->at(j));
      hitsEta->Fill(Acts::AngleHelpers::etaFromTheta(hitTheta));
      hitsTheta->Fill(hitTheta);
      hitsZ->Fill(z->at(j));
    }
  }
  inputFile.Close();

  nHits_eta->Write();
  nHits_theta->Write();
  nHits_z->Write();

  Eta->Write();
  Theta->Write();
  Z->Write();

  hitsEta->Write();
  hitsTheta->Write();
  hitsZ->Write();

  delete nHits_eta;
  delete nHits_theta;
  delete nHits_z;

  delete Eta;
  delete Theta;
  delete Z;

  delete hitsEta;
  delete hitsTheta;
  delete hitsZ;

  delete x;
  delete y;
  delete z;

  outputFile.Close();
}
