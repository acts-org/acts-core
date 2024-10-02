// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <TROOT.h>

#include "Acts/Utilities/Helpers.hpp"

#include "materialPlotHelper.cpp"

#include <fstream>
#include <iostream>
#include <sstream>

/// Draw and save the histograms.

void plot(std::vector<TH2F*> Map, std::vector<int> detectors, const std::string& name){

  std::string sVol = "Detector volumes :";
  for(auto const& det: detectors) {
    sVol += " ";
    sVol += std::to_string(det);
  }
  TText *vol = new TText(.1, .95, sVol.c_str());
  vol->SetNDC();

  TCanvas *c1 = new TCanvas("c1","mat_X0",1200,1200);
  c1->SetRightMargin(0.14);
  c1->SetTopMargin(0.14);
  c1->SetLeftMargin(0.14);
  c1->SetBottomMargin(0.14);
  Map[0]->Draw("COLZ");
  vol->Draw();
  c1->Print( (name+"X0.pdf").c_str());

  TH2F *Unit_Map = (TH2F*) Map[2]->Clone();
  Unit_Map->Divide(Map[2]);
  TCanvas *c2 = new TCanvas("c2", "mat_X0/eta", 1200, 1200);
  c2->SetRightMargin(0.14);
  c2->SetTopMargin(0.14);
  c2->SetLeftMargin(0.14);
  c2->SetBottomMargin(0.14);
  TH1D *Proj_eta = Map[0]->ProjectionX();
  Proj_eta->Divide(Unit_Map->ProjectionX());
  Proj_eta->GetYaxis()->SetTitle("X0");
  Proj_eta->SetMarkerStyle(7);
  Proj_eta->Draw("HIST PC");
  c2->Print( (name+"X0_eta.pdf").c_str());
  TCanvas *c3 = new TCanvas("c3", "mat_X0/phi", 1200, 1200);
  c3->SetRightMargin(0.14);
  c3->SetTopMargin(0.14);
  c3->SetLeftMargin(0.14);
  c3->SetBottomMargin(0.14);
  TH1D *Proj_phi = Map[0]->ProjectionY();
  Proj_phi->Divide(Unit_Map->ProjectionY());
  Proj_phi->GetYaxis()->SetTitle("X0");
  Proj_phi->SetMarkerStyle(7);
  Proj_phi->Draw("HIST PC");
  c3->Print( (name+"X0_phi.pdf").c_str());

  delete c1;
  delete c2;
  delete c3;
  delete vol;
  delete Unit_Map;
  return;
}

/// Initialise the histograms for the detector.

void Initialise_hist(std::vector<TH2F*>& detector_hist){

  TH2F * Map_X0;
  TH2F * Map_L0;
  TH2F * Map_scale;

  Map_X0    = new TH2F("Map_X0_detector","Map_X0_detector",
                       100,-4,4,50,-3.2,3.2);
  Map_L0    = new TH2F("Map_L0_detector","Map_L0_detector",
                       100,-4,4,50,-3.2,3.2);
  Map_scale = new TH2F("Map_Scale_detector","Map_Scale_detector",
                       100,-4,4,50,-3.2,3.2);
  Map_X0->GetXaxis()->SetTitle("Eta");
  Map_X0->GetYaxis()->SetTitle("Phi");
  Map_X0->GetZaxis()->SetTitle("X0");
  Map_L0->GetXaxis()->SetTitle("Eta");
  Map_L0->GetYaxis()->SetTitle("Phi");
  Map_L0->GetZaxis()->SetTitle("L0");
  std::vector<TH2F*> v_hist;
  v_hist.push_back(Map_X0);
  v_hist.push_back(Map_L0);
  v_hist.push_back(Map_scale);
  detector_hist = v_hist;
}

/// Fill the histograms for the detector.

void Fill(std::vector<TH2F*>& detector_hist, const std::string& input_file, std::vector<int> detectors,  const int& nbprocess){


  Initialise_hist(detector_hist);

  //Get file, tree and set top branch address
  TFile *tfile = new TFile(input_file.c_str());
  TTree *tree = (TTree*)tfile->Get("material-tracks");

  float v_phi  = 0;
  float v_eta  = 0;
  float t_X0   = 0;
  std::vector<float> *mat_X0   = 0;
  std::vector<float> *mat_L0   = 0;
  std::vector<float> *mat_step_length = 0;

  std::vector<std::uint64_t> *sur_id = 0;
  std::vector<std::int32_t> *sur_type = 0;

  std::vector<std::uint64_t> *vol_id = 0;

  tree->SetBranchAddress("v_phi",&v_phi);
  tree->SetBranchAddress("v_eta",&v_eta);
  tree->SetBranchAddress("t_X0",&t_X0);
  tree->SetBranchAddress("mat_X0",&mat_X0);
  tree->SetBranchAddress("mat_L0",&mat_L0);
  tree->SetBranchAddress("mat_step_length",&mat_step_length);

  tree->SetBranchAddress("sur_id",&sur_id);
  tree->SetBranchAddress("sur_type",&sur_type);

  tree->SetBranchAddress("vol_id",&vol_id);

  int nentries = tree->GetEntries();
  if(nentries > nbprocess && nbprocess != -1) nentries = nbprocess;
  // Loop over all the material tracks.
  for (Long64_t i=0;i<nentries; i++) {
    if(i%10000==0) std::cout << "processed " << i << " events out of " << nentries << std::endl;
    tree->GetEntry(i);

    double matX0 = 0;
    double matL0 = 0;

    // loop over all the material hits
    for(int j=0; j<mat_X0->size(); j++ ){

      Acts::GeometryIdentifier ID;

      if(sur_id->at(j) != 0){
        ID = Acts::GeometryIdentifier(sur_id->at(j));
      }
      else if(vol_id->at(j) != 0){
        ID = Acts::GeometryIdentifier(vol_id->at(j));
      }

      // Check if the volume/surface is part of the selected ones
      if(rangeContainsValue(detectors, ID.volume())) {
        matX0 += mat_step_length->at(j) / mat_X0->at(j);
        matL0 += mat_step_length->at(j) / mat_L0->at(j);
      }
    }

    if (matX0 != 0 && matL0 != 0){
      detector_hist[0]->Fill(v_eta, v_phi, matX0);
      detector_hist[1]->Fill(v_eta, v_phi, matL0);
      detector_hist[2]->Fill(v_eta, v_phi);
    }
  }
  detector_hist[0]->Divide(detector_hist[2]);
  detector_hist[1]->Divide(detector_hist[2]);
}

/// Plot the material as function of eta and phi for a given detector/sub-detector
/// detectors : list of the ID of the volume constitutive of the detector/sub-detector
/// nbprocess : number of parameter to be processed.
/// name : name of the output directory.

void Mat_map_detector_plot(std::string input_file = "",
                         std::vector<int> detectors = vector<int>(), int nbprocess = -1,
                         std::string name = "") {
  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  std::vector<TH2F*> detector_hist;

  Fill(detector_hist, input_file, detectors, nbprocess);
  plot(detector_hist, detectors, name);
}
