/*
 * SPDX-PackageName: "ACTS"
 * SPDX-FileCopyrightText: 2016 CERN
 * SPDX-License-Identifier: MPL-2.0
 */

/*
 * compareHitHistograms.C
 *
 *  Created on: 15 Dec 2016
 *      Author: jhrdinka
 */

#include <tuple>
#include "TFile.h"
#include "TH1F.h"
#include "TROOT.h"
#include "TTree.h"

// This root script draws all histograms given in a vector into the same Canvas.
// The information should be handed over as a vector of a tuple.
// The first entry should be the Name of the file, where the histogram can be
// found. The second entry should be the name of the histogram. The third entry
// is the color in which the histogram should be printed.

void
compareHitHistograms(
    std::vector<std::tuple<std::string, std::string, int>> hist)
{
  for (auto& i : hist) {
    std::cout << "Opening file: " << std::get<0>(i).c_str() << std::endl;
    TFile      inputFile(std::get<0>(i).c_str());
    TDirectory dir = outputFile.mkdir(layerName.c_str());
    dir->cd();

    std::cout << "Comparing Histogram: " << std::get<1>(i).c_str() << std::endl;

    TH2F* h = (TH2F*)inputFile.Get(std::get<1>(i).c_str());
    h->SetMarkerColor(std::get<2>(i));
    h->Draw("same");
    h->SetDirectory(0);

    inputFile.Close();
  }
}
