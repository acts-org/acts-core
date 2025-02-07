/*
 * SPDX-PackageName: "ACTS"
 * SPDX-FileCopyrightText: 2016 CERN
 * SPDX-License-Identifier: MPL-2.0
 */

#include <cmath>
#include <iostream>
#include <map>
#include <numbers>
#include <string>
#include <tuple>

#include <TCanvas.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TH1F.h>
#include <TProfile.h>
#include <TTree.h>

struct MaterialHistograms {
  TProfile* x0_vs_eta = nullptr;
  TProfile* l0_vs_eta = nullptr;

  TProfile* x0_vs_phi = nullptr;
  TProfile* l0_vs_phi = nullptr;

  float s_x0 = 0.;
  float s_l0 = 0.;

  MaterialHistograms() = default;

  /// Material histogram constructor
  ///
  /// @param iA the atomic number
  MaterialHistograms(const std::string& name, unsigned int iA,
                     unsigned int bins, float eta) {
    std::string x0NameEta =
        (iA == 0) ? name + std::string("_x0_vs_eta_all")
                  : name + std::string("_x0_vs_eta_A") + std::to_string(iA);
    std::string l0NameEta =
        (iA == 0) ? name + std::string("_l0_vs_eta_all")
                  : name + std::string("_l0_vs_eta_A") + std::to_string(iA);

    x0_vs_eta =
        new TProfile(x0NameEta.c_str(), "X_{0} vs. #eta", bins, -eta, eta);
    l0_vs_eta =
        new TProfile(l0NameEta.c_str(), "L_{0} vs. #eta", bins, -eta, eta);

    std::string x0NamePhi =
        (iA == 0) ? name + std::string("_x0_vs_phi_all")
                  : name + std::string("_x0_vs_phi_A") + std::to_string(iA);
    std::string l0NamePhi =
        (iA == 0) ? name + std::string("_l0_vs_phi_all")
                  : name + std::string("_l0_vs_phi_A") + std::to_string(iA);

    x0_vs_phi =
        new TProfile(x0NamePhi.c_str(), "X_{0} vs. #phi", bins, -std::numbers::pi, std::numbers::pi);
    l0_vs_phi =
        new TProfile(l0NamePhi.c_str(), "L_{0} vs. #phi", bins, -std::numbers::pi, std::numbers::pi);
  }

  /// This fills the event into the histograms
  /// and clears the cache accordingly
  ///
  /// @param eta the pseudorapidity value
  /// @param phi the phi value
  ///
  void fillAndClear(float eta, float phi) {
    x0_vs_eta->Fill(eta, s_x0);
    l0_vs_eta->Fill(eta, s_l0);

    x0_vs_phi->Fill(phi, s_x0);
    l0_vs_phi->Fill(phi, s_l0);

    s_x0 = 0.;
    s_l0 = 0.;
  }

  /// Write out the histograms, the TDirectory needs
  /// to be set before
  ///
  /// Histograms with no contribution will not be
  /// written to file.
  void write() {
    if (x0_vs_eta->GetMaximum() > 0.) {
      x0_vs_eta->Write();
      l0_vs_eta->Write();

      x0_vs_phi->Write();
      l0_vs_phi->Write();
    }
  }
};

struct Region {
  std::string name;
  std::vector<std::tuple<float, float, float, float>> boxes;

  bool inside(float r, float z) const {
    for (const auto& [minR, maxR, minZ, maxZ] : boxes) {
      if (minR <= r && r < maxR && minZ <= z && z < maxZ) {
        return true;
      }
    }
    return false;
  }
};

/// Plot the material composition
///
/// @param inFile the input root file
/// @param treeNAme the input tree name (default: 'trackstates)
/// @param outFile the output root file
/// @param bins the number of bins
/// @param eta the eta range
void materialComposition(const std::string& inFile, const std::string& treeName,
                         const std::string& outFile, unsigned int bins,
                         float eta, const std::vector<Region>& regions) {
  // Open the input file & get the tree
  auto inputFile = TFile::Open(inFile.c_str());
  auto inputTree = dynamic_cast<TTree*>(inputFile->Get(treeName.c_str()));
  if (inputTree == nullptr) {
    return;
  }

  // Get the different atomic numbers
  TCanvas* materialCanvas =
      new TCanvas("materialCanvas", "Materials", 100, 100, 620, 400);
  materialCanvas->cd();
  // Draw all the atomic elements & get the histogram
  inputTree->Draw("mat_A>>hA(200,0.5,200.5)");
  TH1F* histA = dynamic_cast<TH1F*>(gDirectory->Get("hA"));
  if (histA == nullptr) {
    throw std::runtime_error{"Could not get the histogram"};
  }

  histA->Draw();

  auto outputFile = TFile::Open(outFile.c_str(), "recreate");

  float v_eta = 0;
  float v_phi = 0;
  std::vector<float>* stepLength = new std::vector<float>;
  std::vector<float>* stepX0 = new std::vector<float>;
  std::vector<float>* stepL0 = new std::vector<float>;
  std::vector<float>* stepA = new std::vector<float>;

  std::vector<float>* stepX = new std::vector<float>;
  std::vector<float>* stepY = new std::vector<float>;
  std::vector<float>* stepZ = new std::vector<float>;

  inputTree->SetBranchAddress("v_eta", &v_eta);
  inputTree->SetBranchAddress("v_phi", &v_phi);
  inputTree->SetBranchAddress("mat_step_length", &stepLength);

  inputTree->SetBranchAddress("mat_X0", &stepX0);
  inputTree->SetBranchAddress("mat_L0", &stepL0);
  inputTree->SetBranchAddress("mat_A", &stepA);

  inputTree->SetBranchAddress("mat_x", &stepX);
  inputTree->SetBranchAddress("mat_y", &stepY);
  inputTree->SetBranchAddress("mat_z", &stepZ);

  // Loop over all entries ---------------
  unsigned int entries = inputTree->GetEntries();

#ifdef BOOST_AVAILABLE
  std::cout << "*** Event Loop: " << std::endl;
  progress_display event_loop_progress(entries * regions.size());
#endif

  // Loop of the regions
  for (auto& region : regions) {
    const auto rName = region.name;

    // The material histograms ordered by atomic mass
    std::map<unsigned int, MaterialHistograms> mCache;

    // The material histograms for all
    mCache[0] = MaterialHistograms(rName, 0, bins, eta);
    for (unsigned int ib = 1; ib <= 200; ++ib) {
      if (histA->GetBinContent(ib) > 0.) {
        mCache[ib] = MaterialHistograms(rName, ib, bins, eta);
      }
    }

    for (unsigned int ie = 0; ie < entries; ++ie) {
      // Get the entry
      inputTree->GetEntry(ie);

#ifdef BOOST_AVAILABLE
      ++event_loop_progress;
#endif

      // Accumulate the material per track
      std::size_t steps = stepLength->size();
      for (unsigned int is = 0; is < steps; ++is) {
        float x = stepX->at(is);
        float y = stepY->at(is);
        float z = stepZ->at(is);
        float r = std::hypot(x, y);

        if (!region.inside(r, z)) {
          continue;
        }

        float step = stepLength->at(is);
        float X0 = stepX0->at(is);
        float L0 = stepL0->at(is);

        // The integral one
        auto& all = mCache[0];
        all.s_x0 += step / X0;
        all.s_l0 += step / L0;

        unsigned int sA = histA->FindBin(stepA->at(is));
        // The current one
        auto currentIt = mCache.find(sA);
        if (currentIt == mCache.end()) {
          throw std::runtime_error{"Unknown atomic number " +
                                   std::to_string(sA)};
        }
        auto& current = currentIt->second;
        current.s_x0 += step / X0;
        current.s_l0 += step / L0;
      }
      // Fill the profiles and clear the cache
      for (auto& [key, cache] : mCache) {
        cache.fillAndClear(v_eta, v_phi);
      }
    }
    // -----------------------------------

    for (auto [key, cache] : mCache) {
      cache.write();
    }
  }

  outputFile->Close();

  delete stepLength;
  delete stepX0;
  delete stepL0;
  delete stepA;

  delete stepX;
  delete stepY;
  delete stepZ;

  delete materialCanvas;
}
