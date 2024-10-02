// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/*
 * compareAssignedRealPos.C
 *
 *  Created on: 16 Dec 2016
 *      Author: jhrdinka
 */

#include <tuple>
#include "TFile.h"
#include "TH2F.h"
#include "TIterator.h"
#include "TROOT.h"
#include "TTree.h"

// This root script prints global real position of layers in darker
// color and the assigned positions in corresponding lighter color.
// All the layers which should be printed into one canvas need to be in the same
// file. Every layer has its own directory with the contained material
// histograms.
// This script is foreseen to use the input of scripts/layerMaterial.C

void
compareAssignedRealPos(std::string inFile,
                       std::string infile_geoRZ   = "",
                       std::string histName_geoRZ = "",
                       std::string infile_geoXY   = "",
                       std::string histName_geoXY = "")
{
  std::cout << "Opening file: " << inFile << std::endl;
  TFile  inputFile(inFile.c_str());
  TList* layers = inputFile.GetListOfKeys();
  std::cout << "Layers to print: " << std::endl;
  layers->Print();
  TIter    next(layers);
  TObject* obj = 0;

  int      entry   = 2;
  TCanvas* canvas1 = new TCanvas();
  TCanvas* canvas2 = new TCanvas();

  while ((obj = next())) {
    inputFile.cd();
    TDirectory* dir          = inputFile.GetDirectory(obj->GetName());
    TH2F*       r_z          = (TH2F*)dir->Get("r_z");
    TH2F*       r_z_assigned = (TH2F*)dir->Get("r_z_assigned");
    TH2F*       x_y          = (TH2F*)dir->Get("x_y");
    TH2F*       x_y_assigned = (TH2F*)dir->Get("x_y_assigned");
    if (entry == 17) entry = 20;
    if (entry == 10 || entry == 50) entry++;
    if (r_z && r_z_assigned) {
      canvas1->cd();
      r_z->SetStats(0);
      r_z->SetMarkerColor(TColor::GetColorDark(entry));
      r_z->SetMarkerStyle(6);
      r_z->GetXaxis()->SetTitle("z");
      r_z->GetYaxis()->SetTitle("r");
      r_z->Draw("same");

      r_z_assigned->SetStats(0);
      r_z_assigned->SetMarkerColor(TColor::GetColorBright(entry));
      r_z_assigned->SetMarkerStyle(6);
      r_z_assigned->GetXaxis()->SetTitle("z");
      r_z_assigned->GetYaxis()->SetTitle("r");
      r_z_assigned->Draw("same");

      r_z->SetDirectory(0);
      r_z_assigned->SetDirectory(0);
    }
    if (x_y && x_y_assigned) {
      canvas2->cd();
      x_y->SetStats(0);
      x_y->SetMarkerColor(TColor::GetColorDark(entry));
      x_y->SetMarkerStyle(6);
      x_y->GetXaxis()->SetTitle("x");
      x_y->GetYaxis()->SetTitle("y");
      x_y->Draw("same");

      x_y_assigned->SetStats(0);
      x_y_assigned->SetMarkerColor(TColor::GetColorBright(entry));
      x_y_assigned->SetMarkerStyle(6);
      x_y_assigned->GetXaxis()->SetTitle("x");
      x_y_assigned->GetYaxis()->SetTitle("y");
      x_y_assigned->Draw("same");

      x_y->SetDirectory(0);
      x_y_assigned->SetDirectory(0);
    }
    entry++;
  }

  inputFile.Close();

  std::cout << "Opening file: " << infile_geoRZ << std::endl;
  TFile inputFileRZ(infile_geoRZ.c_str());

  TH2F* geo_rz = (TH2F*)inputFileRZ.Get(histName_geoRZ.c_str());
  canvas1->cd();
  if (geo_rz)
    geo_rz->Draw("same");
  else
    std::cout << "Can not access histogram with name: " << histName_geoRZ
              << std::endl;
  geo_rz->SetDirectory(0);

  inputFileRZ.Close();

  std::cout << "Opening file: " << infile_geoXY << std::endl;
  TFile inputFileXY(infile_geoXY.c_str());

  TH2F* geo_xy = (TH2F*)inputFileXY.Get(histName_geoXY.c_str());
  canvas2->cd();
  if (geo_xy)
    geo_xy->Draw("same");
  else
    std::cout << "Can not access histogram with name: " << histName_geoXY
              << std::endl;
  geo_xy->SetDirectory(0);

  inputFileXY.Close();
}
