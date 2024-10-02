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

/// This root script reads in two histograms with the names 'hist1Name' and
/// 'hist2Name' from the root file 'inFile' and draws them normalized with
/// different colors in one Canvas

void
compareDistributions(std::string inFile1,
                     std::string hist1Name,
                     int         col1,
                     std::string inFile2,
                     std::string hist2Name,
                     int         col2)
{
  std::cout << "Opening file: " << inFile1 << std::endl;
  TFile inputFile1(inFile1.c_str());
  std::cout << "Opening file: " << inFile2 << std::endl;
  TFile inputFile2(inFile2.c_str());
  std::cout << "Comparing Histograms: " << hist1Name << " & " << hist2Name
            << std::endl;

  TH1F* h1 = (TH1F*)inputFile1.Get(hist1Name.c_str());
  TH1F* h2 = (TH1F*)inputFile2.Get(hist2Name.c_str());

  h1->SetLineColor(col1);
  h1->DrawNormalized();
  h2->SetLineColor(col2);
  h2->DrawNormalized("same");

  TLegend* leg = new TLegend(0.72, 0.71, 0.99, 0.95);
  leg->AddEntry(h1, hist1Name.c_str());
  leg->AddEntry(h2, hist2Name.c_str());
  leg->Draw();

  h1->SetDirectory(0);
  h2->SetDirectory(0);

  inputFile1.Close();
  inputFile2.Close();
}
