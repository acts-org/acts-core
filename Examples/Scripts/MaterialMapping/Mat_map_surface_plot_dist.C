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

// Draw the plot for each surface.
void plot(TGraph* Dist, const sinfo& surface_info, const std::string& name){

  std::string out_name = name+"/"+surface_info.name+"/"+surface_info.name+"_"+surface_info.idname;
  gSystem->Exec( Form("mkdir %s", (name+"/"+surface_info.name).c_str()) );

  TCanvas *c = new TCanvas("c","dist",1200,1200);
  c->SetRightMargin(0.14);
  c->SetTopMargin(0.14);
  c->SetLeftMargin(0.14);
  c->SetBottomMargin(0.14);

  Dist->Draw("AP");
  TLine *line_pos;

  TText *vol = new TText(.1,.95,surface_info.name.c_str());
  TText *surface = new TText(.1,.9,surface_info.id.c_str());
  TText *surface_z = new TText(.1,.85,("Z = " + to_string(surface_info.pos)).c_str() );
  TText *surface_r = new TText(.1,.85,("R = " + to_string(surface_info.pos)).c_str() );
  vol->SetNDC();
  surface->SetNDC();
  surface_z->SetNDC();
  surface_r->SetNDC();

  // Disk
  if(surface_info.type == 2 || surface_info.type == 4){
    vol->Draw();
    surface->Draw();
    surface_z->Draw();

    Dist->GetYaxis()->SetRangeUser(surface_info.range_min-(surface_info.range_max-surface_info.range_min)/10,
                                   surface_info.range_max+(surface_info.range_max-surface_info.range_min)/10);

    // Position of the disk surface
    line_pos = new TLine(surface_info.pos,surface_info.range_min,surface_info.pos,surface_info.range_max);
    line_pos->SetLineColor(kRed);
    line_pos->Draw("Same");
  }

  // Cylinder
  if(surface_info.type == 1){
    vol->Draw();
    surface->Draw();
    surface_r->Draw();

    Dist->GetYaxis()->SetRangeUser(surface_info.range_min-(surface_info.range_max-surface_info.range_min)/20,
                                   surface_info.range_max+(surface_info.range_max-surface_info.range_min)/20);

    // Position of the cylinder surface
    line_pos = new TLine(-1*surface_info.range_max, surface_info.pos, surface_info.range_max, surface_info.pos);
    line_pos->SetLineColor(kRed);
    line_pos->Draw("Same");
  }

  c->Print( (out_name+"_Dist.pdf").c_str());
  //c->Print( (out_name+"_Dist.root").c_str());

  delete c;

  delete vol;
  delete surface;
  delete surface_z;
  delete line_pos;
}


/// Create the Tgraphs from each surface based on a vector of positions.

void Initialise_hist(TGraph*& surface_hist,
  const std::pair<std::vector<float>,std::vector<float>>& surface_pos, const sinfo& surface_info){

  if(surface_info.type != -1){
    TGraph * Dist = new TGraph(surface_pos.first.size(), &surface_pos.second[0], &surface_pos.first[0]);
    Dist->Draw();
    Dist->GetXaxis()->SetTitle("Z [mm]");
    Dist->GetYaxis()->SetTitle("R [mm]");
    surface_hist = Dist;
  }
}

/// Fill the histograms for each surfaces.

void Fill(std::map<std::uint64_t,TGraph*>& surface_hist,  std::map<std::uint64_t,sinfo>& surface_info,
  const std::string& input_file, const int& nbprocess){
  std::map<std::string,std::string> surface_name;
  std::map<std::uint64_t,std::pair<std::vector<float>,std::vector<float>>> surface_pos;

  //Get file, tree and set top branch address
  TFile *tfile = new TFile(input_file.c_str());
  TTree *tree = (TTree*)tfile->Get("material-tracks");

  std::vector<float> *mat_x = 0;
  std::vector<float> *mat_y = 0;
  std::vector<float> *mat_z = 0;

  std::vector<std::uint64_t> *sur_id = 0;
  std::vector<std::int32_t> *sur_type = 0;
  std::vector<float> *sur_x = 0;
  std::vector<float> *sur_y = 0;
  std::vector<float> *sur_z = 0;
  std::vector<float> *sur_range_min = 0;
  std::vector<float> *sur_range_max = 0;

  tree->SetBranchAddress("mat_x",&mat_x);
  tree->SetBranchAddress("mat_y",&mat_y);
  tree->SetBranchAddress("mat_z",&mat_z);

  tree->SetBranchAddress("sur_id",&sur_id);
  tree->SetBranchAddress("sur_type",&sur_type);
  tree->SetBranchAddress("sur_x",&sur_x);
  tree->SetBranchAddress("sur_y",&sur_y);
  tree->SetBranchAddress("sur_z",&sur_z);
  tree->SetBranchAddress("sur_range_min",&sur_range_min);
  tree->SetBranchAddress("sur_range_max",&sur_range_max);

  int nentries = tree->GetEntries();
  if(nentries > nbprocess && nbprocess != -1) nentries = nbprocess;
  // Limit the number of event processed event to 10000
  // more could lead to errors with the TGraphs
  if(nentries > 10000){
    nentries = 10000;
    std::cout << "Number of event reduced to 10000" << std::endl;
  }
  // Loop over all the material tracks.
  for (Long64_t i=0;i<nentries; i++) {
    if(i%1000==0) std::cout << "processed " << i << " events out of " << nentries << std::endl;
    tree->GetEntry(i);

    // loop over all the material hits.
    for(int j=0; j<mat_x->size(); j++ ){

      // Ignore surface of incorrect type
      if(sur_type->at(j) == -1) continue;

      // If a surface was never encountered initialise the position info
      if(surface_hist.find(sur_id->at(j))==surface_hist.end()){

        float pos;
        float range;
        if(sur_type->at(j) == 1){
          pos = sqrt(sur_x->at(j)*sur_x->at(j)+sur_y->at(j)*sur_y->at(j));
        }
        if(sur_type->at(j) == 2 || sur_type->at(j) == 4){
          pos = sur_z->at(j);
        }
        Initialise_info(surface_info[sur_id->at(j)], surface_name, sur_id->at(j), sur_type->at(j), pos, sur_range_min->at(j), sur_range_max->at(j));
      }
      // Fill the vector of positions for each layer.
      surface_pos[sur_id->at(j)].first.push_back(sqrt(mat_y->at(j)*mat_y->at(j)+mat_x->at(j)*mat_x->at(j)));
      surface_pos[sur_id->at(j)].second.push_back(mat_z->at(j));

    }
  }
  // Use the vector of positions to create the TGraphs
  for (auto pos_it = surface_pos.begin(); pos_it != surface_pos.end(); pos_it++){
    Initialise_hist(surface_hist[pos_it->first], pos_it->second, surface_info[pos_it->first]);
  }
}


/// Plot the position of material interaction with respect with the associated surface.
/// nbprocess : number of parameter to be processed.
/// name : name of the output directory.


void Mat_map_surface_plot_dist(std::string input_file = "", int nbprocess = -1, std::string name = ""){

  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  std::map<std::uint64_t,TGraph*> surface_hist;
  std::map<std::uint64_t,sinfo> surface_info;
  Fill(surface_hist, surface_info, input_file, nbprocess);
  for (auto hist_it = surface_hist.begin(); hist_it != surface_hist.end(); hist_it++){
    if(hist_it->second)
    plot(hist_it->second, surface_info[hist_it->first], name);
  }
}
