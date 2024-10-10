// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/TrackFinding/MuonHoughSeeder.hpp"

#include "ActsExamples/EventData/MuonSimHit.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <variant>

#include "TBox.h"
#include "TLatex.h"
#include "TLegend.h"

namespace {
/// map the station name integers in the CSV to the ATLAS station names
static const std::map<int, std::string> stationDict{
    {0, "BIL"},  {1, "BIS"},  {7, "BIR"},  {2, "BML"},  {3, "BMS"},
    {8, "BMF"},  {53, "BME"}, {54, "BMG"}, {52, "BIM"}, {4, "BOL"},
    {5, "BOS"},  {9, "BOF"},  {10, "BOG"}, {6, "BEE"},  {14, "EEL"},
    {15, "EES"}, {13, "EIL"}, {49, "EIS"}, {17, "EML"}, {18, "EMS"},
    {20, "EOL"}, {21, "EOS"}};

}  // namespace

ActsExamples::MuonHoughSeeder::MuonHoughSeeder(
    ActsExamples::MuonHoughSeeder::Config cfg, Acts::Logging::Level lvl)
    : ActsExamples::IAlgorithm("MuonHoughSeeder", lvl),
      m_cfg(std::move(cfg)),
      m_logger(Acts::getDefaultLogger("MuonHoughSeeder", lvl)) {
  if (m_cfg.inDriftCircles.empty()) {
    throw std::invalid_argument(
        "MuonHoughSeeder: Missing drift circle collection");
  }
  if (m_cfg.inSimHits.empty()) {
    throw std::invalid_argument("MuonHoughSeeder: Missing sim hit collection");
  }

  m_inputDriftCircles.initialize(m_cfg.inDriftCircles);
  m_inputSimHits.initialize(m_cfg.inSimHits);
}

using PatternSeed = std::pair<double, double>;  // y0, tan theta

ActsExamples::ProcessCode ActsExamples::MuonHoughSeeder::execute(
    const AlgorithmContext& ctx) const {
  // read the hits and circles
  auto gotSH = m_inputSimHits(ctx);
  auto gotDC = m_inputDriftCircles(ctx);

  // configure the binning of the hough plane
  Acts::HoughTransformUtils::HoughPlaneConfig planeCfg;
  planeCfg.nBinsX = 1000;
  planeCfg.nBinsY = 1000;

  // instantiate the peak finder
  Acts::HoughTransformUtils::PeakFinders::IslandsAroundMaxConfig peakFinderCfg;
  peakFinderCfg.fractionCutoff = 0.7;
  peakFinderCfg.threshold = 3.;
  peakFinderCfg.minSpacingBetweenPeaks = {0., 30.};

  // and map the hough plane to parameter ranges.
  // The first coordinate is tan(theta), the second is z0 in mm
  Acts::HoughTransformUtils::HoughAxisRanges axisRanges{-3., 3., -2000., 2000.};

  // create the functions parametrising the hough space lines for drift circles.
  // Note that there are two solutions for each drift circle and angle

  // left solution
  auto houghParam_fromDC_left = [](double tanTheta, const DriftCircle& DC) {
    return DC.y() - tanTheta * DC.z() -
           DC.rDrift() / std::cos(std::atan(tanTheta));
  };
  // right solution
  auto houghParam_fromDC_right = [](double tanTheta, const DriftCircle& DC) {
    return DC.y() - tanTheta * DC.z() +
           DC.rDrift() / std::cos(std::atan(tanTheta));
  };

  // create the function parametrising the drift radius uncertainty
  auto houghWidth_fromDC = [](double, const DriftCircle& DC) {
    return std::min(DC.rDriftError() * 3.,
                    1.0);  // scale reported errors up to at least 1mm or 3
                           // times the reported error as drift circle calib not
                           // fully reliable at this stage
  };

  // store the true parameters
  std::vector<PatternSeed> truePatterns;

  // instantiate the hough plane
  Acts::HoughTransformUtils::HoughPlane<Acts::GeometryIdentifier::Value>
      houghPlane(planeCfg);
  // also insantiate the peak finder
  Acts::HoughTransformUtils::PeakFinders::IslandsAroundMax<
      Acts::GeometryIdentifier::Value>
      peakFinder(peakFinderCfg);

  // loop pver true hirs
  for (auto& SH : gotSH) {
    // read the identifier
    MuonMdtIdentifierFields detailedInfo =
        ActsExamples::splitId(SH.geometryId().value());
    // store the true parameters
    truePatterns.emplace_back(SH.direction().y() / SH.direction().z(),
                              SH.fourPosition().y());
    // std::cout<<"station name=" <<
    // static_cast<int>(SH.stationName)<<std::endl;
    std::cout << "direction = " << SH.direction().y() << std::endl;
    std::cout << "fourposition y = " << SH.fourPosition().y() << std::endl;
    std::cin.ignore();
    // reset the hough plane
    houghPlane.reset();
    int foundDC = 0;
    // loop over drift circles
    for (DriftCircle& DC : gotDC) {
      if (DC.stationEta() == detailedInfo.stationEta &&
          DC.stationPhi() == detailedInfo.stationPhi &&
          DC.stationName() == detailedInfo.stationName) {
        // build a single identifier for the drift circles
        MuonMdtIdentifierFields idf;
        idf.multilayer = DC.multilayer();
        idf.stationEta = DC.stationEta();
        idf.stationPhi = DC.stationPhi();
        idf.stationName = DC.stationName();
        idf.tubeLayer = DC.tubeLayer();
        idf.tube = DC.tube();
        auto identifier = compressId(idf);
        auto effectiveLayer = 3 * (DC.multilayer() - 1) + (DC.tubeLayer() - 1);
        ++foundDC;
        // populate the hough plane with both solutions.
        houghPlane.fill<DriftCircle>(DC, axisRanges, houghParam_fromDC_left,
                                     houghWidth_fromDC, identifier,
                                     effectiveLayer, 1.0);
        houghPlane.fill<DriftCircle>(DC, axisRanges, houghParam_fromDC_right,
                                     houghWidth_fromDC, identifier,
                                     effectiveLayer, 1.0);
      }
    }
    // now get the peaks
    auto maxima = peakFinder.findPeaks(houghPlane, axisRanges);

    // visualisation in ROOT
    // represent the hough space as a TH2
    TH2D houghHistoForPlot("houghHist", "HoughPlane;tan(#theta);z0 [mm]",
                           houghPlane.nBinsX(), axisRanges.xMin,
                           axisRanges.xMax, houghPlane.nBinsY(),
                           axisRanges.yMin, axisRanges.yMax);
    for (int bx = 0; bx < houghHistoForPlot.GetNbinsX(); ++bx) {
      for (int by = 0; by < houghHistoForPlot.GetNbinsY(); ++by) {
        houghHistoForPlot.SetBinContent(bx + 1, by + 1,
                                        houghPlane.nHits(bx, by));
      }
    }
    m_outCanvas->SetTitle(Form("Station %s, Eta %i, Phi %i",
                               stationDict.at(detailedInfo.stationName).c_str(),
                               static_cast<int>(detailedInfo.stationEta),
                               static_cast<int>(detailedInfo.stationPhi)));
    houghHistoForPlot.SetTitle(
        Form("Station %s, Eta %i, Phi %i",
             stationDict.at(detailedInfo.stationName).c_str(),
             static_cast<int>(detailedInfo.stationEta),
             static_cast<int>(detailedInfo.stationPhi)));
    m_outCanvas->cd();
    int maxHitsAsInt = static_cast<int>(houghPlane.maxHits());
    houghHistoForPlot.SetContour(maxHitsAsInt + 1);
    for (int k = 0; k < maxHitsAsInt + 1; ++k) {
      houghHistoForPlot.SetContourLevel(k, k - 0.5);
    }
    std::vector<std::unique_ptr<TMarker>> markers;
    std::vector<std::unique_ptr<TBox>> boxes;
    houghHistoForPlot.SetContourLevel(maxHitsAsInt + 1,
                                      houghHistoForPlot.GetMaximum() + 0.5);
    houghHistoForPlot.Draw("COLZ");
    // mark the true parameters
    auto trueMarker = std::make_unique<TMarker>(
        truePatterns.back().first, truePatterns.back().second, kOpenCrossX);
    trueMarker->SetMarkerSize(3);
    trueMarker->SetMarkerColor(kRed);
    trueMarker->Draw();

    // now draw the hough maxima
    for (auto& max : maxima) {
      markers.push_back(std::make_unique<TMarker>(max.x, max.y, kFullSquare));
      markers.back()->SetMarkerSize(1);
      markers.back()->SetMarkerColor(kBlue);
      markers.back()->Draw();
      boxes.push_back(std::make_unique<TBox>(max.x - max.wx, max.y - max.wy,
                                             max.x + max.wx, max.y + max.wy));
      boxes.back()->SetLineColor(kBlue);
      boxes.back()->SetFillStyle(1001);
      boxes.back()->SetFillColorAlpha(kBlue, 0.1);
      boxes.back()->SetLineWidth(0);
      boxes.back()->Draw();
    }
    TLegend legend(0.5, 0.7, 1. - gPad->GetRightMargin(),
                   1. - gPad->GetTopMargin());
    legend.AddEntry(trueMarker.get(), "True coordinates");
    legend.SetBorderSize(0);
    legend.SetFillStyle(0);
    legend.Draw();

    if (!boxes.empty()) {
      legend.AddEntry(markers.back().get(), "Hough maxima");
      legend.AddEntry(boxes.back().get(), "Hough uncertainties");
    }
    TLatex tl(gPad->GetLeftMargin() + 0.03, 1. - gPad->GetTopMargin() - 0.1,
              Form("%i drift circles on station", foundDC));
    tl.SetTextFont(43);
    tl.SetTextSize(24);
    tl.SetNDC();
    tl.Draw();
    m_outCanvas->SaveAs("HoughHistograms.pdf");
  }

  ACTS_VERBOSE("SH: " << gotSH.size());
  ACTS_VERBOSE("DC: " << gotDC.size());
  return ActsExamples::ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode ActsExamples::MuonHoughSeeder::initialize() {
  // book the output canvas
  m_outCanvas = std::make_unique<TCanvas>("canvas", "", 800, 800);
  m_outCanvas->SaveAs("HoughHistograms.pdf[");
  m_outCanvas->SetRightMargin(0.12);
  m_outCanvas->SetLeftMargin(0.12);
  gStyle->SetPalette(kGreyScale);
  gStyle->SetOptStat(0);
  return ProcessCode::SUCCESS;
}
ActsExamples::ProcessCode ActsExamples::MuonHoughSeeder::finalize() {
  m_outCanvas->SaveAs("HoughHistograms.pdf]");
  return ProcessCode::SUCCESS;
}
