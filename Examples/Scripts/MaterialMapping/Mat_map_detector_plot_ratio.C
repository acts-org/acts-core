// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Mat_map_detector_plot.C"

/// Draw and save the ratio plots.

void plot_ratio(std::vector<TH2F*> Map_prop, std::vector<TH2F*> Map_geant, std::vector<int> detectors, const std::string& name){


  TH2F *Unit_Map_prop = (TH2F*) Map_prop[2]->Clone();
  Unit_Map_prop->Divide(Map_prop[2]);
  TH2F *Unit_Map_geant = (TH2F*) Map_geant[2]->Clone();
  Unit_Map_geant->Divide(Map_geant[2]);

  TH1D *Proj_eta_prop = (TH1D*) Map_prop[0]->ProjectionX()->Clone();
  Proj_eta_prop->Divide(Unit_Map_prop->ProjectionX());
  TH1D *Proj_eta_geant = (TH1D*) Map_geant[0]->ProjectionX()->Clone();
  Proj_eta_geant->Divide(Unit_Map_geant->ProjectionX());

  TH1D *Proj_phi_prop = (TH1D*) Map_prop[0]->ProjectionY()->Clone();
  Proj_phi_prop->Divide(Unit_Map_prop->ProjectionY());
  TH1D *Proj_phi_geant = (TH1D*) Map_geant[0]->ProjectionY()->Clone();
  Proj_phi_geant->Divide(Unit_Map_geant->ProjectionY());

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
  Map_prop[0]->Divide(Map_geant[0]);
  Map_prop[0]->GetZaxis()->SetTitle("X0 Val/Geant");
  Map_prop[0]->SetMaximum(2.);
  Map_prop[0]->Draw("COLZ");
  vol->Draw();
  c1->Print( (name+"ratio_X0.pdf").c_str());

  TCanvas *c2 = new TCanvas("c2", "mat_X0/eta", 1200, 1200);
  c2->SetRightMargin(0.14);
  c2->SetTopMargin(0.14);
  c2->SetLeftMargin(0.14);
  c2->SetBottomMargin(0.14);

  Proj_eta_prop->Divide(Proj_eta_geant);
  Proj_eta_prop->GetYaxis()->SetTitle("X0 Val/Geant");
  Proj_eta_prop->SetMarkerStyle(7);
  Proj_eta_prop->Draw("HIST PC");
  c2->Print((name + "ratio_X0_eta.pdf").c_str());

  TCanvas *c3 = new TCanvas("c3", "mat_X0/phi", 1200, 1200);
  c3->SetRightMargin(0.14);
  c3->SetTopMargin(0.14);
  c3->SetLeftMargin(0.14);
  c3->SetBottomMargin(0.14);
  Proj_phi_prop->Divide(Proj_phi_geant);
  Proj_phi_prop->GetYaxis()->SetTitle("X0 Val/Geant");
  Proj_phi_prop->SetMarkerStyle(7);
  Proj_phi_prop->Draw("HIST PC");
  c3->Print((name + "ratio_X0_phi.pdf").c_str());

  delete c1;
  delete c2;
  delete c3;
  delete vol;
  delete Unit_Map_prop;
  delete Unit_Map_geant;
}


/// Plot the material ratio between the geantino scan and the map validation for each detector.
/// detectors : list of the ID of the volume constitutive of the detector/sub-detector
/// nbprocess : number of parameter to be processed
/// name : name of the output directory.
/// name_prop : name of the output directory for the map valdation.
/// name_geant : name of the output directory for the geantino scan.
/// The map valdation and geantino scan plots are only saved if name_prop and name_geant are defined.

void Mat_map_detector_plot_ratio(std::string input_file_prop = "", std::string input_file_geant = "", std::vector<int> detectors = vector<int>(), int nbprocess = -1, std::string name = "", std::string name_prop = "", std::string name_geant = ""){

  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  std::vector<TH2F*> detector_hist_prop;
  std::vector<TH2F*> detector_hist_geant;

  Fill(detector_hist_prop, input_file_prop, detectors, nbprocess);
  Fill(detector_hist_geant, input_file_geant, detectors, nbprocess);

  plot(detector_hist_prop, detectors, name_prop);
  plot(detector_hist_geant, detectors, name_geant);
  plot_ratio(detector_hist_prop, detector_hist_geant, detectors, name);
}
