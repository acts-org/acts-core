// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Validation/ResPlotTool.hpp"

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/Result.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"

#include <cmath>
#include <optional>
#include <ostream>

#include <TH1.h>
#include <TH2.h>
#include <TString.h>

ActsExamples::ResPlotTool::ResPlotTool(
    const ActsExamples::ResPlotTool::Config& cfg, Acts::Logging::Level lvl)
    : m_cfg(cfg), m_logger(Acts::getDefaultLogger("ResPlotTool", lvl)) {}

void ActsExamples::ResPlotTool::book(
    ResPlotTool::ResPlotCache& resPlotCache) const {
  PlotHelpers::Binning bEta = m_cfg.varBinning.at("Eta");
  PlotHelpers::Binning bPt = m_cfg.varBinning.at("Pt");
  PlotHelpers::Binning bPull = m_cfg.varBinning.at("Pull");

  ACTS_DEBUG("Initialize the histograms for residual and pull plots");
  for (unsigned int parID = 0; parID < Acts::eBoundSize; parID++) {
    std::string parName = m_cfg.paramNames.at(parID);

    std::string parResidual = "Residual_" + parName;
    // Binning for residual is parameter dependent
    PlotHelpers::Binning bResidual = m_cfg.varBinning.at(parResidual);

    // residual distributions
    resPlotCache.res[parName] = PlotHelpers::bookHisto(
        Form("res_%s", parName.c_str()),
        Form("Residual of %s", parName.c_str()), bResidual);
    // residual vs eta scatter plots
    resPlotCache.res_vs_eta[parName] = PlotHelpers::bookHisto(
        Form("res_%s_vs_eta", parName.c_str()),
        Form("Residual of %s vs eta", parName.c_str()), bEta, bResidual);
    // residual mean in each eta bin
    resPlotCache.resMean_vs_eta[parName] = PlotHelpers::bookHisto(
        Form("resmean_%s_vs_eta", parName.c_str()),
        Form("Residual mean of %s", parName.c_str()), bEta);
    // residual width in each eta bin
    resPlotCache.resWidth_vs_eta[parName] = PlotHelpers::bookHisto(
        Form("reswidth_%s_vs_eta", parName.c_str()),
        Form("Residual width of %s", parName.c_str()), bEta);
    // residual vs pT scatter plots
    resPlotCache.res_vs_pT[parName] = PlotHelpers::bookHisto(
        Form("res_%s_vs_pT", parName.c_str()),
        Form("Residual of %s vs pT", parName.c_str()), bPt, bResidual);
    // residual mean in each pT bin
    resPlotCache.resMean_vs_pT[parName] = PlotHelpers::bookHisto(
        Form("resmean_%s_vs_pT", parName.c_str()),
        Form("Residual mean of %s", parName.c_str()), bPt);
    // residual width in each pT bin
    resPlotCache.resWidth_vs_pT[parName] = PlotHelpers::bookHisto(
        Form("reswidth_%s_vs_pT", parName.c_str()),
        Form("Residual width of %s", parName.c_str()), bPt);

    // pull distritutions
    resPlotCache.pull[parName] =
        PlotHelpers::bookHisto(Form("pull_%s", parName.c_str()),
                               Form("Pull of %s", parName.c_str()), bPull);
    // pull vs eta scatter plots
    resPlotCache.pull_vs_eta[parName] = PlotHelpers::bookHisto(
        Form("pull_%s_vs_eta", parName.c_str()),
        Form("Pull of %s vs eta", parName.c_str()), bEta, bPull);
    // pull mean in each eta bin
    resPlotCache.pullMean_vs_eta[parName] =
        PlotHelpers::bookHisto(Form("pullmean_%s_vs_eta", parName.c_str()),
                               Form("Pull mean of %s", parName.c_str()), bEta);
    // pull width in each eta bin
    resPlotCache.pullWidth_vs_eta[parName] =
        PlotHelpers::bookHisto(Form("pullwidth_%s_vs_eta", parName.c_str()),
                               Form("Pull width of %s", parName.c_str()), bEta);
    // pull vs pT scatter plots
    resPlotCache.pull_vs_pT[parName] = PlotHelpers::bookHisto(
        Form("pull_%s_vs_pT", parName.c_str()),
        Form("Pull of %s vs pT", parName.c_str()), bPt, bPull);
    // pull mean in each pT bin
    resPlotCache.pullMean_vs_pT[parName] =
        PlotHelpers::bookHisto(Form("pullmean_%s_vs_pT", parName.c_str()),
                               Form("Pull mean of %s", parName.c_str()), bPt);
    // pull width in each pT bin
    resPlotCache.pullWidth_vs_pT[parName] =
        PlotHelpers::bookHisto(Form("pullwidth_%s_vs_pT", parName.c_str()),
                               Form("Pull width of %s", parName.c_str()), bPt);
  }
}

void ActsExamples::ResPlotTool::clear(ResPlotCache& resPlotCache) const {
  ACTS_DEBUG("Delete the hists.");
  for (unsigned int parID = 0; parID < Acts::eBoundSize; parID++) {
    std::string parName = m_cfg.paramNames.at(parID);
    delete resPlotCache.res.at(parName);
    delete resPlotCache.res_vs_eta.at(parName);
    delete resPlotCache.resMean_vs_eta.at(parName);
    delete resPlotCache.resWidth_vs_eta.at(parName);
    delete resPlotCache.res_vs_pT.at(parName);
    delete resPlotCache.resMean_vs_pT.at(parName);
    delete resPlotCache.resWidth_vs_pT.at(parName);
    delete resPlotCache.pull.at(parName);
    delete resPlotCache.pull_vs_eta.at(parName);
    delete resPlotCache.pullMean_vs_eta.at(parName);
    delete resPlotCache.pullWidth_vs_eta.at(parName);
    delete resPlotCache.pull_vs_pT.at(parName);
    delete resPlotCache.pullMean_vs_pT.at(parName);
    delete resPlotCache.pullWidth_vs_pT.at(parName);
  }
}

void ActsExamples::ResPlotTool::write(
    const ResPlotTool::ResPlotCache& resPlotCache) const {
  ACTS_DEBUG("Write the hists to output file.");
  for (unsigned int parID = 0; parID < Acts::eBoundSize; parID++) {
    std::string parName = m_cfg.paramNames.at(parID);
    resPlotCache.res.at(parName)->Write();
    resPlotCache.res_vs_eta.at(parName)->Write();
    resPlotCache.resMean_vs_eta.at(parName)->Write();
    resPlotCache.resWidth_vs_eta.at(parName)->Write();
    resPlotCache.res_vs_pT.at(parName)->Write();
    resPlotCache.resMean_vs_pT.at(parName)->Write();
    resPlotCache.resWidth_vs_pT.at(parName)->Write();
    resPlotCache.pull.at(parName)->Write();
    resPlotCache.pull_vs_eta.at(parName)->Write();
    resPlotCache.pullMean_vs_eta.at(parName)->Write();
    resPlotCache.pullWidth_vs_eta.at(parName)->Write();
    resPlotCache.pull_vs_pT.at(parName)->Write();
    resPlotCache.pullMean_vs_pT.at(parName)->Write();
    resPlotCache.pullWidth_vs_pT.at(parName)->Write();
  }
}

void ActsExamples::ResPlotTool::fill(
    ResPlotTool::ResPlotCache& resPlotCache, const Acts::GeometryContext& gctx,
    const SimParticleState& truthParticle,
    const Acts::BoundTrackParameters& fittedParamters) const {
  using ParametersVector = Acts::BoundTrackParameters::ParametersVector;
  using Acts::VectorHelpers::eta;
  using Acts::VectorHelpers::perp;
  using Acts::VectorHelpers::phi;
  using Acts::VectorHelpers::theta;

  // get the fitted parameter (at perigee surface) and its error
  auto trackParameter = fittedParamters.parameters();

  // get the perigee surface
  const auto& pSurface = fittedParamters.referenceSurface();

  // get the truth position and momentum
  ParametersVector truthParameter = ParametersVector::Zero();

  // get the truth perigee parameter
  auto intersection =
      pSurface
          .intersect(gctx, truthParticle.position(), truthParticle.direction())
          .closest();
  if (intersection.isValid()) {
    auto lpResult = pSurface.globalToLocal(gctx, intersection.position(),
                                           truthParticle.direction());
    assert(lpResult.ok());

    truthParameter[Acts::BoundIndices::eBoundLoc0] =
        lpResult.value()[Acts::BoundIndices::eBoundLoc0];
    truthParameter[Acts::BoundIndices::eBoundLoc1] =
        lpResult.value()[Acts::BoundIndices::eBoundLoc1];
  } else {
    ACTS_ERROR("Cannot get the truth perigee parameter");
  }
  truthParameter[Acts::BoundIndices::eBoundPhi] =
      phi(truthParticle.direction());
  truthParameter[Acts::BoundIndices::eBoundTheta] =
      theta(truthParticle.direction());
  truthParameter[Acts::BoundIndices::eBoundQOverP] = truthParticle.qOverP();
  truthParameter[Acts::BoundIndices::eBoundTime] = truthParticle.time();

  // get the truth eta and pT
  const auto truthEta = eta(truthParticle.direction());
  const auto truthPt = truthParticle.transverseMomentum();

  // fill the histograms for residual and pull
  for (unsigned int parID = 0; parID < Acts::eBoundSize; parID++) {
    std::string parName = m_cfg.paramNames.at(parID);
    float residual = trackParameter[parID] - truthParameter[parID];
    PlotHelpers::fillHisto(resPlotCache.res.at(parName), residual);
    PlotHelpers::fillHisto(resPlotCache.res_vs_eta.at(parName), truthEta,
                           residual);
    PlotHelpers::fillHisto(resPlotCache.res_vs_pT.at(parName), truthPt,
                           residual);

    if (fittedParamters.covariance().has_value()) {
      auto covariance = *fittedParamters.covariance();
      if (covariance(parID, parID) > 0) {
        float pull = residual / sqrt(covariance(parID, parID));
        PlotHelpers::fillHisto(resPlotCache.pull[parName], pull);
        PlotHelpers::fillHisto(resPlotCache.pull_vs_eta.at(parName), truthEta,
                               pull);
        PlotHelpers::fillHisto(resPlotCache.pull_vs_pT.at(parName), truthPt,
                               pull);
      } else {
        ACTS_WARNING("Fitted track parameter :" << parName
                                                << " has negative covariance = "
                                                << covariance(parID, parID));
      }
    } else {
      ACTS_WARNING("Fitted track parameter :" << parName
                                              << " has no covariance");
    }
  }
}

// get the mean and width of residual/pull in each eta/pT bin and fill them into
// histograms
void ActsExamples::ResPlotTool::refinement(
    ResPlotTool::ResPlotCache& resPlotCache) const {
  PlotHelpers::Binning bEta = m_cfg.varBinning.at("Eta");
  PlotHelpers::Binning bPt = m_cfg.varBinning.at("Pt");
  for (unsigned int parID = 0; parID < Acts::eBoundSize; parID++) {
    std::string parName = m_cfg.paramNames.at(parID);
    // refine the plots vs eta
    for (int j = 1; j <= static_cast<int>(bEta.nBins()); j++) {
      TH1D* temp_res = resPlotCache.res_vs_eta.at(parName)->ProjectionY(
          Form("%s_projy_bin%d", "Residual_vs_eta_Histo", j), j, j);
      PlotHelpers::anaHisto(temp_res, j,
                            resPlotCache.resMean_vs_eta.at(parName),
                            resPlotCache.resWidth_vs_eta.at(parName));

      TH1D* temp_pull = resPlotCache.pull_vs_eta.at(parName)->ProjectionY(
          Form("%s_projy_bin%d", "Pull_vs_eta_Histo", j), j, j);
      PlotHelpers::anaHisto(temp_pull, j,
                            resPlotCache.pullMean_vs_eta.at(parName),
                            resPlotCache.pullWidth_vs_eta.at(parName));
    }

    // refine the plots vs pT
    for (int j = 1; j <= static_cast<int>(bPt.nBins()); j++) {
      TH1D* temp_res = resPlotCache.res_vs_pT.at(parName)->ProjectionY(
          Form("%s_projy_bin%d", "Residual_vs_pT_Histo", j), j, j);
      PlotHelpers::anaHisto(temp_res, j, resPlotCache.resMean_vs_pT.at(parName),
                            resPlotCache.resWidth_vs_pT.at(parName));

      TH1D* temp_pull = resPlotCache.pull_vs_pT.at(parName)->ProjectionY(
          Form("%s_projy_bin%d", "Pull_vs_pT_Histo", j), j, j);
      PlotHelpers::anaHisto(temp_pull, j,
                            resPlotCache.pullMean_vs_pT.at(parName),
                            resPlotCache.pullWidth_vs_pT.at(parName));
    }
  }
}
