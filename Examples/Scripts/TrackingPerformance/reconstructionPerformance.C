// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <string>
#include <vector>
#include <iostream>

#include <TCanvas.h>
#include <TEfficiency.h>
#include <TProfile.h>
#include <TFile.h>

#include "CommonUtils.h"

/// This script allows a fast reading and replotting of the existing performance plots, e.g. 'trackeff_vs_*' and 'nMeasurements_vs_*',
/// from the root file 'performance_track_fitter.root' or 'performance_ckf.root'.
/// Note that redefinition of the tracking efficiency etc. is not possible with this script.
/// If you want to define your own efficiency etc., please refer to 'defineReconstructionPerformance.C'.
///
void reconstructionPerformance(std::vector<std::string> inputFileNames) {
  std::array<TCanvas*, 3> emho = {nullptr, nullptr, nullptr};
  std::vector<std::string> tags = {"eta", "pT"};
  std::vector<std::string> params = {"trackeff", "nMeasurements", "nHoles",
                                    "nOutliers", "nStates"};

  // Create the Canvae
  unsigned int itag = 0;
  for (const auto& t : tags) {
    std::string name = "efd_" + t;
    std::string label = "Efficiency, Fakes, Duplicate vs. " + t;
    emho[itag] =
        new TCanvas(name.c_str(), label.c_str(), itag * 10, 10, 1800, 300);
    emho[itag]->Divide(5, 1);
    ++itag;
  }

  unsigned int imode = 1;
  for (auto fileName : inputFileNames) {
    // The appropriate file
    auto file = TFile::Open(fileName.c_str(), "read");
    unsigned int itag = 0;
    for (const auto& t : tags) {
      unsigned int ipar = 0;
      for (const auto& p : params) {
        std::string hName = p + std::string("_vs_") + t;
        emho[itag]->cd(ipar+1);
        auto h = file->Get(hName.c_str());
        auto heff = dynamic_cast<TEfficiency*>(h);
        if (heff != nullptr){
          setEffStyle(heff, imode);
        } else {
          auto hprof = dynamic_cast<TProfile*>(h);
          if (hprof != nullptr){
            setHistStyle(hprof, imode);
          }
        }
        std::string dOpt = imode == 1 ? "" : "same";
        h->Draw(dOpt.c_str());
        ++ipar;
      }
      ++itag;
    }
    ++imode;
  }
}

/**
TFile**		performance_track_fitter.root
 TFile*		performance_track_fitter.root
  KEY: TH1F	res_d0;1	Residual of d0
  KEY: TH2F	res_d0_vs_eta;1	Residual of d0 vs eta
  KEY: TH1F	resmean_d0_vs_eta;1	Residual mean of d0
  KEY: TH1F	reswidth_d0_vs_eta;1	Residual width of d0
  KEY: TH2F	res_d0_vs_pT;1	Residual of d0 vs pT
  KEY: TH1F	resmean_d0_vs_pT;1	Residual mean of d0
  KEY: TH1F	reswidth_d0_vs_pT;1	Residual width of d0
  KEY: TH1F	pull_d0;1	Pull of d0
  KEY: TH2F	pull_d0_vs_eta;1	Pull of d0 vs eta
  KEY: TH1F	pullmean_d0_vs_eta;1	Pull mean of d0
  KEY: TH1F	pullwidth_d0_vs_eta;1	Pull width of d0
  KEY: TH2F	pull_d0_vs_pT;1	Pull of d0 vs pT
  KEY: TH1F	pullmean_d0_vs_pT;1	Pull mean of d0
  KEY: TH1F	pullwidth_d0_vs_pT;1	Pull width of d0
  KEY: TH1F	res_z0;1	Residual of z0
  KEY: TH2F	res_z0_vs_eta;1	Residual of z0 vs eta
  KEY: TH1F	resmean_z0_vs_eta;1	Residual mean of z0
  KEY: TH1F	reswidth_z0_vs_eta;1	Residual width of z0
  KEY: TH2F	res_z0_vs_pT;1	Residual of z0 vs pT
  KEY: TH1F	resmean_z0_vs_pT;1	Residual mean of z0
  KEY: TH1F	reswidth_z0_vs_pT;1	Residual width of z0
  KEY: TH1F	pull_z0;1	Pull of z0
  KEY: TH2F	pull_z0_vs_eta;1	Pull of z0 vs eta
  KEY: TH1F	pullmean_z0_vs_eta;1	Pull mean of z0
  KEY: TH1F	pullwidth_z0_vs_eta;1	Pull width of z0
  KEY: TH2F	pull_z0_vs_pT;1	Pull of z0 vs pT
  KEY: TH1F	pullmean_z0_vs_pT;1	Pull mean of z0
  KEY: TH1F	pullwidth_z0_vs_pT;1	Pull width of z0
  KEY: TH1F	res_phi;1	Residual of phi
  KEY: TH2F	res_phi_vs_eta;1	Residual of phi vs eta
  KEY: TH1F	resmean_phi_vs_eta;1	Residual mean of phi
  KEY: TH1F	reswidth_phi_vs_eta;1	Residual width of phi
  KEY: TH2F	res_phi_vs_pT;1	Residual of phi vs pT
  KEY: TH1F	resmean_phi_vs_pT;1	Residual mean of phi
  KEY: TH1F	reswidth_phi_vs_pT;1	Residual width of phi
  KEY: TH1F	pull_phi;1	Pull of phi
  KEY: TH2F	pull_phi_vs_eta;1	Pull of phi vs eta
  KEY: TH1F	pullmean_phi_vs_eta;1	Pull mean of phi
  KEY: TH1F	pullwidth_phi_vs_eta;1	Pull width of phi
  KEY: TH2F	pull_phi_vs_pT;1	Pull of phi vs pT
  KEY: TH1F	pullmean_phi_vs_pT;1	Pull mean of phi
  KEY: TH1F	pullwidth_phi_vs_pT;1	Pull width of phi
  KEY: TH1F	res_theta;1	Residual of theta
  KEY: TH2F	res_theta_vs_eta;1	Residual of theta vs eta
  KEY: TH1F	resmean_theta_vs_eta;1	Residual mean of theta
  KEY: TH1F	reswidth_theta_vs_eta;1	Residual width of theta
  KEY: TH2F	res_theta_vs_pT;1	Residual of theta vs pT
  KEY: TH1F	resmean_theta_vs_pT;1	Residual mean of theta
  KEY: TH1F	reswidth_theta_vs_pT;1	Residual width of theta
  KEY: TH1F	pull_theta;1	Pull of theta
  KEY: TH2F	pull_theta_vs_eta;1	Pull of theta vs eta
  KEY: TH1F	pullmean_theta_vs_eta;1	Pull mean of theta
  KEY: TH1F	pullwidth_theta_vs_eta;1	Pull width of theta
  KEY: TH2F	pull_theta_vs_pT;1	Pull of theta vs pT
  KEY: TH1F	pullmean_theta_vs_pT;1	Pull mean of theta
  KEY: TH1F	pullwidth_theta_vs_pT;1	Pull width of theta
  KEY: TH1F	res_qop;1	Residual of qop
  KEY: TH2F	res_qop_vs_eta;1	Residual of qop vs eta
  KEY: TH1F	resmean_qop_vs_eta;1	Residual mean of qop
  KEY: TH1F	reswidth_qop_vs_eta;1	Residual width of qop
  KEY: TH2F	res_qop_vs_pT;1	Residual of qop vs pT
  KEY: TH1F	resmean_qop_vs_pT;1	Residual mean of qop
  KEY: TH1F	reswidth_qop_vs_pT;1	Residual width of qop
  KEY: TH1F	pull_qop;1	Pull of qop
  KEY: TH2F	pull_qop_vs_eta;1	Pull of qop vs eta
  KEY: TH1F	pullmean_qop_vs_eta;1	Pull mean of qop
  KEY: TH1F	pullwidth_qop_vs_eta;1	Pull width of qop
  KEY: TH2F	pull_qop_vs_pT;1	Pull of qop vs pT
  KEY: TH1F	pullmean_qop_vs_pT;1	Pull mean of qop
  KEY: TH1F	pullwidth_qop_vs_pT;1	Pull width of qop
  KEY: TH1F	res_t;1	Residual of t
  KEY: TH2F	res_t_vs_eta;1	Residual of t vs eta
  KEY: TH1F	resmean_t_vs_eta;1	Residual mean of t
  KEY: TH1F	reswidth_t_vs_eta;1	Residual width of t
  KEY: TH2F	res_t_vs_pT;1	Residual of t vs pT
  KEY: TH1F	resmean_t_vs_pT;1	Residual mean of t
  KEY: TH1F	reswidth_t_vs_pT;1	Residual width of t
  KEY: TH1F	pull_t;1	Pull of t
  KEY: TH2F	pull_t_vs_eta;1	Pull of t vs eta
  KEY: TH1F	pullmean_t_vs_eta;1	Pull mean of t
  KEY: TH1F	pullwidth_t_vs_eta;1	Pull width of t
  KEY: TH2F	pull_t_vs_pT;1	Pull of t vs pT
  KEY: TH1F	pullmean_t_vs_pT;1	Pull mean of t
  KEY: TH1F	pullwidth_t_vs_pT;1	Pull width of t
  KEY: TEfficiency	trackeff_vs_pT;1	Tracking efficiency
  KEY: TEfficiency	trackeff_vs_eta;1	Tracking efficiency
  KEY: TEfficiency	trackeff_vs_phi;1	Tracking efficiency
  KEY: TProfile	nStates_vs_eta;1	Number of total states vs. #eta
  KEY: TProfile	nMeasurements_vs_eta;1	Number of measurements vs. #eta
  KEY: TProfile	nOutliers_vs_eta;1	Number of outliers vs. #eta
  KEY: TProfile	nHoles_vs_eta;1	Number of holes vs. #eta
  KEY: TProfile	nStates_vs_pT;1	Number of total states vs. pT
  KEY: TProfile	nMeasurements_vs_pT;1	Number of measurements vs. pT
  KEY: TProfile	nOutliers_vs_pT;1	Number of outliers vs. pT
  KEY: TProfile	nHoles_vs_pT;1	Number of holes vs. pT
  */
