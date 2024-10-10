// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <bitset>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <string>
#include <vector>

#include <TCanvas.h>
#include <TChain.h>
#include <TF1.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TH3F.h>
#include <TLegend.h>
#include <TMath.h>
#include <TStyle.h>
#include <TTree.h>
#include <TVectorF.h>

#include "CommonUtils.h"
#include "TreeReader.h"

using namespace ROOT;

/// This ROOT script will plot the residual and pull of perigee track parameters
/// (d0, z0, phi, theta, q/p, pT, t) from root file produced by the
/// RootTrackSummaryWriter
///
/// @param inFiles the list of input files
/// @param inTree the name of the input tree
/// @param outFile the name of the output file
/// @param inConfig the (optional) input configuration JSON file
/// @param outConfig the (optional) output configuration JSON file
/// @param residualPulls the bitset of the parameters set
///
///
/// @note A lot of this could be done with creating appropriate TH3F histograms
/// and chose the relevant projections, but this would need one loop through the
/// entries per parameter and would create a very long analysis turn-around.
/// That's why the data/histograms are prepared in handles and then filled in a
/// single event analysis loop.
///
int trackSummaryAnalysis(
    const std::vector<std::string>& inFiles, const std::string& inTree,
    const std::string& outFile, const std::string& inConfig = "",
    const std::string& outConfig = "", unsigned long nEntries = 0,
    unsigned int nPeakEntries = 0, float pullRange = 6.,
    unsigned int nHistBins = 61, unsigned int nPhiBins = 10,
    const std::array<float, 2>& phiRange = {-M_PI, M_PI},
    unsigned int nEtaBins = 10, const std::array<float, 2>& etaRange = {-3, 3},
    const std::vector<double>& ptBorders =
        {0., std::numeric_limits<double>::infinity()},
    const std::bitset<7>& residualPulls = std::bitset<7>{"1111111"},
    const std::bitset<5>& auxiliary = std::bitset<5>{"11111"}) {
  // Load the tree chain
  TChain* treeChain = new TChain(inTree.c_str());
  for (const auto& inFile : inFiles) {
    treeChain->Add(inFile.c_str());
    // Open root file written by RootTrackWriter
    std::cout << "*** Adding file: " << inFile << std::endl;
  }

  if (treeChain->GetEntries() == 0) {
    std::cout << "xxx No entries found ... bailing out." << std::endl;
    return -1;
  }

  TCanvas* rangeCanvas =
      new TCanvas("rangeCanvas", "Range Estimation", 100, 100, 620, 400);

  TrackSummaryReader tracks(treeChain, true);

  // Loop over the entries of the keys
  unsigned long entries = estimateEntries(*tracks.tree, nEntries);
  unsigned long peakEntries = estimateEntries(*tracks.tree, nPeakEntries);

  // Temporary counter
  unsigned int histBarcode = 0;

  // Deduction
  unsigned int nPtBins = ptBorders.size() - 1;

  // One time initialization of the residual and pull handles
  using ResidualPulls = std::vector<ResidualPullHandle>;
  ResidualPulls baseResidualPulls = {};

  if (residualPulls.test(0)) {
    // A standard d0Handle
    ResidualPullHandle d0Handle;
    d0Handle.tag = "d0";
    d0Handle.residualStr = "d_{0}^{rec} - d_{0}^{true}";
    d0Handle.residualUnit = "[mm]";
    d0Handle.errorStr = "#sigma(d_{0})";
    d0Handle.rangeDrawStr = "eLOC0_fit-t_d0";
    d0Handle.rangeMaxStr = "(1000,-10,10)";
    d0Handle.value = ResidualAccessor{tracks.eLOC0_fit, tracks.t_d0};
    d0Handle.error = DirectAccessor<float>{tracks.err_eLOC0_fit};
    d0Handle.accept = AcceptAll{};
    // Push it
    baseResidualPulls.push_back(d0Handle);
  }

  if (residualPulls.test(1)) {
    // A standard z0Handle
    ResidualPullHandle z0Handle;
    z0Handle.tag = "z0";
    z0Handle.residualStr = "z_{0}^{rec} - z_{0}^{true}";
    z0Handle.residualUnit = "[mm]";
    z0Handle.errorStr = "#sigma(z_{0})";
    z0Handle.rangeDrawStr = "eLOC1_fit-t_z0";
    z0Handle.rangeMaxStr = "(1000,-10,10)";
    z0Handle.value = ResidualAccessor{tracks.eLOC1_fit, tracks.t_z0};
    z0Handle.error = DirectAccessor<float>{tracks.err_eLOC1_fit};
    z0Handle.accept = AcceptAll{};
    // Push it
    baseResidualPulls.push_back(z0Handle);
  }

  if (residualPulls.test(2)) {
    // A standard phi0Handle
    ResidualPullHandle phi0Handle;
    phi0Handle.tag = "phi0";
    phi0Handle.residualStr = "#phi_{0}^{rec} - #phi_{0}^{true}";
    phi0Handle.errorStr = "#sigma(phi_{0})";
    phi0Handle.rangeDrawStr = "ePHI_fit-t_phi";
    phi0Handle.rangeMaxStr = "(1000,-0.01,0.01)";
    phi0Handle.value = ResidualAccessor{tracks.ePHI_fit, tracks.t_phi};
    phi0Handle.error = DirectAccessor<float>{tracks.err_ePHI_fit};
    phi0Handle.accept = AcceptAll{};
    // Push it
    baseResidualPulls.push_back(phi0Handle);
  }

  if (residualPulls.test(3)) {
    // A standard theta0Handle
    ResidualPullHandle theta0Handle;
    theta0Handle.tag = "theta0";
    theta0Handle.residualStr = "#theta_{0}^{rec} - #theta_{0}^{true}";
    theta0Handle.errorStr = "#sigma(theta_{0})";
    theta0Handle.rangeDrawStr = "eTHETA_fit-t_theta";
    theta0Handle.rangeMaxStr = "(1000,-0.01,0.01)";
    theta0Handle.value = ResidualAccessor{tracks.eTHETA_fit, tracks.t_theta};
    theta0Handle.error = DirectAccessor<float>{tracks.err_eTHETA_fit};
    theta0Handle.accept = AcceptAll{};
    // Push it
    baseResidualPulls.push_back(theta0Handle);
  }

  if (residualPulls.test(4)) {
    // The standard qop Handle
    ResidualPullHandle qopHandle;
    qopHandle.tag = "qop";
    qopHandle.residualStr = "q/p^{rec} - q/p^{true}";
    qopHandle.residualUnit = "[GeV^{-1}]";
    qopHandle.errorStr = "#sigma(q/p)";
    qopHandle.rangeDrawStr = "eQOP_fit-t_charge/t_p";
    qopHandle.rangeMaxStr = "(1000,-0.1,0.1)";
    qopHandle.value =
        QopResidualAccessor{tracks.eQOP_fit, tracks.t_charge, tracks.t_p};
    qopHandle.error = DirectAccessor<float>{tracks.err_eQOP_fit};
    qopHandle.accept = AcceptAll{};
    // Push it
    baseResidualPulls.push_back(qopHandle);
  }

  if (residualPulls.test(5)) {
    // The pt measurement
    ResidualPullHandle tHandle;
    tHandle.tag = "t";
    tHandle.residualStr = "t^{rec} - t^{true}";
    tHandle.residualUnit = "[ns]";
    tHandle.errorStr = "#sigma(t})";
    tHandle.rangeDrawStr = "eT_fit - t_time";
    tHandle.rangeMaxStr = "(1000,-10.,10.)";
    tHandle.value = ResidualAccessor{tracks.eT_fit, tracks.t_time};
    tHandle.error = DirectAccessor<float>{tracks.err_eT_fit};
    tHandle.accept = AcceptAll{};
    // Push it
    baseResidualPulls.push_back(tHandle);
  }

  if (residualPulls.test(6)) {
    // The pt measurement
    ResidualPullHandle ptHandle;
    ptHandle.tag = "pt";
    ptHandle.residualStr = "p_{T}^{rec} - p_{T}^{true}";
    ptHandle.residualUnit = "[GeV]";
    ptHandle.errorStr = "#sigma(p_{T})";
    ptHandle.rangeDrawStr = "1./abs(eQOP_fit) * sin(eTHETA_fit) - (1./t_pT)";
    ptHandle.rangeMaxStr = "(1000,-10.,10.)";
    ptHandle.value =
        PtResidualAccessor{tracks.eQOP_fit, tracks.eTHETA_fit, tracks.t_pT};
    ptHandle.error = PtErrorAccessor{tracks.eQOP_fit, tracks.err_eQOP_fit,
                                     tracks.eTHETA_fit, tracks.err_eTHETA_fit};
    ptHandle.accept = AcceptAll{};
    // Push it
    baseResidualPulls.push_back(ptHandle);
  }

  using Auxiliaries = std::vector<SingleHandle>;
  Auxiliaries baseAuxilaries;

  if (auxiliary.test(0)) {
    // Chi2/ndf
    SingleHandle chi2ndf;
    chi2ndf.tag = "chi2ndf";
    chi2ndf.label = "#Chi^{2}/ndf";
    chi2ndf.bins = nHistBins;
    chi2ndf.range = {0., 5.};
    chi2ndf.value =
        DivisionAccessor<float, unsigned int>{tracks.chi2Sum, tracks.NDF};
    chi2ndf.accept = AcceptAll{};
    // Push it
    baseAuxilaries.push_back(chi2ndf);
  }

  if (auxiliary.test(1)) {
    // Measurements
    SingleHandle measurements;
    measurements.tag = "measurements";
    measurements.label = "#(measurements)";
    measurements.rangeDrawStr = "nMeasurements";
    measurements.value = DirectAccessor<unsigned int>{tracks.nMeasurements};
    measurements.accept = AcceptAll{};
    estimateIntegerRange(measurements, *rangeCanvas, *tracks.tree, peakEntries,
                         250, 5, ++histBarcode);
    // Push it
    baseAuxilaries.push_back(measurements);
  }

  if (auxiliary.test(2)) {
    // Holes
    SingleHandle holes;
    holes.tag = "holes";
    holes.label = "#(holes)";
    holes.rangeDrawStr = "nHoles";
    holes.value = DirectAccessor<unsigned int>{tracks.nHoles};
    holes.accept = AcceptAll{};
    estimateIntegerRange(holes, *rangeCanvas, *tracks.tree, peakEntries, 10, 2,
                         ++histBarcode);
    // Push it
    baseAuxilaries.push_back(holes);
  }

  if (auxiliary.test(3)) {
    // Holes
    SingleHandle outliers;
    outliers.tag = "outliers";
    outliers.label = "#(outliers)";
    outliers.rangeDrawStr = "nOutliers";
    outliers.value = DirectAccessor<unsigned int>{tracks.nOutliers};
    outliers.accept = AcceptAll{};
    estimateIntegerRange(outliers, *rangeCanvas, *tracks.tree, peakEntries, 10,
                         2, ++histBarcode);
    // Push it
    baseAuxilaries.push_back(outliers);
  }

  if (auxiliary.test(4)) {
    // Holes
    SingleHandle shared;
    shared.tag = "shared";
    shared.label = "#(shared)";
    shared.rangeDrawStr = "nSharedHits";
    shared.value = DirectAccessor<unsigned int>{tracks.nSharedHits};
    shared.accept = AcceptAll{};
    estimateIntegerRange(shared, *rangeCanvas, *tracks.tree, peakEntries, 10, 2,
                         ++histBarcode);
    // Push it
    baseAuxilaries.push_back(shared);
  }

  // Preparation phase for handles
#ifdef NLOHMANN_AVAILABLE
  nlohmann::json handle_configs;
  if (! inConfig.empty()) {
    std::ifstream ifs(inConfig.c_str());
    handle_configs = nlohmann::json::parse(ifs);
  }

  /// Helper method to handle the range, it is either read in or estimated
  ///
  /// It attempts to read in from a json input, if that does not work,
  /// it will estimate it (time consuming)
  ///
  /// @param handle the parameter handle in question
  /// @param handleTag the unique tangle tag
  /// @param peakE the number of entries used for range peaking
  auto handleRange = [&](ResidualPullHandle& handle, const TString& handleTag, unsigned int peakE) -> void {
    bool rangeDetermined = false;
    if (! inConfig.empty()) {
      if (handle_configs.contains((handleTag).Data())) {
        auto handle_config = handle_configs[(handleTag).Data()];
        handle.range = handle_config["range"].get<std::array<float, 2>>();
        rangeDetermined = true;
      }
    }
    if (! rangeDetermined) {
      estimateResiudalRange(handle, *rangeCanvas, *tracks.tree, peakE,
                            ++histBarcode);
    }

    if (! outConfig.empty()) {
      nlohmann::json range_config;
      range_config["range"] = handle.range;
      handle_configs[(handleTag).Data()] = range_config;
    }
  };
#else
  /// Helper method to handle range without possibility to read in
  ///
  /// @param handle the parameter handle in question
  /// @param handleTag the unique tangle tag
  /// @param peakEntries the number of entries used for range peaking
  auto handleRange = [&](ResidualPullHandle& handle, const TString& handleTag,
                         unsigned long peakEntries) -> void {
    estimateResiudalRange(handle, *rangeCanvas, *tracks.tree, peakEntries,
                          ++histBarcode);
  };
#endif

  // Full Range handles - they accept all tracks
  ResidualPulls fullResidualPulls = baseResidualPulls;
  // Range Estimation and booking histogram
  for (auto& fHandle : fullResidualPulls) {
    // The full tag
    TString handleTag(fHandle.tag + std::string("_all"));
    handleRange(fHandle, handleTag, peakEntries);
    // Book histograms
    bookHistograms(fHandle, pullRange, nHistBins, ++histBarcode);
    // The Histogram names
    TString residualN = TString("res_") + handleTag;
    TString pullN = TString("pull_") + handleTag;
    // Style and name
    setHistStyle(fHandle.residualHist);
    fHandle.residualHist->SetName(residualN);
    setHistStyle(fHandle.pullHist);
    fHandle.pullHist->SetName(pullN);
  }

  // Regional/Binned  handles
  using ResidualPullsVector = std::vector<ResidualPulls>;
  using ResidualPullsMatrix = std::vector<ResidualPullsVector>;

  // Eta-Pt residual/pull handles
  ResidualPullsVector ptResidualPulls =
      ResidualPullsVector(nPtBins, baseResidualPulls);
  ResidualPullsMatrix etaPtResidualPulls =
      ResidualPullsMatrix(nEtaBins, ptResidualPulls);

  // Eta-Phi residual/pull handles
  ResidualPullsVector phiResidualPulls =
      ResidualPullsVector(nPhiBins, baseResidualPulls);
  ResidualPullsMatrix etaPhiResidualPulls =
      ResidualPullsMatrix(nEtaBins, phiResidualPulls);

  Auxiliaries fullAuxiliaries = baseAuxilaries;

  // Histogram booking for full range auxiliaries
  for (auto& fAuxiliary : fullAuxiliaries) {
    fAuxiliary.hist =
        new TH1F(fAuxiliary.tag.c_str(), fAuxiliary.tag.c_str(),
                 fAuxiliary.bins, fAuxiliary.range[0], fAuxiliary.range[1]);
    fAuxiliary.hist->GetXaxis()->SetTitle(fAuxiliary.label.c_str());
    fAuxiliary.hist->GetYaxis()->SetTitle("Entries");
    setHistStyle(fAuxiliary.hist);
  }

  using AuxiliariesVector = std::vector<Auxiliaries>;
  using AuxiliariesMatrix = std::vector<AuxiliariesVector>;

  // Eta-Pt auxiliaries
  AuxiliariesVector ptAuxiliaries = AuxiliariesVector(nPtBins, baseAuxilaries);
  AuxiliariesMatrix etaPtAuxiliaries =
      AuxiliariesMatrix(nEtaBins, ptAuxiliaries);

  // Eta-Phi auxiliaries
  AuxiliariesVector phiAuxiliaries =
      AuxiliariesVector(nPhiBins, baseAuxilaries);
  AuxiliariesMatrix etaPhiAuxiliaries =
      AuxiliariesMatrix(nEtaBins, phiAuxiliaries);

  // Loop over the binned handles & fill the acceptors
  float phiStep = (phiRange[1] - phiRange[0]) / nPhiBins;
  float etaStep = (etaRange[1] - etaRange[0]) / nEtaBins;

  TVectorF phiVals(nPhiBins + 1);
  TVectorF etaVals(nEtaBins + 1);
  TVectorF ptVals(nPtBins + 1);

  phiVals[0] = phiRange[0];
  etaVals[0] = etaRange[0];
  ptVals[0] = ptBorders[0];

#ifdef BOOST_AVAILABLE
  std::cout << "*** Handle Preparation: " << std::endl;
  progress_display handle_preparation_progress(
      nPhiBins * nEtaBins * nPtBins * baseResidualPulls.size());
#endif

  std::string land = " && ";

  /// Preparation of handles / acceptance range
  for (unsigned int iphi = 0; iphi < nPhiBins; ++iphi) {
    // Prepare the phi range for this batch
    float phiMin = phiRange[0] + iphi * phiStep;
    float phiMax = phiRange[0] + (iphi + 1) * phiStep;
    phiVals[iphi + 1] = phiMax;

    // Acceptance range
    AcceptRange phiBin{tracks.t_phi, {phiMin, phiMax}};
    // Name tag
    TString phiTag = "_phi";
    phiTag += iphi;
    // Range cut string
    std::string phiCut = "t_phi >= ";
    phiCut += std::to_string(phiMin);
    phiCut += land;
    phiCut += std::string("t_phi < ");
    phiCut += std::to_string(phiMax);

    for (unsigned int ieta = 0; ieta < nEtaBins; ++ieta) {
      // Prepare the eta ragne for this batch
      float etaMin = etaRange[0] + ieta * etaStep;
      float etaMax = etaRange[0] + (ieta + 1) * etaStep;
      etaVals[ieta + 1] = etaMax;
      // Acceptance range
      AcceptRange etaBin{tracks.t_eta, {etaMin, etaMax}};
      // Name tag
      TString etaTag = "_eta";
      etaTag += ieta;
      // Range cut string
      std::string etaCut = "t_eta >= ";
      etaCut += std::to_string(etaMin);
      etaCut += land;
      etaCut += std::string("t_eta < ");
      etaCut += std::to_string(etaMax);

      // Combined eta/phi acceptance
      AcceptCombination etaPhiBin{etaBin, phiBin};
      TString etaPhiTag = etaTag + phiTag;

      for (unsigned int ipt = 0; ipt < nPtBins; ++ipt) {
        // Acceptance range for this pT bin
        float ptMin = static_cast<float>(ptBorders[ipt]);
        float ptMax = static_cast<float>(ptBorders[ipt + 1]);
        AcceptRange ptBin{tracks.t_pT, {ptMin, ptMax}};

        float upperPtBorder =
            ptBorders[ipt + 1] == std::numeric_limits<double>::infinity()
                ? 100.
                : ptBorders[ipt + 1];
        ptVals[ipt + 1] = upperPtBorder;
        // Name tag
        TString ptTag = "_pt";
        ptTag += ipt;

        // Range cut string
        std::string ptCut = "t_pT >= ";
        ptCut += std::to_string(ptMin);
        ptCut += land;
        ptCut += std::string("t_pT < ");
        ptCut += std::to_string(ptMax);

        // Combined eta/pt acceptance
        AcceptCombination etaPtBin{etaBin, ptBin};

        for (unsigned int iresp = 0; iresp < baseResidualPulls.size();
             ++iresp) {
          // Eta-Pt handles -- restrict for iphi == 0
          if (iphi == 0) {
            auto& etaPtHandle = etaPtResidualPulls[ieta][ipt][iresp];
            // Create the handle tag
            TString handleTag(etaPtHandle.tag + etaTag + ptTag);
            // Accept range and range cut
            etaPtHandle.accept = etaPtBin;
            etaPtHandle.rangeCutStr = ptCut + land + etaCut;
            handleRange(etaPtHandle, handleTag, peakEntries);
            bookHistograms(etaPtHandle, pullRange, nHistBins, ++histBarcode);

            // Set name and style/
            TString residualN = TString("res_") + handleTag;
            TString pullN = TString("pull_") + handleTag;
            // Range histogram does not exist when read in from configuration
            if (etaPtHandle.rangeHist != nullptr) {
              TString rangeN = TString("range_") + handleTag;
              setHistStyle(etaPtHandle.rangeHist);
              etaPtHandle.rangeHist->SetName(rangeN);
            }
            setHistStyle(etaPtHandle.residualHist);
            etaPtHandle.residualHist->SetName(residualN);
            setHistStyle(etaPtHandle.pullHist);
            etaPtHandle.pullHist->SetName(pullN);
          }
          // Eta-Phi handles --- restrice for ipt == 0
          if (ipt == 0) {
            auto& etaPhiHandle = etaPhiResidualPulls[ieta][iphi][iresp];
            // Create the handle tag
            TString handleTag(etaPhiHandle.tag + etaTag + phiTag);
            etaPhiHandle.accept = etaPhiBin;
            handleRange(etaPhiHandle, handleTag, peakEntries);
            bookHistograms(etaPhiHandle, pullRange, nHistBins, ++histBarcode);

            // Set name and style
            TString residualN = TString("res_") + handleTag;
            TString pullN = TString("pull_") + handleTag;

            // Range histogram does not exist when read in from configuration
            if (etaPhiHandle.rangeHist != nullptr) {
              TString rangeN = TString("range_") + handleTag;
              setHistStyle(etaPhiHandle.rangeHist);
              etaPhiHandle.rangeHist->SetName(rangeN);
            }
            setHistStyle(etaPhiHandle.residualHist);
            etaPhiHandle.residualHist->SetName(residualN);
            setHistStyle(etaPhiHandle.pullHist);
            etaPhiHandle.pullHist->SetName(pullN);
          }

#ifdef BOOST_AVAILABLE
          ++handle_preparation_progress;
#endif
        }

        // Auxiliary plots
        for (unsigned int iaux = 0; iaux < baseAuxilaries.size(); ++iaux) {
          // Eta-Pt handles - restrict to iphi == 0
          if (iphi == 0) {
            auto& etaPtAux = etaPtAuxiliaries[ieta][ipt][iaux];
            etaPtAux.accept = etaPtBin;
            TString handleTag(etaPtAux.tag + etaTag + ptTag);
            etaPtAux.hist =
                new TH1F(handleTag.Data(), etaPtAux.tag.c_str(), etaPtAux.bins,
                         etaPtAux.range[0], etaPtAux.range[1]);
            etaPtAux.hist->GetXaxis()->SetTitle(etaPtAux.label.c_str());
            etaPtAux.hist->GetYaxis()->SetTitle("Entries");
            setHistStyle(etaPtAux.hist);
          }

          // Eta-Phi handles - restrict to ipt == 0
          if (ipt == 0) {
            auto& etaPhiAux = etaPhiAuxiliaries[ieta][iphi][iaux];
            etaPhiAux.accept = etaPhiBin;
            TString handleTag(etaPhiAux.tag + etaTag + phiTag);
            etaPhiAux.hist = new TH1F(handleTag.Data(), etaPhiAux.tag.c_str(),
                                      etaPhiAux.bins, etaPhiAux.range[0],
                                      etaPhiAux.range[1]);
            etaPhiAux.hist->GetXaxis()->SetTitle(etaPhiAux.label.c_str());
            etaPhiAux.hist->GetYaxis()->SetTitle("Entries");
            setHistStyle(etaPhiAux.hist);
          }
        }
      }
    }
  }

#ifdef NLOHMANN_AVAILABLE
  if (! outConfig.empty()) {
    std::ofstream config_out;
    config_out.open(outConfig.c_str());
    config_out << handle_configs.dump(4);
  }
#endif

#ifdef BOOST_AVAILABLE
  std::cout << "*** Event Loop: " << std::endl;
  progress_display event_loop_progress(entries);
#endif

  for (unsigned long ie = 0; ie < entries; ++ie) {
#ifdef BOOST_AVAILABLE
    ++event_loop_progress;
#endif

    // Make sure you have the entry
    tracks.tree->GetEntry(ie);
    std::size_t nTracks = tracks.hasFittedParams->size();
    for (std::size_t it = 0; it < nTracks; ++it) {
      if (tracks.hasFittedParams->at(it)) {
        // Residual handlesL
        // Full range handles
        for (auto& fHandle : fullResidualPulls) {
          fHandle.fill(it);
        }

        // Eta-Pt handles
        for (auto& etaBatch : etaPtResidualPulls) {
          for (auto& ptBatch : etaBatch) {
            for (auto& bHandle : ptBatch) {
              bHandle.fill(it);
            }
          }
        }

        // Eta-Phi handles
        for (auto& etaBatch : etaPhiResidualPulls) {
          for (auto& phiBatch : etaBatch) {
            for (auto& bHandle : phiBatch) {
              bHandle.fill(it);
            }
          }
        }
      }

      // Auxiliary handles:
      // Full range handles
      for (auto& fAuxiliary : fullAuxiliaries) {
        fAuxiliary.fill(it);
      }

      // Eta-Pt handles
      for (auto& etaBatch : etaPtAuxiliaries) {
        for (auto& ptBatch : etaBatch) {
          for (auto& bHandle : ptBatch) {
            bHandle.fill(it);
          }
        }
      }

      // Eta-Phi handles
      for (auto& etaBatch : etaPhiAuxiliaries) {
        for (auto& phiBatch : etaBatch) {
          for (auto& bHandle : phiBatch) {
            bHandle.fill(it);
          }
        }
      }
    }
  }

  // The output file section
  auto output = TFile::Open(outFile.c_str(), "recreate");
  output->cd();

  // Full range handles : residual and pulls
  for (auto& fHandle : fullResidualPulls) {
    if (fHandle.rangeHist != nullptr) {
      fHandle.rangeHist->Write();
    }
    fHandle.residualHist->Write();
    fHandle.pullHist->Write();
  }

  // Full range handles : auxiliaries
  for (auto& fAuxiliary : fullAuxiliaries) {
    fAuxiliary.hist->SetName(fAuxiliary.hist->GetName() + TString("_all"));
    fAuxiliary.hist->Write();
  }

  struct SummaryHistograms {
    TH2F* fillStats = nullptr;

    std::vector<TH2F*> residualRMS;
    std::vector<TH2F*> residualMean;
    std::vector<TH2F*> pullSigma;
    std::vector<TH2F*> pullMean;

    std::vector<TH2F*> auxiliaries;
  };

  /// Helper method to analyse bins
  /// @param residualPullsMatrix the 2D matrix of handles
  /// @param auxiliaryMatrix the 2D matrix of the auxiliary handles
  /// @param matrixTag the identification tag for the matrix
  /// @param outputBorders the border vector for the outer bins
  /// @param innerBorders the border vector for the inner bins
  /// @param fXTitle the title of the x axis of the first projection
  /// @param sXTitle the title of the x axis of the second projection
  ///
  /// @note this is a void function
  auto analyseBins = [&](ResidualPullsMatrix& residualPullsMatrix,
                         AuxiliariesMatrix& auxiliaryMatrix,
                         const TString& matrixTag, const TVectorF& outerBorders,
                         const TVectorF& innerBorders,
                         const TString& fXTitle = "#eta",
                         const TString& sXTitle = "#phi") -> void {
    // The summary histogram set
    SummaryHistograms summary;

    // 2D handles ---------------------------
    unsigned int nOuterBins = outerBorders.GetNrows() - 1;
    auto outerValues = outerBorders.GetMatrixArray();
    unsigned int nInnerBins = innerBorders.GetNrows() - 1;
    auto innerValues = innerBorders.GetMatrixArray();

    TString statN = TString("entries") + matrixTag;
    summary.fillStats =
        new TH2F(statN, "", nOuterBins, outerValues, nInnerBins, innerValues);

#ifdef BOOST_AVAILABLE
    progress_display analysis_progress(nOuterBins * nInnerBins);
#endif

    // Prepare by looping over the base bHandles - residuals
    for (auto& bHandle : baseResidualPulls) {
      // Create a unique handle tag
      TString handleTag = TString(bHandle.tag) + matrixTag;
      // ... and names
      TString residualRMSN = TString("res_rms_") + handleTag;
      TString residualMeanN = TString("res_mean_") + handleTag;
      TString pullSigmaN = TString("pull_sigma_") + handleTag;
      TString pullMeanN = TString("pull_mean_") + handleTag;

      TH2F* residualRMS = new TH2F(residualRMSN, "", nOuterBins, outerValues,
                                   nInnerBins, innerValues);
      TH2F* residualMean = new TH2F(residualMeanN, "", nOuterBins, outerValues,
                                    nInnerBins, innerValues);
      TH2F* pullSigma = new TH2F(pullSigmaN, "", nOuterBins, outerValues,
                                 nInnerBins, innerValues);
      TH2F* pullMean = new TH2F(pullMeanN, "", nOuterBins, outerValues,
                                nInnerBins, innerValues);
      // Booked & ready
      summary.residualRMS.push_back(residualRMS);
      summary.residualMean.push_back(residualMean);
      summary.pullSigma.push_back(pullSigma);
      summary.pullMean.push_back(pullMean);
    }

    // Prepare by looping over the base handles - auxiliaries
    for (auto& aHandle : baseAuxilaries) {
      // Create a unique handle tag
      TString auxiliaryTag = TString(aHandle.tag) + matrixTag;
      TH2F* auxHist = new TH2F(auxiliaryTag, auxiliaryTag, nOuterBins,
                                 outerValues, nInnerBins, innerValues);
      summary.auxiliaries.push_back(auxHist);
    }

    unsigned int io = 0;
    for (auto& outerBatch : residualPullsMatrix) {
      unsigned int ii = 0;
      for (auto& innerBatch : outerBatch) {
        // residual/pull loop
        unsigned int iresp = 0;
        for (auto& bHandle : innerBatch) {
          // Range estimates / could be empty if read from configuration
          if (bHandle.rangeHist != nullptr) {
            bHandle.rangeHist->Write();
          }
          // Fill the stats
          if (iresp == 0) {
            summary.fillStats->SetBinContent(io + 1, ii + 1, bHandle.accepted);
          }

          // Residuals
          // Get RMS/ RMSError
          float rrms = bHandle.residualHist->GetRMS();
          float rrerr = bHandle.residualHist->GetRMSError();
          float rmean = bHandle.residualHist->GetMean();
          float rmerr = bHandle.residualHist->GetMeanError();
          summary.residualRMS[iresp]->SetBinContent(io + 1, ii + 1, rrms);
          summary.residualRMS[iresp]->SetBinError(io + 1, ii + 1, rrerr);
          summary.residualMean[iresp]->SetBinContent(io + 1, ii + 1, rmean);
          summary.residualMean[iresp]->SetBinError(io + 1, ii + 1, rmerr);
          bHandle.residualHist->Write();
          // Pulls
          bHandle.pullHist->Fit("gaus", "q");
          TF1* gauss = bHandle.pullHist->GetFunction("gaus");
          if (gauss != nullptr) {
            float pmu = gauss->GetParameter(1);
            float pmerr = gauss->GetParError(1);
            float psigma = gauss->GetParameter(2);
            float pserr = gauss->GetParError(2);
            summary.pullSigma[iresp]->SetBinContent(io + 1, ii + 1, psigma);
            summary.pullSigma[iresp]->SetBinError(io + 1, ii + 1, pserr);
            summary.pullMean[iresp]->SetBinContent(io + 1, ii + 1, pmu);
            summary.pullMean[iresp]->SetBinError(io + 1, ii + 1, pmerr);
          }
          bHandle.pullHist->Write();
          ++iresp;
        }

        // auxilaiary loop
        auto auxiliaryBatch = auxiliaryMatrix[io][ii];
        unsigned int iaux = 0;
        for (auto& aHandle : auxiliaryBatch) {
          float value = aHandle.hist->GetMean();
          float error = aHandle.hist->GetMeanError();
          summary.auxiliaries[iaux]->SetBinContent(io + 1, ii + 1, value);
          summary.auxiliaries[iaux]->SetBinError(io + 1, ii + 1, error);
          aHandle.hist->Write();

          ++iaux;
        }
#ifdef BOOST_AVAILABLE
        ++analysis_progress;
#endif
        ++ii;
      }
      ++io;
    }

    /// Write out the projection histogram
    ///
    /// @param h2 the 2D histogram as source for the projects
    /// @param fXTitle the title of the x axis of the first projection
    /// @param fYTitle the title of the y axis of the first projection
    /// @param sXTitle the title of the x axis of the second projection
    /// @param sYTitle the title of the y axis of the second projection
    auto writeProjections = [](const TH2F& h2, const TString& fXTitleP = "#eta",
                               const TString& fYTitleP = "sigma",
                               const TString& sXTitleP = "#phi",
                               const TString& sYTitleP = "sigma") -> void {
      const TString& fTag = "_pX";
      const TString& sTag = "_pY";

      unsigned int nBinsX = h2.GetXaxis()->GetNbins();
      unsigned int nBinsY = h2.GetYaxis()->GetNbins();
      if (fTag != "") {
        TH1D* pX =
            dynamic_cast<TH1D*>(h2.ProjectionX((h2.GetName() + fTag).Data()));
        setHistStyle(pX);
        if (pX != nullptr) {
          pX->GetXaxis()->SetTitle(fXTitleP.Data());
          pX->GetYaxis()->SetTitle(fYTitleP.Data());
          pX->Write();
        }
        // Bin-wise projections
        for (unsigned int iy = 1; iy <= nBinsY; ++iy) {
          pX = dynamic_cast<TH1D*>(h2.ProjectionX(
              (h2.GetName() + fTag + sTag + (iy - 1)).Data(), iy, iy));
          setHistStyle(pX);
          if (pX != nullptr) {
            pX->GetXaxis()->SetTitle(fXTitleP.Data());
            pX->GetYaxis()->SetTitle(fYTitleP.Data());
            pX->Write();
          }
        }
      }
      if (sTag != "") {
        TH1D* pY =
            dynamic_cast<TH1D*>(h2.ProjectionY((h2.GetName() + sTag).Data()));
        setHistStyle(pY);
        if (pY != nullptr) {
          pY->GetXaxis()->SetTitle(sXTitleP.Data());
          pY->GetYaxis()->SetTitle(sYTitleP.Data());
          pY->Write();
        }
        // Bin-wise projections
        for (unsigned int ix = 1; ix <= nBinsX; ++ix) {
          pY = dynamic_cast<TH1D*>(h2.ProjectionY(
              (h2.GetName() + sTag + fTag + (ix - 1)).Data(), ix, ix));
          setHistStyle(pY);
          if (pY != nullptr) {
            pY->GetXaxis()->SetTitle(sXTitleP.Data());
            pY->GetYaxis()->SetTitle(sYTitleP.Data());
            pY->Write();
          }
        }
      }
    };

    setHistStyle(summary.fillStats);
    summary.fillStats->Write();

    // Write mapped residual/pull histograms and their projections
    for (unsigned int iresp = 0; iresp < baseResidualPulls.size(); ++iresp) {
      // Get the handle for writing out
      auto bHandle = baseResidualPulls[iresp];

      TString rrms = TString("RMS[") + bHandle.residualStr + TString("] ") +
                     bHandle.residualUnit;
      TString rmu = TString("#mu[") + bHandle.residualStr + TString("] ") +
                    bHandle.residualUnit;

      TString psigma = TString("#sigma[(") + bHandle.residualStr +
                       TString(")/") + bHandle.errorStr + TString("]");
      TString pmu = TString("#mu[(") + bHandle.residualStr + TString(")/") +
                    bHandle.errorStr + TString("]");

      // 2D map histograms
      setHistStyle(summary.residualRMS[iresp]);
      summary.residualRMS[iresp]->GetXaxis()->SetTitle(fXTitle);
      summary.residualRMS[iresp]->GetYaxis()->SetTitle(sXTitle);
      summary.residualRMS[iresp]->GetZaxis()->SetTitle(rrms);
      summary.residualRMS[iresp]->Write();

      setHistStyle(summary.residualMean[iresp]);
      summary.residualMean[iresp]->GetXaxis()->SetTitle(fXTitle);
      summary.residualMean[iresp]->GetYaxis()->SetTitle(sXTitle);
      summary.residualMean[iresp]->GetZaxis()->SetTitle(rmu);
      summary.residualMean[iresp]->Write();

      setHistStyle(summary.pullSigma[iresp]);
      adaptColorPalette(summary.pullSigma[iresp], 0., 4., 1., 0.1, 104);
      summary.pullSigma[iresp]->GetXaxis()->SetTitle(fXTitle);
      summary.pullSigma[iresp]->GetYaxis()->SetTitle(sXTitle);
      summary.pullSigma[iresp]->GetZaxis()->SetRangeUser(0., 4.);
      summary.pullSigma[iresp]->GetZaxis()->SetTitle(psigma);
      summary.pullSigma[iresp]->Write();

      setHistStyle(summary.pullMean[iresp]);
      adaptColorPalette(summary.pullMean[iresp], -1., 1., 0., 0.1, 104);
      summary.pullMean[iresp]->GetXaxis()->SetTitle(fXTitle);
      summary.pullMean[iresp]->GetYaxis()->SetTitle(sXTitle);
      summary.pullMean[iresp]->GetZaxis()->SetRangeUser(-1., 1.);
      summary.pullMean[iresp]->GetZaxis()->SetTitle(pmu);
      summary.pullMean[iresp]->Write();

      // Write the projection histograms
      writeProjections(*summary.residualRMS[iresp], fXTitle, rrms, sXTitle,
                       rrms);
      writeProjections(*summary.residualMean[iresp], fXTitle, rmu, sXTitle,
                       rmu);
      writeProjections(*summary.pullSigma[iresp], fXTitle, psigma, sXTitle,
                       psigma);
      writeProjections(*summary.pullMean[iresp], fXTitle, pmu, sXTitle, pmu);
    }

    // Write mapped auxiliary histograms and their projections
    for (unsigned int iaux = 0; iaux < baseAuxilaries.size(); ++iaux) {
      setHistStyle(summary.auxiliaries[iaux]);
      summary.auxiliaries[iaux]->GetXaxis()->SetTitle(fXTitle);
      summary.auxiliaries[iaux]->GetYaxis()->SetTitle(sXTitle);
      summary.auxiliaries[iaux]->GetZaxis()->SetTitle(
          baseAuxilaries[iaux].label.c_str());
      summary.auxiliaries[iaux]->Write();

      writeProjections(*summary.auxiliaries[iaux], fXTitle,
                       baseAuxilaries[iaux].label, sXTitle,
                       baseAuxilaries[iaux].label);
    }

    return;
  };

// The handle matrices
#ifdef BOOST_AVAILABLE
  std::cout << "*** Bin/Projection Analysis: " << std::endl;
#endif
  analyseBins(etaPtResidualPulls, etaPtAuxiliaries, TString("_eta_pt"), etaVals,
              ptVals, "#eta", "p_{T} [GeV]");
  analyseBins(etaPhiResidualPulls, etaPhiAuxiliaries, TString("_eta_phi"),
              etaVals, phiVals, "#eta", "#phi");

  output->Close();

  return 1;
}
