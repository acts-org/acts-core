// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <array>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <TCanvas.h>
#include <TColor.h>
#include <TDirectory.h>
#include <TError.h>
#include <TF1.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TLegend.h>
#include <TMath.h>
#include <TProfile2D.h>
#include <TStyle.h>
#include <TTree.h>

#include "CommonUtils.h"
#include "TreeReader.h"

using namespace ROOT;

/// Plot the bound parameter resolutions
///
/// (loc1, phi, theta, q/p, t) at all track states from root file produced by
/// the RootTrackStatesWriter
///
/// @param inFile the input root file
/// @param treeName the input tree name (default: 'trackstates)
/// @param outFile the output root file
/// @param pTypes the track parameter types (prt, flt, smt)
/// @param saveAs the plot saving type
int boundParamResolution(const std::string& inFile, const std::string& treeName,
                         const std::string& outFile, bool predicted = true,
                         bool filtered = true, bool smoothed = true,
                         bool fitFiltered = true, bool fitPredicted = true,
                         bool fitSmoothed = true,
                         const std::string& saveAs = "") {
  // Some style options
  gStyle->SetOptFit(0000);
  gStyle->SetOptStat(0000);
  gStyle->SetPadLeftMargin(0.20);
  gStyle->SetPadRightMargin(0.1);
  gStyle->SetPadTopMargin(0.1);
  gStyle->SetPadBottomMargin(0.15);

  // Pull range, residual ranges are automatically determined
  double pullRange = 5;

  //
  // Residual ranges should be largest in predicted and smallest in smoothed,
  // hence reverse the order.
  //
  std::string range_tag = "smt";
  if (predicted) {
    range_tag = "prt";
  } else if (filtered) {
    range_tag = "flt";
  }

  // Section 0: file handling ---------------------------------------------
  //
  // Open root file written by RootTrackWriter
  // Create output root file
  std::cout << "Opening file: " << inFile << std::endl;
  TFile* file = TFile::Open(inFile.c_str(), "read");

  // Bail out if no tree was found
  if (file == nullptr) {
    return -1;
  }

  std::cout << "Reading tree: " << treeName << std::endl;
  TTree* tree = static_cast<TTree*>(file->Get(treeName.c_str()));

  // Bail out if no tree was found
  if (tree == nullptr) {
    return -2;
  }

  TFile* out = TFile::Open(outFile.c_str(), "recreate");
  out->cd();

  // Section 1: geometry parsing ------------------------------------------
  //
  // Gathers accessible volume_id and layer_id values
  // Draw the volume_id : layer_id correlation matrix
  std::map<int, std::vector<int>> volLayIds;
  TCanvas* geometryCanvas =
      new TCanvas("volLayCanvas", "Volume Layer Matrix", 10, 10, 450, 600);
  geometryCanvas->Divide(1, 2);
  // Volume/Layer Id
  geometryCanvas->cd(1);
  tree->Draw("layer_id:volume_id>>volID_layID(100,0.5,100.5,100,0.5,100.5)", "",
             "colz");
  auto h2_volID_layID = dynamic_cast<TH2F*>(out->Get("volID_layID"));
  setHistStyle(h2_volID_layID, 2);
  h2_volID_layID->Write();
  // Geometry size
  geometryCanvas->cd(2);
  std::string rz_draw_string = "(sqrt(g_x_";
  rz_draw_string += range_tag;
  rz_draw_string += "*g_x_";
  rz_draw_string += range_tag;
  rz_draw_string += "+g_y_";
  rz_draw_string += range_tag;
  rz_draw_string += "*g_y_";
  rz_draw_string += range_tag;
  rz_draw_string += ")):g_z_";
  rz_draw_string += range_tag;
  rz_draw_string += ">>geo_dim";
  tree->Draw(rz_draw_string.c_str());
  auto h2_geo_dim = dynamic_cast<TH2F*>(out->Get("geo_dim"));
  setHistStyle(h2_geo_dim, 2);
  float detectorZ = 1.15 * h2_geo_dim->GetXaxis()->GetXmax();
  float detectorR = 1.15 * h2_geo_dim->GetYaxis()->GetXmax();
  h2_geo_dim->Write();

  // Save the plots on screen
  if (!saveAs.empty()) {
    geometryCanvas->SaveAs((std::string("all_vol_lay_ids.") + saveAs).c_str());
  }

  // Now determine the valid volume / layer bins
  int volBins = h2_volID_layID->GetXaxis()->GetNbins();
  int layBins = h2_volID_layID->GetYaxis()->GetNbins();
  // Add the overall plots
  volLayIds[-1] = {-1};
  // Go through volumes and add plots per volume
  for (int volID = 1; volID <= volBins; ++volID) {
    for (int layID = 1; layID <= layBins; ++layID) {
      if (h2_volID_layID->GetBinContent(volID, layID) != 0.) {
        if (!volLayIds.contains(volID)) {
          // First occurrence of this layer, add -1 for volume plots
          volLayIds[volID] = {-1, layID};
        } else {
          volLayIds[volID].push_back(layID);
        }
      }
    }
  }

  // Section 2: Branch assignment -----------------------------------------
  //
  // Helper for assigning branches to the input tree
  TrackStatesReader tsReader(tree, false);

  // Section 3: Histogram booking -----------------------------------------

  TCanvas* rangeCanvas =
      new TCanvas("rangeCanvas", "Range Estimation", 10, 10, 900, 600);
  rangeCanvas->Divide(3, 2);

  std::vector<std::string> res_ranges = {std::string("res_eLOC0_") + range_tag,
                                         std::string("res_eLOC1_") + range_tag,
                                         std::string("res_ePHI_") + range_tag,
                                         std::string("res_eTHETA_") + range_tag,
                                         std::string("res_eQOP_") + range_tag,
                                         std::string("res_eT_") + range_tag};

  /// Helper method to make a volLayId string for identification & cut
  ///
  /// ensures a unique identification
  auto volLayIdCut = [](int vol, int lay) -> std::array<std::string, 2> {
    if (vol < 0 && lay < 0) {
      return {std::string("all_"), ""};
    }

    std::string vlstr = "vol" + std::to_string(vol);
    std::string vlcut = "volume_id == " + std::to_string(vol);
    if (lay > 0) {
      vlstr += (std::string("_lay") + std::to_string(lay));
      vlcut += (std::string(" && layer_id == ") + std::to_string(lay));
    }
    vlstr += std::string("_");
    return {vlstr, vlcut};
  };

  /// Helper method to estimate the ranges
  ///
  /// Takes identification & cut from the first one
  auto histRanges =
      [&](const std::array<std::string, 2>& vlIdCut) -> std::array<float, 6> {
    std::array<float, 6> ranges = {0., 0., 0., 0., 0., 0.};
    for (unsigned int ir = 0; ir < 6; ++ir) {
      rangeCanvas->cd(ir + 1);
      std::string drawString = res_ranges[ir] + ">>";
      std::string histString =
          vlIdCut[0] + std::string("range_") + res_ranges[ir];
      tree->Draw((drawString + histString).c_str(), vlIdCut[1].c_str());
      auto h1_range = dynamic_cast<TH1F*>(out->Get(histString.c_str()));
      h1_range->Write();
      float range = pullRange * h1_range->GetRMS();
      ranges[ir] = range;
    }
    if (!saveAs.empty()) {
      rangeCanvas->SaveAs(
          (vlIdCut[0] + std::string("res_ranges.") + saveAs).c_str());
    }
    return ranges;
  };

  // Track parameter name
  std::vector<std::string> paramNames = {"loc0",   "loc1", "#phi",
                                         "#theta", "q/p",  "t"};

  // Book histograms (with adapted range):
  //
  // Global profile histograms : residuals/pulls
  std::array<TProfile2D*, 6> p2d_res_zr_prt{};
  std::array<TProfile2D*, 6> p2d_res_zr_flt{};
  std::array<TProfile2D*, 6> p2d_res_zr_smt{};
  std::array<TProfile2D*, 6> p2d_pull_zr_prt{};
  std::array<TProfile2D*, 6> p2d_pull_zr_flt{};
  std::array<TProfile2D*, 6> p2d_pull_zr_smt{};

  for (unsigned int ipar = 0; ipar < paramNames.size(); ++ipar) {
    const auto& par = paramNames[ipar];

    if (predicted) {
      p2d_res_zr_prt[ipar] =
          new TProfile2D(Form("all_prof_res_prt_%s", par.c_str()),
                         Form("residual profile of %s", par.c_str()), 100,
                         -detectorZ, detectorZ, 50, 0., detectorR);

      p2d_pull_zr_prt[ipar] =
          new TProfile2D(Form("all_prof_pull_prt_%s", par.c_str()),
                         Form("pull profile of %s", par.c_str()), 100,
                         -detectorZ, detectorZ, 50, 0., detectorR);

      p2d_res_zr_prt[ipar]->SetErrorOption("s");
      p2d_res_zr_prt[ipar]->GetXaxis()->SetTitle("z [mm]");
      p2d_res_zr_prt[ipar]->GetYaxis()->SetTitle("R [mm]");
      p2d_res_zr_prt[ipar]->GetZaxis()->SetTitle(Form("r_{%s}", par.c_str()));
      setHistStyle(p2d_res_zr_prt[ipar], 1);

      p2d_pull_zr_prt[ipar]->SetErrorOption("s");
      p2d_pull_zr_prt[ipar]->GetXaxis()->SetTitle("z [mm]");
      p2d_pull_zr_prt[ipar]->GetYaxis()->SetTitle("R [mm]");
      p2d_pull_zr_prt[ipar]->GetZaxis()->SetTitle(
          Form("pull_{%s}", par.c_str()));
      setHistStyle(p2d_pull_zr_prt[ipar], 1);
    }

    if (filtered) {
      p2d_res_zr_flt[ipar] =
          new TProfile2D(Form("all_prof_res_flt_%s", par.c_str()),
                         Form("residual profile of %s", par.c_str()), 100,
                         -detectorZ, detectorZ, 50, 0., detectorR);
      p2d_pull_zr_flt[ipar] =
          new TProfile2D(Form("all_prof_pull_flt_%s", par.c_str()),
                         Form("pull profile of %s", par.c_str()), 100,
                         -detectorZ, detectorZ, 50, 0., detectorR);

      p2d_res_zr_flt[ipar]->SetErrorOption("s");
      p2d_res_zr_flt[ipar]->GetXaxis()->SetTitle("z [mm]");
      p2d_res_zr_flt[ipar]->GetYaxis()->SetTitle("R [mm]");
      p2d_res_zr_flt[ipar]->GetZaxis()->SetTitle(Form("r_{%s}", par.c_str()));
      setHistStyle(p2d_res_zr_flt[ipar], 2);

      p2d_pull_zr_flt[ipar]->SetErrorOption("s");
      p2d_pull_zr_flt[ipar]->GetXaxis()->SetTitle("z [mm]");
      p2d_pull_zr_flt[ipar]->GetYaxis()->SetTitle("R [mm]");
      p2d_pull_zr_flt[ipar]->GetZaxis()->SetTitle(
          Form("pull_{%s}", par.c_str()));
      setHistStyle(p2d_pull_zr_flt[ipar], 2);
    }

    if (smoothed) {
      p2d_res_zr_smt[ipar] =
          new TProfile2D(Form("all_prof_res_smt_%s", par.c_str()),
                         Form("residual profile of %s", par.c_str()), 100,
                         -detectorZ, detectorZ, 50, 0., detectorR);

      p2d_pull_zr_smt[ipar] =
          new TProfile2D(Form("all_prof_pull_smt_%s", par.c_str()),
                         Form("pull profile of %s", par.c_str()), 100,
                         -detectorZ, detectorZ, 50, 0., detectorR);

      p2d_res_zr_smt[ipar]->SetErrorOption("s");
      p2d_pull_zr_smt[ipar]->SetErrorOption("s");

      p2d_res_zr_smt[ipar]->GetXaxis()->SetTitle("z [mm]");
      p2d_res_zr_smt[ipar]->GetYaxis()->SetTitle("R [mm]");
      p2d_res_zr_smt[ipar]->GetZaxis()->SetTitle(Form("r_{%s}", par.c_str()));
      setHistStyle(p2d_res_zr_smt[ipar], 4);

      p2d_pull_zr_smt[ipar]->GetXaxis()->SetTitle("z [mm]");
      p2d_pull_zr_smt[ipar]->GetYaxis()->SetTitle("R [mm]");
      p2d_pull_zr_smt[ipar]->GetZaxis()->SetTitle(
          Form("pull_{%s}", par.c_str()));
      setHistStyle(p2d_pull_zr_smt[ipar], 4);
    }
  }

  // Residual / Pull histograms
  std::map<std::string, TH1F*> res_prt;
  std::map<std::string, TH1F*> res_flt;
  std::map<std::string, TH1F*> res_smt;
  std::map<std::string, TH1F*> pull_prt;
  std::map<std::string, TH1F*> pull_flt;
  std::map<std::string, TH1F*> pull_smt;

  // - per layer (identified by vol, lay)
  // - per volume (identified by vol, -1)
  // - overall (identified by lay)
  for (const auto& [vol, layers] : volLayIds) {
    for (const auto& lay : layers) {
      // Estimate the ranges from smoothed
      auto vlIdCut = volLayIdCut(vol, lay);
      auto ranges = histRanges(vlIdCut);

      // Residual range
      std::map<std::pair<std::string, std::string>, double> paramResidualRange =
          {{{"loc0", "l_{0}"}, ranges[0]}, {{"loc1", "l_{1}"}, ranges[1]},
           {{"#phi", "#phi"}, ranges[2]},  {{"#theta", "#theta"}, ranges[3]},
           {{"q/p", "q/p"}, ranges[4]},    {{"t", "t"}, ranges[5]}};

      // Create the hists and set up for them
      for (const auto& [partwin, resRange] : paramResidualRange) {
        // histogram creation
        std::string par = partwin.first;
        std::string id_par = vlIdCut[0] + par;

        TString par_string(partwin.second.c_str());
        TString res_string =
            par_string + TString("^{rec} - ") + par_string + TString("^{true}");

        TString pull_string = TString("(") + res_string +
                              TString(")/#sigma_{") + par_string + TString("}");

        if (predicted) {
          res_prt[id_par] =
              new TH1F(Form((vlIdCut[0] + std::string("res_prt_%s")).c_str(),
                            par.c_str()),
                       Form("residual of %s", par.c_str()), 100, -1 * resRange,
                       resRange);
          res_prt[id_par]->GetXaxis()->SetTitle(res_string.Data());
          res_prt[id_par]->GetYaxis()->SetTitle("Entries");
          setHistStyle(res_prt[id_par], kRed);

          pull_prt[id_par] = new TH1F(
              Form((vlIdCut[0] + std::string("pull_prt_%s")).c_str(),
                   par.c_str()),
              Form("pull of %s", par.c_str()), 100, -1 * pullRange, pullRange);
          pull_prt[id_par]->GetXaxis()->SetTitle(pull_string.Data());
          pull_prt[id_par]->GetYaxis()->SetTitle("Arb. Units");
          setHistStyle(pull_prt[id_par], kRed);
        }

        if (filtered) {
          res_flt[id_par] =
              new TH1F(Form((vlIdCut[0] + std::string("res_flt_%s")).c_str(),
                            par.c_str()),
                       Form("residual of %s", par.c_str()), 100, -1 * resRange,
                       resRange);
          res_flt[id_par]->GetXaxis()->SetTitle(res_string.Data());
          res_flt[id_par]->GetYaxis()->SetTitle("Entries");
          setHistStyle(res_flt[id_par], kBlue);

          pull_flt[id_par] = new TH1F(
              Form((vlIdCut[0] + std::string("pull_flt_%s")).c_str(),
                   par.c_str()),
              Form("pull of %s", par.c_str()), 100, -1 * pullRange, pullRange);
          pull_flt[id_par]->GetXaxis()->SetTitle(pull_string.Data());
          pull_flt[id_par]->GetYaxis()->SetTitle("Arb. Units");
          setHistStyle(pull_flt[id_par], kBlue);
        }

        if (smoothed) {
          res_smt[id_par] =
              new TH1F(Form((vlIdCut[0] + std::string("res_smt_%s")).c_str(),
                            par.c_str()),
                       Form("residual of %s", par.c_str()), 100, -1 * resRange,
                       resRange);

          res_smt[id_par]->GetXaxis()->SetTitle(res_string.Data());
          res_smt[id_par]->GetYaxis()->SetTitle("Entries");
          setHistStyle(res_smt[id_par], kBlack);

          pull_smt[id_par] = new TH1F(
              Form((vlIdCut[0] + std::string("pull_smt_%s")).c_str(),
                   par.c_str()),
              Form("pull of %s", par.c_str()), 100, -1 * pullRange, pullRange);

          pull_smt[id_par]->GetXaxis()->SetTitle(pull_string.Data());
          pull_smt[id_par]->GetYaxis()->SetTitle("Arb. Units");
          setHistStyle(pull_smt[id_par], kBlack);
        }
      }
    }
  }

  // Section 4: Histogram filling -----------------------------------------
  //
  // - Running through the entries and filling the histograms
  int entries = tree->GetEntries();
  for (int j = 0; j < entries; j++) {
    tsReader.getEntry(j);

    for (unsigned int i = 0; i < tsReader.nMeasurements; i++) {
      // global profile filling
      if (predicted && tsReader.predicted->at(i)) {
        auto x_prt = tsReader.g_x_prt->at(i);
        auto y_prt = tsReader.g_y_prt->at(i);
        auto r_prt = std::hypot(x_prt, y_prt);
        auto z_prt = tsReader.g_z_prt->at(i);
        p2d_res_zr_prt[0]->Fill(z_prt, r_prt, tsReader.res_LOC0_prt->at(i));
        p2d_res_zr_prt[1]->Fill(z_prt, r_prt, tsReader.res_LOC1_prt->at(i));
        p2d_res_zr_prt[2]->Fill(z_prt, r_prt, tsReader.res_PHI_prt->at(i));
        p2d_res_zr_prt[3]->Fill(z_prt, r_prt, tsReader.res_THETA_prt->at(i));
        p2d_res_zr_prt[4]->Fill(z_prt, r_prt, tsReader.res_QOP_prt->at(i));
        p2d_res_zr_prt[5]->Fill(z_prt, r_prt, tsReader.res_T_prt->at(i));
        p2d_pull_zr_prt[0]->Fill(z_prt, r_prt, tsReader.pull_LOC0_prt->at(i));
        p2d_pull_zr_prt[1]->Fill(z_prt, r_prt, tsReader.pull_LOC1_prt->at(i));
        p2d_pull_zr_prt[2]->Fill(z_prt, r_prt, tsReader.pull_PHI_prt->at(i));
        p2d_pull_zr_prt[3]->Fill(z_prt, r_prt, tsReader.pull_THETA_prt->at(i));
        p2d_pull_zr_prt[4]->Fill(z_prt, r_prt, tsReader.pull_QOP_prt->at(i));
        p2d_pull_zr_prt[5]->Fill(z_prt, r_prt, tsReader.pull_T_prt->at(i));
      }
      if (filtered && tsReader.filtered->at(i)) {
        auto x_flt = tsReader.g_x_flt->at(i);
        auto y_flt = tsReader.g_y_flt->at(i);
        auto r_flt = std::hypot(x_flt, y_flt);
        auto z_flt = tsReader.g_z_flt->at(i);
        p2d_res_zr_flt[0]->Fill(z_flt, r_flt, tsReader.res_LOC0_flt->at(i));
        p2d_res_zr_flt[1]->Fill(z_flt, r_flt, tsReader.res_LOC1_flt->at(i));
        p2d_res_zr_flt[2]->Fill(z_flt, r_flt, tsReader.res_PHI_flt->at(i));
        p2d_res_zr_flt[3]->Fill(z_flt, r_flt, tsReader.res_THETA_flt->at(i));
        p2d_res_zr_flt[4]->Fill(z_flt, r_flt, tsReader.res_QOP_flt->at(i));
        p2d_res_zr_flt[5]->Fill(z_flt, r_flt, tsReader.res_T_flt->at(i));
        p2d_pull_zr_flt[0]->Fill(z_flt, r_flt, tsReader.pull_LOC0_flt->at(i));
        p2d_pull_zr_flt[1]->Fill(z_flt, r_flt, tsReader.pull_LOC1_flt->at(i));
        p2d_pull_zr_flt[2]->Fill(z_flt, r_flt, tsReader.pull_PHI_flt->at(i));
        p2d_pull_zr_flt[3]->Fill(z_flt, r_flt, tsReader.pull_THETA_flt->at(i));
        p2d_pull_zr_flt[4]->Fill(z_flt, r_flt, tsReader.pull_QOP_flt->at(i));
        p2d_pull_zr_flt[5]->Fill(z_flt, r_flt, tsReader.pull_T_flt->at(i));
      }
      if (smoothed && tsReader.smoothed->at(i)) {
        auto x_smt = tsReader.g_x_smt->at(i);
        auto y_smt = tsReader.g_y_smt->at(i);
        auto r_smt = std::hypot(x_smt, y_smt);
        auto z_smt = tsReader.g_z_smt->at(i);
        p2d_res_zr_smt[0]->Fill(z_smt, r_smt, tsReader.res_LOC0_smt->at(i));
        p2d_res_zr_smt[1]->Fill(z_smt, r_smt, tsReader.res_LOC1_smt->at(i));
        p2d_res_zr_smt[2]->Fill(z_smt, r_smt, tsReader.res_PHI_smt->at(i));
        p2d_res_zr_smt[3]->Fill(z_smt, r_smt, tsReader.res_THETA_smt->at(i));
        p2d_res_zr_smt[4]->Fill(z_smt, r_smt, tsReader.res_QOP_smt->at(i));
        p2d_res_zr_smt[5]->Fill(z_smt, r_smt, tsReader.res_T_smt->at(i));
        p2d_pull_zr_smt[0]->Fill(z_smt, r_smt, tsReader.pull_LOC0_smt->at(i));
        p2d_pull_zr_smt[1]->Fill(z_smt, r_smt, tsReader.pull_LOC1_smt->at(i));
        p2d_pull_zr_smt[2]->Fill(z_smt, r_smt, tsReader.pull_PHI_smt->at(i));
        p2d_pull_zr_smt[3]->Fill(z_smt, r_smt, tsReader.pull_THETA_smt->at(i));
        p2d_pull_zr_smt[4]->Fill(z_smt, r_smt, tsReader.pull_QOP_smt->at(i));
        p2d_pull_zr_smt[5]->Fill(z_smt, r_smt, tsReader.pull_T_smt->at(i));
      }

      int vol = tsReader.volume_id->at(i);
      int lay = tsReader.layer_id->at(i);

      /// Always fill (-1,-1), (vol, -1), (vol, lay)
      std::vector<std::array<int, 2>> fillIds = {
          {-1, -1}, {vol, -1}, {vol, lay}};

      for (const auto& fid : fillIds) {
        auto vlID = volLayIdCut(fid[0], fid[1])[0];
        // Fill predicated parameters
        if (predicted && tsReader.predicted->at(i)) {
          res_prt[vlID + paramNames[0]]->Fill(tsReader.res_LOC0_prt->at(i), 1);
          res_prt[vlID + paramNames[1]]->Fill(tsReader.res_LOC1_prt->at(i), 1);
          res_prt[vlID + paramNames[2]]->Fill(tsReader.res_PHI_prt->at(i), 1);
          res_prt[vlID + paramNames[3]]->Fill(tsReader.res_THETA_prt->at(i), 1);
          res_prt[vlID + paramNames[4]]->Fill(tsReader.res_QOP_prt->at(i), 1);
          res_prt[vlID + paramNames[5]]->Fill(tsReader.res_T_prt->at(i), 1);
          pull_prt[vlID + paramNames[0]]->Fill(tsReader.pull_LOC0_prt->at(i),
                                               1);
          pull_prt[vlID + paramNames[1]]->Fill(tsReader.pull_LOC1_prt->at(i),
                                               1);
          pull_prt[vlID + paramNames[2]]->Fill(tsReader.pull_PHI_prt->at(i), 1);
          pull_prt[vlID + paramNames[3]]->Fill(tsReader.pull_THETA_prt->at(i),
                                               1);
          pull_prt[vlID + paramNames[4]]->Fill(tsReader.pull_QOP_prt->at(i), 1);
          pull_prt[vlID + paramNames[5]]->Fill(tsReader.pull_T_prt->at(i), 1);
        }
        // Fill filtered parameters
        if (filtered && tsReader.filtered->at(i)) {
          res_flt[vlID + paramNames[0]]->Fill(tsReader.res_LOC0_flt->at(i), 1);
          res_flt[vlID + paramNames[1]]->Fill(tsReader.res_LOC1_flt->at(i), 1);
          res_flt[vlID + paramNames[2]]->Fill(tsReader.res_PHI_flt->at(i), 1);
          res_flt[vlID + paramNames[3]]->Fill(tsReader.res_THETA_flt->at(i), 1);
          res_flt[vlID + paramNames[4]]->Fill(tsReader.res_QOP_flt->at(i), 1);
          res_flt[vlID + paramNames[5]]->Fill(tsReader.res_T_flt->at(i), 1);
          pull_flt[vlID + paramNames[0]]->Fill(tsReader.pull_LOC0_flt->at(i),
                                               1);
          pull_flt[vlID + paramNames[1]]->Fill(tsReader.pull_LOC1_flt->at(i),
                                               1);
          pull_flt[vlID + paramNames[2]]->Fill(tsReader.pull_PHI_flt->at(i), 1);
          pull_flt[vlID + paramNames[3]]->Fill(tsReader.pull_THETA_flt->at(i),
                                               1);
          pull_flt[vlID + paramNames[4]]->Fill(tsReader.pull_QOP_flt->at(i), 1);
          pull_flt[vlID + paramNames[5]]->Fill(tsReader.pull_T_flt->at(i), 1);
        }
        // Fill smoothed parameters
        if (smoothed && tsReader.smoothed->at(i)) {
          res_smt[vlID + paramNames[0]]->Fill(tsReader.res_LOC0_smt->at(i), 1);
          res_smt[vlID + paramNames[1]]->Fill(tsReader.res_LOC1_smt->at(i), 1);
          res_smt[vlID + paramNames[2]]->Fill(tsReader.res_PHI_smt->at(i), 1);
          res_smt[vlID + paramNames[3]]->Fill(tsReader.res_THETA_smt->at(i), 1);
          res_smt[vlID + paramNames[4]]->Fill(tsReader.res_QOP_smt->at(i), 1);
          res_smt[vlID + paramNames[5]]->Fill(tsReader.res_T_smt->at(i), 1);
          pull_smt[vlID + paramNames[0]]->Fill(tsReader.pull_LOC0_smt->at(i),
                                               1);
          pull_smt[vlID + paramNames[1]]->Fill(tsReader.pull_LOC1_smt->at(i),
                                               1);
          pull_smt[vlID + paramNames[2]]->Fill(tsReader.pull_PHI_smt->at(i), 1);
          pull_smt[vlID + paramNames[3]]->Fill(tsReader.pull_THETA_smt->at(i),
                                               1);
          pull_smt[vlID + paramNames[4]]->Fill(tsReader.pull_QOP_smt->at(i), 1);
          pull_smt[vlID + paramNames[5]]->Fill(tsReader.pull_T_smt->at(i), 1);
        }
      }
    }
  }

  // Section 5: Histogram plotting

  // Plotting global profiles
  auto respull_mean_prf =
      new TCanvas("respull_mean_prf",
                  "Residual/Pull Distributions - mean profiles", 1800, 800);
  respull_mean_prf->Divide(3, 2);

  auto respull_var_prf =
      new TCanvas("respull_var_prf",
                  "Residual/Pull Distributions - variance profiles", 1800, 800);
  respull_var_prf->Divide(3, 2);

  auto plotProfiles = [&](std::array<TProfile2D*, 6>& profiles,
                          const std::string& res_pull,
                          const std::string& type) -> void {
    // Mean
    for (std::size_t ipar = 0; ipar < paramNames.size(); ++ipar) {
      respull_mean_prf->cd(ipar + 1);

      if (res_pull == "pull") {
        profiles[ipar]->GetZaxis()->SetRangeUser(-1. * pullRange, pullRange);
      }
      adaptColorPalette(profiles[ipar], -1. * pullRange, pullRange, 0., 0.25,
                        104);
      profiles[ipar]->Draw("colz");
      profiles[ipar]->Write();
    }
    // Save the canvas: mean
    if (!saveAs.empty()) {
      respull_mean_prf->SaveAs((std::string("all_") + res_pull +
                                std::string("_mean_prf_") + type +
                                std::string(".") + saveAs)
                                   .c_str());
    }

    // Variance
    for (std::size_t ipar = 0; ipar < paramNames.size(); ++ipar) {
      respull_var_prf->cd(ipar + 1);
      auto zAxis = profiles[ipar]->GetXaxis();
      auto rAxis = profiles[ipar]->GetYaxis();
      auto binsZ = zAxis->GetNbins();
      auto binsR = rAxis->GetNbins();
      std::string hist_name = "all_";
      hist_name += res_pull;
      hist_name += "_";
      hist_name += paramNames[ipar];
      hist_name += "_";
      hist_name += type;
      TH2F* var =
          new TH2F(hist_name.c_str(),
                   (res_pull + std::string(" ") + paramNames[ipar]).c_str(),
                   binsZ, zAxis->GetXmin(), zAxis->GetXmax(), binsR,
                   rAxis->GetXmin(), rAxis->GetXmax());
      for (int iz = 1; iz <= binsZ; ++iz) {
        for (int ir = 1; ir <= binsR; ++ir) {
          if (profiles[ipar]->GetBinContent(iz, ir) != 0.) {
            var->SetBinContent(iz, ir, profiles[ipar]->GetBinError(iz, ir));
          }
        }
      }
      if (res_pull == "pull") {
        var->GetZaxis()->SetRangeUser(0., pullRange);
      }
      adaptColorPalette(var, 0., pullRange, 1., 0.5, 104);
      var->Draw("colz");
      var->Write();
    }
    // Save the canvas: pulls
    if (!saveAs.empty()) {
      respull_var_prf->SaveAs((std::string("all_") + res_pull +
                               std::string("_var_prf_") + type +
                               std::string(".") + saveAs)
                                  .c_str());
    }
  };

  // Plotting profiles: res/pulls
  if (predicted) {
    plotProfiles(p2d_res_zr_prt, "res", "prt");
    plotProfiles(p2d_pull_zr_prt, "pull", "prt");
  }
  if (filtered) {
    plotProfiles(p2d_res_zr_flt, "res", "flt");
    plotProfiles(p2d_pull_zr_flt, "pull", "flt");
  }
  if (smoothed) {
    plotProfiles(p2d_res_zr_smt, "res", "smt");
    plotProfiles(p2d_pull_zr_smt, "pull", "smt");
  }

  // Plotting residual
  auto residuals =
      new TCanvas("residuals", "Residual Distributions", 1200, 800);
  residuals->Divide(3, 2);

  auto pulls = new TCanvas("pulls", "Pull distributions", 1200, 800);
  pulls->Divide(3, 2);

  for (const auto& [vol, layers] : volLayIds) {
    for (const auto& lay : layers) {
      auto vlID = volLayIdCut(vol, lay)[0];

      // Residual plotting
      for (std::size_t ipar = 0; ipar < paramNames.size(); ipar++) {
        residuals->cd(ipar + 1);

        auto legend = new TLegend(0.7, 0.7, 0.9, 0.9);

        const std::string name = vlID + paramNames.at(ipar);

        // Draw them
        if (smoothed) {
          res_smt[name]->DrawNormalized("");
          res_smt[name]->Write();
          legend->AddEntry(res_smt[name], "smoothed", "l");
        }
        if (filtered) {
          std::string drawOptions = smoothed ? "same" : "";
          res_flt[name]->DrawNormalized(drawOptions.c_str());
          res_flt[name]->Write();
          legend->AddEntry(res_flt[name], "filtered", "l");
        }
        if (predicted) {
          std::string drawOptions = (smoothed || filtered) ? "same" : "";
          res_prt[name]->DrawNormalized(drawOptions.c_str());
          res_prt[name]->Write();
          legend->AddEntry(res_prt[name], "predicted", "l");
        }

        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextFont(42);
        legend->Draw();
      }
      if (!saveAs.empty()) {
        residuals->SaveAs((vlID + std::string("residuals.") + saveAs).c_str());
      }

      // Pull plotting & writing
      for (std::size_t ipar = 0; ipar < paramNames.size(); ipar++) {
        const std::string name = vlID + paramNames.at(ipar);

        pulls->cd(ipar + 1);

        auto legend = new TLegend(0.7, 0.5, 0.9, 0.9);

        // Fit the smoothed distribution
        if (smoothed) {
          auto drawOptions = "pe";

          auto scale = 1. / pull_smt[name]->Integral("width");
          pull_smt[name]->Scale(scale);
          pull_smt[name]->Draw(drawOptions);
          pull_smt[name]->Write();

          legend->AddEntry(pull_smt[name], "smoothed", "pe");

          if (fitSmoothed) {
            pull_smt[name]->Fit("gaus", "q");
            TF1* gauss = pull_smt[name]->GetFunction("gaus");
            gauss->SetLineColorAlpha(kBlack, 0.5);

            auto mu = gauss->GetParameter(1);
            auto sigma = gauss->GetParameter(2);
            auto mu_fit_info = TString::Format("#mu = %.3f", mu);
            auto su_fit_info = TString::Format("#sigma = %.3f", sigma);

            legend->AddEntry(gauss, mu_fit_info.Data(), "l");
            legend->AddEntry(pull_prt[name], su_fit_info.Data(), "");
          }
        }

        if (filtered) {
          auto drawOptions = smoothed ? "pe same" : "pe";

          auto scale = 1. / pull_flt[name]->Integral("width");
          pull_flt[name]->Scale(scale);
          pull_flt[name]->Draw(drawOptions);
          pull_flt[name]->Write();

          legend->AddEntry(pull_flt[name], "filtered", "pe");

          if (fitFiltered) {
            pull_flt[name]->Fit("gaus", "q", "same");
            TF1* gauss = pull_flt[name]->GetFunction("gaus");
            gauss->SetLineColorAlpha(kBlue, 0.5);

            auto mu = gauss->GetParameter(1);
            auto sigma = gauss->GetParameter(2);
            auto mu_fit_info = TString::Format("#mu = %.3f", mu);
            auto su_fit_info = TString::Format("#sigma = %.3f", sigma);

            legend->AddEntry(gauss, mu_fit_info.Data(), "l");
            legend->AddEntry(pull_prt[name], su_fit_info.Data(), "");
          }
        }

        if (predicted) {
          auto drawOptions = (smoothed || filtered) ? "pe same" : "pe";

          auto scale = 1. / pull_prt[name]->Integral("width");
          pull_prt[name]->Scale(scale);
          pull_prt[name]->Draw(drawOptions);
          pull_prt[name]->Write();

          legend->AddEntry(pull_prt[name], "predicted", "pe");

          if (fitPredicted) {
            pull_prt[name]->Fit("gaus", "q", "same");
            TF1* gauss = pull_prt[name]->GetFunction("gaus");
            gauss->SetLineColorAlpha(kRed, 0.5);

            auto mu = gauss->GetParameter(1);
            auto sigma = gauss->GetParameter(2);
            auto mu_fit_info = TString::Format("#mu = %.3f", mu);
            auto su_fit_info = TString::Format("#sigma = %.3f", sigma);

            legend->AddEntry(gauss, mu_fit_info.Data(), "l");
            legend->AddEntry(pull_prt[name], su_fit_info.Data(), "");
          }
        }

        // Reference standard normal pdf
        auto ref = new TF1("ref", "ROOT::Math::normal_pdf(x,1,0)", -5, 5);
        ref->SetLineColor(kGreen);
        ref->Draw("same");
        legend->AddEntry(ref, "#mu = 0 #sigma = 1", "l");

        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextFont(42);
        legend->Draw();
      }

      // Save the Canvases as pictures
      if (!saveAs.empty()) {
        pulls->SaveAs((vlID + std::string("pulls.") + saveAs).c_str());
      }
    }
  }

  if (out != nullptr) {
    out->Close();
  }
  return 0;
}
