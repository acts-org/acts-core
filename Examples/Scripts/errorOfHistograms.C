// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

/*
 * errorOfHistograms.C
 *
 *  Created on: 24 Jan 2017
 *      Author: jhrdinka
 */

#include <iostream>
#include "TFile.h"
#include "TH1F.h"
#include "TROOT.h"

// This root script compares either two 1D or two 2D histograms with each other,
// by plotting them in different colors into the same canvas and displaying the
// relative error beneath. The histograms to be compared can either be directly
// in the file or in a directory.

void
errorOf1DHistograms(std::string inFile1,
                    std::string hist1Name,
                    std::string legendName1,
                    std::string inFile2,
                    std::string hist2Name,
                    std::string legendName2,
                    std::string dirName1 = "",
                    std::string dirName2 = "")
{
  std::cout << "Opening file: " << inFile1 << std::endl;
  TFile inputFile1(inFile1.c_str());
  std::cout << "Opening file: " << inFile2 << std::endl;
  TFile inputFile2(inFile2.c_str());
  std::cout << "Comparing Histograms: " << hist1Name << " & " << hist2Name
            << std::endl;

  TH1F* h1 = nullptr;
  TH1F* h2 = nullptr;

  if (!dirName1.empty() && !dirName2.empty()) {
    TDirectory* dir1 = inputFile1.GetDirectory(dirName1.c_str());
    TDirectory* dir2 = inputFile2.GetDirectory(dirName2.c_str());
    h1               = (TH1F*)dir1->Get(hist1Name.c_str());
    h2               = (TH1F*)dir2->Get(hist2Name.c_str());
  } else {
    h1 = (TH1F*)inputFile1.Get(hist1Name.c_str());
    h2 = (TH1F*)inputFile2.Get(hist2Name.c_str());
  }

  h1->Sumw2();
  h2->Sumw2();

  int   bins = h1->GetXaxis()->GetNbins();
  float min  = h1->GetXaxis()->GetXmin();
  float max  = h1->GetXaxis()->GetXmax();
  std::cout << "creating new histogram with min: " << min << ", max: " << max
            << ", nBins: " << bins << std::endl;
  TH1F* error = new TH1F("error", "relative error", bins, min, max);

  if (!(bins == h2->GetXaxis()->GetNbins())) {
    std::cout << "Number of bins of the two histograms not equal : return"
              << std::endl;
    return;
  }
  for (int i = 0; i < bins; i++) {
    error->Fill(h1->GetBinCenter(i),
                (h1->GetBinContent(i) - h2->GetBinContent(i)));
  }
  TCanvas* c1 = new TCanvas();
  gStyle->SetOptStat(0);
  c1->Divide(1, 2);
  c1->cd(1);
  h1->SetMarkerColor(1);
  h1->SetLineColor(1);
  h1->Draw("");
  h2->SetMarkerColor(2);
  h2->SetLineColor(2);
  h2->Draw("same");
  TLegend* leg = new TLegend(0.72, 0.696, 0.99, 0.936);
  leg->AddEntry(h1, legendName1.c_str());
  leg->AddEntry(h2, legendName2.c_str());
  leg->Draw();
  h1->SetDirectory(0);
  h2->SetDirectory(0);
  c1->cd(2);
  error->Divide(h2);
  error->Draw("");
  error->SetDirectory(0);
  inputFile1.Close();
  inputFile2.Close();
}

void
errorOf2DHistograms(std::string inFile1,
                    std::string hist1Name,
                    std::string legendName1,
                    std::string inFile2,
                    std::string hist2Name,
                    std::string legendName2,
                    std::string dirName1 = "",
                    std::string dirName2 = "")
{
  std::cout << "Opening file: " << inFile1 << std::endl;
  TFile inputFile1(inFile1.c_str());
  std::cout << "Opening file: " << inFile2 << std::endl;
  TFile inputFile2(inFile2.c_str());
  std::cout << "Comparing Histograms: " << hist1Name << " & " << hist2Name
            << std::endl;

  TH2F* h1 = nullptr;
  TH2F* h2 = nullptr;

  if (!dirName1.empty() && !dirName2.empty()) {
    TDirectory* dir1 = inputFile1.GetDirectory(dirName1.c_str());
    TDirectory* dir2 = inputFile2.GetDirectory(dirName2.c_str());
    h1               = (TH2F*)dir1->Get(hist1Name.c_str());
    h2               = (TH2F*)dir2->Get(hist2Name.c_str());
  } else {
    h1 = (TH2F*)inputFile1.Get(hist1Name.c_str());
    h2 = (TH2F*)inputFile2.Get(hist2Name.c_str());
  }
  h1->Sumw2();
  h2->Sumw2();

  int   bins1 = h1->GetXaxis()->GetNbins();
  int   bins2 = h1->GetYaxis()->GetNbins();
  float min1  = h1->GetXaxis()->GetXmin();
  float max1  = h1->GetXaxis()->GetXmax();
  float min2  = h1->GetYaxis()->GetXmin();
  float max2  = h1->GetYaxis()->GetXmax();
  std::cout << "creating new histogram with min1: " << min1
            << ", max1: " << max1 << ", nBins1: " << bins1 << ", min2: " << min2
            << ", max2: " << max2 << ", nBins2: " << bins2 << std::endl;

  std::string title = "relative error";
  if (hist1Name == hist2Name)
    title += " of " + hist1Name + " in " + legendName1 + " and " + legendName2;
  else
    title += " of " + hist1Name + " in " + legendName1 + " and " + hist1Name
        + legendName2;
  TH2F* error
      = new TH2F("error", title.c_str(), bins1, min1, max1, bins2, min2, max2);

  if (!(bins1 == h2->GetXaxis()->GetNbins())
      && !(bins2 == h2->GetYaxis()->GetNbins())) {
    std::cout << "Number of bins of the two histograms not equal : return"
              << std::endl;
    return;
  }
  for (int i = 0; i < bins1; i++) {
    for (int j = 0; j < bins2; j++) {
      error->Fill(h1->GetXaxis()->GetBinCenter(i),
                  h1->GetYaxis()->GetBinCenter(j),
                  (h1->GetBinContent(i, j) - h2->GetBinContent(i, j)));
    }
  }
  TCanvas* c1 = new TCanvas();
  gStyle->SetOptStat(0);
  error->Divide(h2);
  error->Draw("colZ");

  error->SetDirectory(0);

  inputFile1.Close();
  inputFile2.Close();
}
