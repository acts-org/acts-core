// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <TROOT.h>

#include "materialPlotHelper.cpp"

#include <fstream>
#include <iostream>
#include <sstream>

// Draw the two 1D histograms for each surface.

void plot(std::vector<TH1F*> Map, const sinfo& surface_info, const std::string& name){

  std::string out_name = name+"/"+surface_info.name+"/"+surface_info.name+"_"+surface_info.idname;
  gSystem->Exec( Form("mkdir %s", (name+"/"+surface_info.name).c_str()) );

  // Disk
  if(surface_info.type == 2 || surface_info.type == 4){

    TText *vol = new TText(.1,.95,surface_info.name.c_str());
    vol->SetNDC();
    TText *surface = new TText(.1,.9,surface_info.id.c_str());
    surface->SetNDC();
    TText *surface_z = new TText(.1,.85,("Z = " + to_string(surface_info.pos)).c_str() );
    surface_z->SetNDC();

    TCanvas *c1 = new TCanvas("c1","mat_R",1200,1200);
    c1->SetRightMargin(0.14);
    c1->SetTopMargin(0.14);
    c1->SetLeftMargin(0.14);
    c1->SetBottomMargin(0.14);
    Map[0]->Draw("P HIST");
    vol->Draw();
    surface->Draw();
    surface_z->Draw();
    c1->Print( (out_name+"_R.pdf").c_str());
    //c1->Print( (out_name+"_X0.root").c_str());

    TCanvas *c2 = new TCanvas("c2","mat_Phi",1200,1200);
    c2->SetRightMargin(0.14);
    c2->SetTopMargin(0.14);
    c2->SetLeftMargin(0.14);
    c2->SetBottomMargin(0.14);
    Map[1]->Draw("P HIST");
    vol->Draw();
    surface->Draw();
    surface_z->Draw();
    c2->Print( (out_name+"_Phi.pdf").c_str());
    //c2->Print( (out_name+"_Phi.root").c_str());

    delete c1;
    delete c2;

    delete vol;
    delete surface;
    delete surface_z;
  }

  // Cylinder
  if(surface_info.type == 1){

    TText *vol = new TText(.1,.95,surface_info.name.c_str());
    vol->SetNDC();
    TText *surface = new TText(.1,.9,surface_info.id.c_str());
    surface->SetNDC();
    TText *surface_r = new TText(.1,.85,("R = " + to_string(surface_info.pos)).c_str() );
    surface_r->SetNDC();
    TCanvas *c1 = new TCanvas("c1","mat_Z",1200,1200);
    c1->SetRightMargin(0.14);
    c1->SetTopMargin(0.14);
    c1->SetLeftMargin(0.14);
    c1->SetBottomMargin(0.14);
    Map[0]->Draw("P HIST");
    vol->Draw();
    surface->Draw();
    surface_r->Draw();
    c1->Print( (out_name+"_Z.pdf").c_str());
    //c1->Print( (out_name+"_X0.root").c_str());

    TCanvas *c2 = new TCanvas("c2","mat_Phi",1200,1200);
    c2->SetRightMargin(0.14);
    c2->SetTopMargin(0.14);
    c2->SetLeftMargin(0.14);
    c2->SetBottomMargin(0.14);
    Map[1]->Draw("P HIST");
    vol->Draw();
    surface->Draw();
    surface_r->Draw();
    c2->Print( (out_name+"_Phi.pdf").c_str());
    //c2->Print( (out_name+"_Phi.root").c_str());

    delete c1;
    delete c2;

    delete vol;
    delete surface;
    delete surface_r;
  }
  return;
}

/// Initialise the two 1D histograms for each surface.

void Initialise_hist(std::vector<TH1F*>& surface_hist,
  const sinfo& surface_info){

  TH1F * Map;
  TH1F * Map_Phi;
  TH1F * Map_scale;
  TH1F * Map_scale_Phi;

  if(surface_info.type == 1){
    Map           = new TH1F(("Map_"+surface_info.idname).c_str(),("Map_"+surface_info.idname).c_str(),
                              50,-1*surface_info.range_max, surface_info.range_max);
    Map_Phi       = new TH1F(("Map_Phi_"+surface_info.idname).c_str(),("Map_Phi_"+surface_info.idname).c_str(),
                              50,-3.2,3.2);
    Map_scale     = new TH1F(("Map_scale_"+surface_info.idname).c_str(),("Map_scale_"+surface_info.idname).c_str(),
                              50,-1*surface_info.range_max, surface_info.range_max);
    Map_scale_Phi = new TH1F(("Map_scale_Phi_"+surface_info.idname).c_str(),("Map_scale_Phi_"+surface_info.idname).c_str(),
                                50,-3.2,3.2);
    Map->GetXaxis()->SetTitle("Z [mm]");
    Map->GetYaxis()->SetTitle("X0");
    Map_Phi->GetXaxis()->SetTitle("Phi");
    Map_Phi->GetYaxis()->SetTitle("X0");
  }

  if(surface_info.type == 2 || surface_info.type == 4){
    Map           = new TH1F(("Map_"+surface_info.idname).c_str(),("Map_"+surface_info.idname).c_str(),
                              50,surface_info.range_min, surface_info.range_max);
    Map_Phi       = new TH1F(("Map_Phi_"+surface_info.idname).c_str(),("Map_Phi_"+surface_info.idname).c_str(),
                              50,-3.2,3.2);
    Map_scale     = new TH1F(("Map_scale_"+surface_info.idname).c_str(),("Map_scale_"+surface_info.idname).c_str(),
                              50,surface_info.range_min, surface_info.range_max);
    Map_scale_Phi = new TH1F(("Map_scale_Phi_"+surface_info.idname).c_str(),("Map_scale_Phi_"+surface_info.idname).c_str(),
                                50,-3.2,3.2);
    Map->GetXaxis()->SetTitle("R [mm]");
    Map->GetYaxis()->SetTitle("X0");
    Map_Phi->GetXaxis()->SetTitle("Phi");
    Map_Phi->GetYaxis()->SetTitle("X0");
  }

  Map->SetMarkerStyle(20);
  Map_Phi->SetMarkerStyle(20);

  std::vector<TH1F*> v_hist;
  v_hist.push_back(Map);
  v_hist.push_back(Map_Phi);
  v_hist.push_back(Map_scale);
  v_hist.push_back(Map_scale_Phi);
  surface_hist = v_hist;
}

  /// Fill the two 1D histograms for each surfaces.

void Fill(std::map<std::uint64_t,std::vector<TH1F*>>& surface_hist,  std::map<std::uint64_t,sinfo>& surface_info,
  const std::string& input_file, const int& nbprocess){
  std::map<std::string,std::string> surface_name;
  std::map<std::uint64_t,float> surface_weight;

  //Get file, tree and set top branch address
  TFile *tfile = new TFile(input_file.c_str());
  TTree *tree = (TTree*)tfile->Get("material-tracks");

  float v_phi   = 0;
  float v_eta   = 0;
  std::vector<float> *mat_X0   = 0;
  std::vector<float> *mat_L0   = 0;
  std::vector<float> *mat_step_length = 0;

  std::vector<std::uint64_t> *sur_id = 0;
  std::vector<std::int32_t> *sur_type = 0;
  std::vector<float> *sur_x = 0;
  std::vector<float> *sur_y = 0;
  std::vector<float> *sur_z = 0;
  std::vector<float> *sur_range_min = 0;
  std::vector<float> *sur_range_max = 0;

  tree->SetBranchAddress("v_phi",&v_phi);
  tree->SetBranchAddress("v_eta",&v_eta);
  tree->SetBranchAddress("mat_X0",&mat_X0);
  tree->SetBranchAddress("mat_L0",&mat_L0);
  tree->SetBranchAddress("mat_step_length",&mat_step_length);

  tree->SetBranchAddress("sur_id",&sur_id);
  tree->SetBranchAddress("sur_type",&sur_type);
  tree->SetBranchAddress("sur_x",&sur_x);
  tree->SetBranchAddress("sur_y",&sur_y);
  tree->SetBranchAddress("sur_z",&sur_z);
  tree->SetBranchAddress("sur_range_min",&sur_range_min);
  tree->SetBranchAddress("sur_range_max",&sur_range_max);

  int nentries = tree->GetEntries();
  if(nentries > nbprocess && nbprocess != -1) nentries = nbprocess;
  // Loop over all the material tracks.
  for (Long64_t i=0;i<nentries; i++) {
    if(i%10000==0) std::cout << "processed " << i << " events out of " << nentries << std::endl;
    tree->GetEntry(i);

    // Reset the weight
    for (auto weight_it = surface_weight.begin(); weight_it != surface_weight.end(); weight_it++){
      weight_it->second = 0;
    }
    // loop over all the material hits to do initialisation and compute weight
    for(int j=0; j<mat_X0->size(); j++ ){

      // Ignore surface of incorrect type
      if(sur_type->at(j) == -1) continue;
      // If a surface was never encountered initialise the hist, info and weight
      if(surface_hist.find(sur_id->at(j))==surface_hist.end()){

        float pos;
        if(sur_type->at(j) == 1){
          pos = sqrt(sur_x->at(j)*sur_x->at(j)+sur_y->at(j)*sur_y->at(j));
        }
        if(sur_type->at(j) == 2 || sur_type->at(j) == 4){
          pos = sur_z->at(j);
        }
        // Weight for each surface = number of hit associated to it.
        surface_weight[sur_id->at(j)] = 0;
        Initialise_info(surface_info[sur_id->at(j)], surface_name, sur_id->at(j), sur_type->at(j), pos, sur_range_min->at(j), sur_range_max->at(j));
        Initialise_hist(surface_hist[sur_id->at(j)], surface_info[sur_id->at(j)]);
      }
      // Weight for each surface = number of hit associated to it.
      surface_weight[sur_id->at(j)]++;
    }

    // loop over all the material hit to fill the histogram
    for(int j=0; j<mat_X0->size(); j++ ){

      // Ignore surface of incorrect type
      if(sur_type->at(j) == -1) continue;

      if(sur_type->at(j) == 1){
        surface_hist[sur_id->at(j)][0]->Fill(sur_z->at(j), (mat_step_length->at(j)/mat_X0->at(j)));
        surface_hist[sur_id->at(j)][1]->Fill(v_phi, (mat_step_length->at(j)/mat_L0->at(j)));
        surface_hist[sur_id->at(j)][2]->Fill(sur_z->at(j), (1/surface_weight[sur_id->at(j)]));
        surface_hist[sur_id->at(j)][3]->Fill(v_phi, (1/surface_weight[sur_id->at(j)]));
      }
      if(sur_type->at(j) == 2 || sur_type->at(j) == 4){
        surface_hist[sur_id->at(j)][0]->Fill(sqrt(sur_x->at(j)*sur_x->at(j)+sur_y->at(j)*sur_y->at(j)), (mat_step_length->at(j)/mat_X0->at(j)));
        surface_hist[sur_id->at(j)][1]->Fill(v_phi, (mat_step_length->at(j)/mat_L0->at(j)));
        surface_hist[sur_id->at(j)][2]->Fill(sqrt(sur_x->at(j)*sur_x->at(j)+sur_y->at(j)*sur_y->at(j)), (1/surface_weight[sur_id->at(j)]));
        surface_hist[sur_id->at(j)][3]->Fill(v_phi, (1/surface_weight[sur_id->at(j)]));
      }
    }
  }
  // Normalise the histograms
  for (auto hist_it = surface_hist.begin(); hist_it != surface_hist.end(); hist_it++){
    hist_it->second[0]->Divide(hist_it->second[2]);
    hist_it->second[1]->Divide(hist_it->second[3]);
  }
}

/// Plot the material on each surface as function of Phi and Eta (two 1D plots).
/// nbprocess : number of parameter to be processed.
/// name : name of the output directory.

void Mat_map_surface_plot_1D(std::string input_file = "", int nbprocess = -1, std::string name = ""){

  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  std::map<std::uint64_t,std::vector<TH1F*>> surface_hist;
  std::map<std::uint64_t,sinfo> surface_info;

  Fill(surface_hist, surface_info, input_file, nbprocess);
  for (auto hist_it = surface_hist.begin(); hist_it != surface_hist.end(); hist_it++){
    plot(hist_it->second, surface_info[hist_it->first], name);
    for (auto hist : hist_it->second){
      delete hist;
    }
    hist_it->second.clear();
  }
}
