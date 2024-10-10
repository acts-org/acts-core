// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "TFile.h"
#include "TH1F.h"
#include "TROOT.h"
#include "TTree.h"

// This root script draws the histograms with the name "hist1Name" and the name
// "hist2Name" (and possibly also a third histogram with name "hist3Name") from
// the input file into the same canvas with the colors given

void
compareHistograms(std::string inFile1,
                  std::string hist1Name,
                  int         col1,
                  std::string inFile2,
                  std::string hist2Name,
                  int         col2,
                  std::string inFile3   = "",
                  std::string hist3Name = "",
                  int         col3      = 0)
{
  std::cout << "Opening file: " << inFile1 << std::endl;
  TFile inputFile1(inFile1.c_str());
  std::cout << "Opening file: " << inFile2 << std::endl;
  TFile inputFile2(inFile2.c_str());
  std::cout << "Comparing Histograms: " << hist1Name << " & " << hist2Name
            << std::endl;

  TH1F* h1 = (TH1F*)inputFile1.Get(hist1Name.c_str());
  TH1F* h2 = (TH1F*)inputFile2.Get(hist2Name.c_str());
  std::cout << "col1: " << col1 << ", col2: " << col2 << std::endl;

  h1->SetMarkerColor(col1);
  h1->SetLineColor(col1);
  h1->SetMarkerStyle(3);
  h1->Draw("");
  h2->SetMarkerColor(col2);
  h2->SetLineColor(col2);
  h2->Draw("same");
  TLegend* leg = new TLegend(0.72, 0.696, 0.99, 0.936);
  leg->AddEntry(h1, hist1Name.c_str());
  leg->AddEntry(h2, hist2Name.c_str());

  if (!inFile3.empty()) {
    TFile inputFile3(inFile3.c_str());
    TH1F* h3 = (TH1F*)inputFile3.Get(hist3Name.c_str());
    std::cout << " & " << hist3Name << std::endl;
    std::cout << "from file: " << inFile3 << std::endl;
    h3->SetMarkerColor(col3);
    h3->SetLineColor(col3);
    h3->Draw("same");
    h3->SetDirectory(0);
    leg->AddEntry(h3, hist3Name.c_str());
    inputFile3.Close();
  }

  leg->Draw();

  h1->SetDirectory(0);
  h2->SetDirectory(0);

  inputFile1.Close();
  inputFile2.Close();
}
