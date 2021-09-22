// This file is part of the Acts project.
//
// Copyright (C) 2019-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

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
#include <TLegend.h>
#include <TMath.h>
#include <TStyle.h>
#include <TTree.h>
#include <TVectorF.h>

#include "CommonUtils.h"
#include "TreeReader.h"

using namespace ROOT;

void setHistStyle(TH1F* hist, short color);

/// This ROOT script will plot the residual and pull of perigee track parameters
/// (d0, z0, phi, theta, q/p, pT, t) from root file produced by the
/// TrackFitterPerformanceWriter
///
/// @param inFiles the list of input files
/// @param inTree the name of the input tree
/// @param outFile the name of the output file
/// @param inConfig the (optional) input configuration JSON file
/// @param outConfig the (optional) output configuration JSON file
/// @param parametersOn the bitset of the parameters set
int perigeeParamResolution(
    const std::vector<std::string>& inFiles, const std::string& inTree,
    const std::string& outFile, const std::string& inConfig = "",
    const std::string& outConfig = "", unsigned long nEntries = 0,
    unsigned int nPeakEntries = 0, float pullRange = 6.,
    unsigned int nHistBins = 61, unsigned int nPhiBins = 10,
    const std::array<float, 2>& phiRange = {-M_PI, M_PI},
    unsigned int nEtaBins = 10, const std::array<float, 2>& etaRange = {-3, 3},
    const std::vector<double>& ptBorders =
        {0., std::numeric_limits<double>::infinity()},
    std::bitset<7> parametersOn = std::bitset<7>{"1111111"}) {
  // Load the tree chain
  TChain* treeChain = new TChain(inTree.c_str());
  for (const auto& inFile : inFiles) {
    treeChain->Add(inFile.c_str());
    // Open root file written by RootTrajectoryWriter
    std::cout << "*** Adding file: " << inFile << std::endl;
  }

  if (treeChain->GetEntries() == 0) {
    std::cout << "[x] No entries found ... " << std::endl;
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

  // One time initialization of the parameter handles
  using Handles = std::vector<ResidualPullHandle>;
  Handles baseHandles = {};

  if (parametersOn.test(0)) {
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
    baseHandles.push_back(d0Handle);
  }

  if (parametersOn.test(1)) {
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
    baseHandles.push_back(z0Handle);
  }

  if (parametersOn.test(2)) {
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
    baseHandles.push_back(phi0Handle);
  }

  if (parametersOn.test(3)) {
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
    baseHandles.push_back(theta0Handle);
  }

  if (parametersOn.test(4)) {
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
    baseHandles.push_back(qopHandle);
  }

  if (parametersOn.test(5)) {
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
    baseHandles.push_back(ptHandle);
  }

  // Preparation phase for handles
#ifdef NLOHMANN_AVAILABLE
  nlohmann::json handle_configs;
  if (not inConfig.empty()) {
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
  /// @param peakEntries the number of entries used for range peaking
  auto handleRange = [&](ResidualPullHandle& handle, const TString& handleTag,
                         unsigned long peakEntries) -> void {
    bool rangeDetermined = false;
    if (not inConfig.empty()) {
      if (handle_configs.contains((handleTag).Data())) {
        auto handle_config = handle_configs[(handleTag).Data()];
        handle.range = handle_config["range"].get<std::array<float, 2>>();
        rangeDetermined = true;
      }
    }
    if (not rangeDetermined) {
      estimateResiudalRange(handle, *rangeCanvas, *tracks.tree, peakEntries,
                    ++histBarcode);
    }

    if (not outConfig.empty()) {
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
  Handles fullRangeHandles = baseHandles;
  // Range Estimation and booking histogram
  for (auto& fHandle : fullRangeHandles) {
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
  using HandleVector = std::vector<Handles>;
  using HandleMatrix = std::vector<HandleVector>;

  // Eta-Pt handles
  HandleVector ptHandles = HandleVector(nPtBins, baseHandles);
  HandleMatrix etaPtHandles = HandleMatrix(nEtaBins, ptHandles);

  // Eta-Phi handles
  HandleVector phiHandles = HandleVector(nPhiBins, baseHandles);
  HandleMatrix etaPhiHandles = HandleMatrix(nEtaBins, phiHandles);

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
  boost::progress_display handle_preparation_progress(
      nPhiBins * nEtaBins * nPtBins * baseHandles.size());
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

        for (unsigned int ipar = 0; ipar < baseHandles.size(); ++ipar) {
          // Eta-Pt handles -- restrict for iphi == 0
          if (iphi == 0) {
            auto& etaPtHandle = etaPtHandles[ieta][ipt][ipar];
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
            auto& etaPhiHandle = etaPhiHandles[ieta][iphi][ipar];
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
      }
    }
  }

#ifdef NLOHMANN_AVAILABLE
  if (not outConfig.empty()) {
    std::ofstream config_out;
    config_out.open(outConfig.c_str());
    config_out << handle_configs.dump(4);
  }
#endif

#ifdef BOOST_AVAILABLE
  std::cout << "*** Event Loop: " << std::endl;
  boost::progress_display event_loop_progress(entries);
#endif

  for (unsigned long ie = 0; ie < entries; ++ie) {
#ifdef BOOST_AVAILABLE
    ++event_loop_progress;
#endif

    // Make sure you have the entry
    tracks.tree->GetEntry(ie);
    size_t nTracks = tracks.hasFittedParams->size();
    for (size_t it = 0; it < nTracks; ++it) {
      if (tracks.hasFittedParams->at(it)) {
        // Full range handles
        for (auto& fHandle : fullRangeHandles) {
          fHandle.fill(it);
        }

        // Eta-Pt handles
        for (auto& etaBatch : etaPtHandles) {
          for (auto& ptBatch : etaBatch) {
            for (auto& bHandle : ptBatch) {
              bHandle.fill(it);
            }
          }
        }

        // Eta-Phi handles
        for (auto& etaBatch : etaPhiHandles) {
          for (auto& phiBatch : etaBatch) {
            for (auto& bHandle : phiBatch) {
              bHandle.fill(it);
            }
          }
        }
      }
    }
  }

  // The output file section
  auto output = TFile::Open(outFile.c_str(), "recreate");
  output->cd();

  // Full range handles
  for (auto& fHandle : fullRangeHandles) {
    if (fHandle.rangeHist != nullptr) {
      fHandle.rangeHist->Write();
    }
    fHandle.residualHist->Write();
    fHandle.pullHist->Write();
  }

  struct SummaryHistograms {
    std::vector<TH2F*> residualRMS;
    std::vector<TH2F*> residualMean;
    std::vector<TH2F*> pullSigma;
    std::vector<TH2F*> pullMean;
  };

  /// Helper method to analyse bins
  /// @param handleMatrix the 2D matrix of handles
  /// @param matrixTag the identification tag for the matrix
  /// @param outputBordres the border vector for the outer bins
  /// @param innerBorders the border vector for the inner bins
  /// @param fXTitle the title of the x axis of the first projection
  /// @param sXTitle the title of the x axis of the second projection
  ///
  /// @note this is a void function
  auto analyseBins = [&](HandleMatrix& handleMatrix, const TString& matrixTag,
                         const TVectorF& outerBorders,
                         const TVectorF& innerBorders,
                         const TString& fXTitle = "#eta",
                         const TString& sXTitle = "#phi") -> void {
    // The summary histrogram set
    SummaryHistograms summary;

    // 2D handles ---------------------------
    unsigned int nOuterBins = outerBorders.GetNrows() - 1;
    auto outerValues = outerBorders.GetMatrixArray();
    unsigned int nInnerBins = innerBorders.GetNrows() - 1;
    auto innerValuse = innerBorders.GetMatrixArray();

#ifdef BOOST_AVAILABLE
    boost::progress_display analysis_progress(nOuterBins * nInnerBins *
                                              baseHandles.size());
#endif

    // Prepare by looping over the base bhandles
    for (auto& bHandle : baseHandles) {
      // Create a unique handle tag
      TString handleTag = TString(bHandle.tag) + matrixTag;
      // ... and names
      TString residualRMSN = TString("res_rms_") + handleTag;
      TString residualMeanN = TString("res_mean_") + handleTag;
      TString pullSigmaN = TString("pull_sigma_") + handleTag;
      TString pullMeanN = TString("pull_mean_") + handleTag;

      TH2F* residualRMS = new TH2F(residualRMSN, "", nOuterBins, outerValues,
                                   nInnerBins, innerValuse);
      TH2F* residualMean = new TH2F(residualMeanN, "", nOuterBins, outerValues,
                                    nInnerBins, innerValuse);
      TH2F* pullSigma = new TH2F(pullSigmaN, "", nOuterBins, outerValues,
                                 nInnerBins, innerValuse);
      TH2F* pullMean = new TH2F(pullMeanN, "", nOuterBins, outerValues,
                                nInnerBins, innerValuse);
      // Booked & ready
      summary.residualRMS.push_back(residualRMS);
      summary.residualMean.push_back(residualMean);
      summary.pullSigma.push_back(pullSigma);
      summary.pullMean.push_back(pullMean);
    }

    unsigned int io = 0;
    for (auto& outerBatch : handleMatrix) {
      unsigned int ii = 0;
      for (auto& innerBatch : outerBatch) {
        unsigned int ipar = 0;
        for (auto& bHandle : innerBatch) {
          // Range estimates / could be empty if read from configuration
          if (bHandle.rangeHist != nullptr) {
            bHandle.rangeHist->Write();
          }
          // Residuals
          // Get RMS/ RMSError
          float rrms = bHandle.residualHist->GetRMS();
          float rrerr = bHandle.residualHist->GetRMSError();
          float rmean = bHandle.residualHist->GetMean();
          float rmerr = bHandle.residualHist->GetMeanError();
          summary.residualRMS[ipar]->SetBinContent(io + 1, ii + 1, rrms);
          summary.residualRMS[ipar]->SetBinError(io + 1, ii + 1, rrerr);
          summary.residualMean[ipar]->SetBinContent(io + 1, ii + 1, rmean);
          summary.residualMean[ipar]->SetBinError(io + 1, ii + 1, rmerr);
          bHandle.residualHist->Write();
          // Pulls
          bHandle.pullHist->Fit("gaus", "q");
          TF1* gauss = bHandle.pullHist->GetFunction("gaus");
          float pmu = gauss->GetParameter(1);
          float pmerr = gauss->GetParError(1);
          float psigma = gauss->GetParameter(2);
          float pserr = gauss->GetParError(2);
          summary.pullSigma[ipar]->SetBinContent(io + 1, ii + 1, psigma);
          summary.pullSigma[ipar]->SetBinError(io + 1, ii + 1, pserr);
          summary.pullMean[ipar]->SetBinContent(io + 1, ii + 1, pmu);
          summary.pullMean[ipar]->SetBinError(io + 1, ii + 1, pmerr);
          bHandle.pullHist->Write();
          ++ipar;
#ifdef BOOST_AVAILABLE
          ++analysis_progress;
#endif
        }
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
    auto writeProjections = [](const TH2F& h2, const TString& fXTitle = "#eta",
                               const TString& fYTitle = "sigma",
                               const TString& sXTitle = "#phi",
                               const TString& sYTitle = "sigma") -> void {
      const TString& fTag = "_pX";
      const TString& sTag = "_pY";

      unsigned int nBinsX = h2.GetXaxis()->GetNbins();
      unsigned int nBinsY = h2.GetYaxis()->GetNbins();
      if (fTag != "") {
        TH1D* pX =
            dynamic_cast<TH1D*>(h2.ProjectionX((h2.GetName() + fTag).Data()));
        if (pX != nullptr) {
          pX->GetXaxis()->SetTitle(fXTitle.Data());
          pX->GetYaxis()->SetTitle(fYTitle.Data());
          pX->Write();
        }
        // Bin-wise projections
        for (unsigned int iy = 1; iy <= nBinsY; ++iy) {
          pX = dynamic_cast<TH1D*>(h2.ProjectionX(
              (h2.GetName() + fTag + sTag + (iy - 1)).Data(), iy, iy));
          if (pX != nullptr) {
            pX->GetXaxis()->SetTitle(fXTitle.Data());
            pX->GetYaxis()->SetTitle(fYTitle.Data());
            pX->Write();
          }
        }
      }
      if (sTag != "") {
        TH1D* pY =
            dynamic_cast<TH1D*>(h2.ProjectionY((h2.GetName() + sTag).Data()));
        if (pY != nullptr) {
          pY->GetXaxis()->SetTitle(sXTitle.Data());
          pY->GetYaxis()->SetTitle(sYTitle.Data());
          pY->Write();
        }
        // Bin-wise projections
        for (unsigned int ix = 1; ix <= nBinsX; ++ix) {
          pY = dynamic_cast<TH1D*>(h2.ProjectionY(
              (h2.GetName() + sTag + fTag + (ix - 1)).Data(), ix, ix));
          if (pY != nullptr) {
            pY->GetXaxis()->SetTitle(sXTitle.Data());
            pY->GetYaxis()->SetTitle(sYTitle.Data());
            pY->Write();
          }
        }
      }
    };

    // Write
    for (unsigned int ipar = 0; ipar < baseHandles.size(); ++ipar) {
      // Get the handle for writing out
      auto bHandle = baseHandles[ipar];

      TString rrms = TString("RMS[") + bHandle.residualStr + TString("] ") +
                     bHandle.residualUnit;
      TString rmu = TString("#mu[") + bHandle.residualStr + TString("] ") +
                    bHandle.residualUnit;

      TString psigma = TString("#sigma[(") + bHandle.residualStr +
                       TString(")/") + bHandle.errorStr + TString("]");
      TString pmu = TString("#mu[(") + bHandle.residualStr + TString(")/") +
                    bHandle.errorStr + TString("]");

      // 2D map histograms
      summary.residualRMS[ipar]->GetXaxis()->SetTitle(fXTitle);
      summary.residualRMS[ipar]->GetYaxis()->SetTitle(sXTitle);
      summary.residualRMS[ipar]->GetZaxis()->SetTitle(rrms);
      summary.residualRMS[ipar]->Write();
      summary.residualMean[ipar]->GetXaxis()->SetTitle(fXTitle);
      summary.residualMean[ipar]->GetYaxis()->SetTitle(sXTitle);
      summary.residualMean[ipar]->GetZaxis()->SetTitle(rmu);
      summary.residualMean[ipar]->Write();
      summary.pullSigma[ipar]->GetXaxis()->SetTitle(fXTitle);
      summary.pullSigma[ipar]->GetYaxis()->SetTitle(sXTitle);
      summary.pullSigma[ipar]->GetZaxis()->SetTitle(psigma);
      summary.pullSigma[ipar]->Write();
      summary.pullMean[ipar]->GetXaxis()->SetTitle(fXTitle);
      summary.pullMean[ipar]->GetYaxis()->SetTitle(sXTitle);
      summary.pullMean[ipar]->GetZaxis()->SetTitle(pmu);
      summary.pullMean[ipar]->Write();

      // Write the projection histograms
      writeProjections(*summary.residualRMS[ipar], fXTitle, rrms, sXTitle,
                       rrms);
      writeProjections(*summary.residualMean[ipar], fXTitle, rmu, sXTitle, rmu);
      writeProjections(*summary.pullSigma[ipar], fXTitle, psigma, sXTitle,
                       psigma);
      writeProjections(*summary.pullMean[ipar], fXTitle, pmu, sXTitle, pmu);
    }
    return;
  };

// The handle matrices
#ifdef BOOST_AVAILABLE
  std::cout << "*** Bin/Projection Analysis: " << std::endl;
#endif
  analyseBins(etaPtHandles, TString("_eta_pt"), etaVals, ptVals, "#eta",
              "p_{T} [GeV]");
  analyseBins(etaPhiHandles, TString("_eta_phi"), etaVals, phiVals, "#eta",
              "#phi");

  output->Close();

  return 1;
}
