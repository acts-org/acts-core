// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsExamples/Utilities/Helpers.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

class TH1F;
class TH2F;
namespace ActsFatras {
class Particle;
}  // namespace ActsFatras

namespace ActsExamples {

// Tools to make hists to show residual, i.e. smoothed_parameter -
// truth_parameter, and pull, i.e. (smoothed_parameter -
// truth_parameter)/smoothed_paramter_error, of track parameters at perigee
// surface
class ResPlotTool {
 public:
  /// @brief Nested configuration struct
  struct Config {
    /// parameter sets to do plots
    std::vector<std::string> paramNames = {"d0",    "z0",  "phi",
                                           "theta", "qop", "t"};

    /// Binning info for variables
    std::map<std::string, PlotHelpers::Binning> varBinning = {
        {"Eta", PlotHelpers::Binning("#eta", 40, -4, 4)},
        {"Pt", PlotHelpers::Binning("pT [GeV/c]", 40, 0, 100)},
        {"Pull", PlotHelpers::Binning("pull", 100, -5, 5)},
        {"Residual_d0", PlotHelpers::Binning("r_{d0} [mm]", 100, -0.5, 0.5)},
        {"Residual_z0", PlotHelpers::Binning("r_{z0} [mm]", 100, -0.5, 0.5)},
        {"Residual_phi",
         PlotHelpers::Binning("r_{#phi} [rad]", 100, -0.01, 0.01)},
        {"Residual_theta",
         PlotHelpers::Binning("r_{#theta} [rad]", 100, -0.01, 0.01)},
        {"Residual_qop",
         PlotHelpers::Binning("r_{q/p} [c/GeV]", 100, -0.1, 0.1)},
        {"Residual_t", PlotHelpers::Binning("r_{t} [s]", 100, -1000, 1000)}};
  };

  /// @brief Nested Cache struct
  struct ResPlotCache {
    std::map<std::string, TH1F*> res;         ///< Residual distribution
    std::map<std::string, TH2F*> res_vs_eta;  ///< Residual vs eta scatter plot
    std::map<std::string, TH1F*>
        resMean_vs_eta;  ///< Residual mean vs eta distribution
    std::map<std::string, TH1F*>
        resWidth_vs_eta;  ///< Residual width vs eta distribution
    std::map<std::string, TH2F*> res_vs_pT;  ///< Residual vs pT scatter plot
    std::map<std::string, TH1F*>
        resMean_vs_pT;  ///< Residual mean vs pT distribution
    std::map<std::string, TH1F*>
        resWidth_vs_pT;  ///< Residual width vs pT distribution

    std::map<std::string, TH1F*> pull;         ///< Pull distribution
    std::map<std::string, TH2F*> pull_vs_eta;  ///< Pull vs eta scatter plot
    std::map<std::string, TH1F*>
        pullMean_vs_eta;  ///< Pull mean vs eta distribution
    std::map<std::string, TH1F*>
        pullWidth_vs_eta;  ///< Pull width vs eta distribution
    std::map<std::string, TH2F*> pull_vs_pT;  ///< Pull vs pT scatter plot
    std::map<std::string, TH1F*>
        pullMean_vs_pT;  ///< Pull mean vs pT distribution
    std::map<std::string, TH1F*>
        pullWidth_vs_pT;  ///< Pull width vs pT distribution
  };

  /// Constructor
  ///
  /// @param cfg Configuration struct
  /// @param level Message level declaration
  ResPlotTool(const Config& cfg, Acts::Logging::Level lvl);

  /// @brief book the histograms
  ///
  /// @param resPlotCache the cache for residual/pull histograms
  void book(ResPlotCache& resPlotCache) const;

  /// @brief fill the histograms
  ///
  /// @param resPlotCache the cache for residual/pull histograms
  /// @param gctx the geometry context
  /// @param truthParticle the truth particle
  /// @param fittedParamters the fitted parameters at perigee surface
  void fill(ResPlotCache& resPlotCache, const Acts::GeometryContext& gctx,
            const SimParticleState& truthParticle,
            const Acts::BoundTrackParameters& fittedParamters) const;

  /// @brief extract the details of the residual/pull plots and fill details
  ///
  /// into separate histograms
  /// @param resPlotCache the cache object for residual/pull histograms
  void refinement(ResPlotCache& resPlotCache) const;

  /// @brief write the histograms to output file
  ///
  /// @param resPlotCache the cache object for residual/pull histograms
  void write(const ResPlotCache& resPlotCache) const;

  /// @brief delete the histograms
  ///
  /// @param resPlotCache the cache object for residual/pull histograms
  void clear(ResPlotCache& resPlotCache) const;

 private:
  Config m_cfg;                                  ///< The config class
  std::unique_ptr<const Acts::Logger> m_logger;  ///< The logging instance

  /// The logger
  const Acts::Logger& logger() const { return *m_logger; }
};

}  // namespace ActsExamples
