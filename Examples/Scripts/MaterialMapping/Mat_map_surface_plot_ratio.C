// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Mat_map_surface_plot.C"

/// Draw and save the ratio plots.

void plot_ratio(std::vector<TH2F*> Map_prop, std::vector<TH2F*> Map_geant, const sinfo& surface_info, const std::string& name){

  std::string out_name = name+"/"+surface_info.name+"/"+surface_info.name+"_"+surface_info.idname;
  gSystem->Exec( Form("mkdir %s", (name+"/"+surface_info.name).c_str()) );

  // Disk
  if(surface_info.type == 2){

    TText *vol = new TText(.1,.95,surface_info.name.c_str());
    vol->SetNDC();
    TText *surface = new TText(.1,.9,surface_info.id.c_str());
    surface->SetNDC();
    TText *surface_z = new TText(.1,.85,("Z = " + to_string(surface_info.pos)).c_str() );
    surface_z->SetNDC();

    TCanvas *c1 = new TCanvas("c1","mat_X0",1200,1200);
    c1->SetRightMargin(0.14);
    c1->SetTopMargin(0.14);
    c1->SetLeftMargin(0.14);
    c1->SetBottomMargin(0.14);
    Map_prop[0]->Divide(Map_geant[0]);
    Map_prop[0]->GetZaxis()->SetTitle("X0 ratio");
    Map_prop[0]->SetMaximum(2.);
    Map_prop[0]->Draw("COLZ");
    vol->Draw();
    surface->Draw();
    surface_z->Draw();
    c1->Print( (out_name+"_X0.pdf").c_str());
    //c1->Print( (out_name+"_X0.root").c_str());

    delete c1;

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

    TCanvas *c1 = new TCanvas("c1","mat_X0",1200,1200);
    c1->SetRightMargin(0.14);
    c1->SetTopMargin(0.14);
    c1->SetLeftMargin(0.14);
    c1->SetBottomMargin(0.14);
    Map_prop[0]->Divide(Map_geant[0]);
    Map_prop[0]->GetZaxis()->SetTitle("X0 ratio");
    Map_prop[0]->SetMaximum(2.);
    Map_prop[0]->Draw("COLZ");
    vol->Draw();
    surface->Draw();
    surface_r->Draw();
    c1->Print( (out_name+"_X0.pdf").c_str());
    //c1->Print( (out_name+"_X0.root").c_str());

    delete c1;

    delete vol;
    delete surface;
    delete surface_r;
  }

  return;
}


/// Plot the material ratio between the geantino scan and the map validation for each surface.
/// nbprocess : number of parameter to be processed
/// name : name of the output directory.
/// name_prop : name of the output directory for the map valdation.
/// name_geant : name of the output directory for the geantino scan.
/// The map valdation and geantino scan plots are only saved if name_prop and name_geant are defined.

void Mat_map_surface_plot_ratio(std::string input_file_prop = "", std::string input_file_geant = "", int nbprocess = -1, std::string name = "", std::string name_prop = "", std::string name_geant = ""){

  gStyle->SetOptStat(0);
  gStyle->SetOptTitle(0);

  std::map<std::uint64_t,std::vector<TH2F*>> surface_hist_prop;
  std::map<std::uint64_t,sinfo> surface_info_prop;

  std::map<std::uint64_t,std::vector<TH2F*>> surface_hist_geant;
  std::map<std::uint64_t,sinfo> surface_info_geant;

  Fill(surface_hist_prop, surface_info_prop, input_file_prop, nbprocess);
  Fill(surface_hist_geant, surface_info_geant, input_file_geant, nbprocess);

  for (auto hist_it = surface_hist_prop.begin(); hist_it != surface_hist_prop.end(); hist_it++){
    if(name_prop != "") plot(hist_it->second, surface_info_prop[hist_it->first], name_prop);
    if(name_geant != "") plot(surface_hist_geant[hist_it->first], surface_info_geant[hist_it->first], name_geant);
    plot_ratio(hist_it->second,surface_hist_geant[hist_it->first], surface_info_prop[hist_it->first], name);

    for (auto hist : hist_it->second){
      delete hist;
    }
    hist_it->second.clear();
    for (auto hist : surface_hist_geant[hist_it->first]){
      delete hist;
    }
    surface_hist_geant[hist_it->first].clear();
  }

}
