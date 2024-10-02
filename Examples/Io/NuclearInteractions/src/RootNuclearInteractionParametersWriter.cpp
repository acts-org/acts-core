// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "ActsExamples/Io/NuclearInteractions/RootNuclearInteractionParametersWriter.hpp"

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/Definitions/Common.hpp"
#include "ActsExamples/EventData/SimParticle.hpp"
#include "ActsFatras/EventData/Particle.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <TAxis.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TH1.h>
#include <TVectorT.h>

namespace ActsExamples {
struct AlgorithmContext;
}  // namespace ActsExamples

namespace {

/// @brief This method labels events as either soft or hard and sets the
/// multiplicity
///
/// @param [in] events The events that will be labeled
void labelEvents(
    std::vector<
        ActsExamples::detail::NuclearInteractionParametrisation::EventFraction>&
        events) {
  namespace Parametrisation =
      ActsExamples::detail::NuclearInteractionParametrisation;
  // Search for the highest momentum particles per event
  for (Parametrisation::EventFraction& event : events) {
    double maxMom = 0.;
    double maxMomOthers = 0.;
    // Walk over all final state particles
    for (const ActsExamples::SimParticle& p : event.finalParticles) {
      // Search for the maximum in particles with the same PDG ID as the
      // interacting one
      if (p.pdg() == event.initialParticle.pdg()) {
        maxMom = std::max(p.absoluteMomentum(), maxMom);
        // And the maximum among the others
      } else {
        maxMomOthers = std::max(p.absoluteMomentum(), maxMomOthers);
      }
    }

    // Label the initial momentum
    event.initialMomentum = event.initialParticle.absoluteMomentum();

    // Label the indication that the interacting particle carries most of the
    // momentum
    event.soft = (maxMom > maxMomOthers);

    // Get the final state p_T
    double pt = 0.;
    Acts::Vector2 ptVec(0., 0.);
    for (const ActsExamples::SimParticle& p : event.finalParticles) {
      Acts::Vector2 particlePt =
          p.fourMomentum().template segment<2>(Acts::eMom0);
      ptVec[0] += particlePt[0];
      ptVec[1] += particlePt[1];
    }
    pt = ptVec.norm();

    // Use the final state p_T as veto for the soft label
    if (event.soft && pt <= event.interactingParticle.transverseMomentum()) {
      event.soft = false;
    }

    // Store the multiplicity
    event.multiplicity = event.finalParticles.size();
  }
}

/// @brief This method builds components for transforming a probability
/// distribution into components that represent the cumulative probability
/// distribution
///
/// @param [in] hist The probability distribution
///
/// @return Tuple containing the bin borders, the non-normalised cumulative
/// distribution and the sum over all entries
std::tuple<std::vector<float>, std::vector<double>, double>
buildNotNormalisedMap(TH1F const* hist) {
  // Retrieve the number of bins & borders
  const int nBins = hist->GetNbinsX();
  std::vector<float> histoBorders(nBins + 1);

  // Fill the cumulative histogram
  double integral = 0.;
  std::vector<double> temp_HistoContents(nBins);
  for (int iBin = 0; iBin < nBins; iBin++) {
    float binval = hist->GetBinContent(iBin + 1);
    // Avoid negative bin values
    if (binval < 0) {
      binval = 0.;
    }
    // Store the value
    integral += binval;
    temp_HistoContents[iBin] = integral;
  }

  // Ensure that content is available
  if (integral == 0.) {
    histoBorders.clear();
    temp_HistoContents.clear();
    return std::make_tuple(histoBorders, temp_HistoContents, integral);
  }

  // Set the bin borders
  for (int iBin = 1; iBin <= nBins; iBin++) {
    histoBorders[iBin - 1] = hist->GetXaxis()->GetBinLowEdge(iBin);
  }
  histoBorders[nBins] = hist->GetXaxis()->GetXmax();

  return std::make_tuple(histoBorders, temp_HistoContents, integral);
}

/// @brief This function combines neighbouring bins with the same value
///
/// @param [in, out] histoBorders The borders of the bins
/// @param [in, out] histoContents The content of each bin
void reduceMap(std::vector<float>& histoBorders,
               std::vector<std::uint32_t>& histoContents) {
  for (auto cit = histoContents.cbegin(); cit != histoContents.cend(); cit++) {
    while (std::next(cit, 1) != histoContents.end() &&
           *cit == *std::next(cit, 1)) {
      const auto distance =
          std::distance(histoContents.cbegin(), std::next(cit, 1));
      // Remove the bin
      histoBorders.erase(histoBorders.begin() + distance);
      histoContents.erase(histoContents.begin() + distance);
    }
  }
}

/// @brief This method transforms a probability distribution into components
/// that represent the cumulative probability distribution
///
/// @note This method is used to avoid Root dependencies afterwards by
/// decomposing the histogram
/// @param [in] hist The probability distribution
///
/// @return Pair containing the bin borders and the bin content
std::pair<std::vector<float>, std::vector<std::uint32_t>> buildMap(
    TH1F const* hist) {
  // Build the components
  std::tuple<std::vector<float>, std::vector<double>, double> map =
      buildNotNormalisedMap(hist);
  const int nBins = hist->GetNbinsX();
  std::vector<double>& histoContents = std::get<1>(map);

  // Fast exit if the histogram is empty
  if (histoContents.empty()) {
    return std::make_pair(std::get<0>(map), std::vector<std::uint32_t>());
  }

  // Set the bin content
  std::vector<std::uint32_t> normalisedHistoContents(nBins);
  const double invIntegral = 1. / std::get<2>(map);
  for (int iBin = 0; iBin < nBins; ++iBin) {
    normalisedHistoContents[iBin] =
        static_cast<unsigned int>(std::numeric_limits<std::uint32_t>::max() *
                                  (histoContents[iBin] * invIntegral));
  }

  auto histoBorders = std::get<0>(map);
  reduceMap(histoBorders, normalisedHistoContents);
  return std::make_pair(histoBorders, normalisedHistoContents);
}

/// @brief This method transforms a probability distribution into components
/// that represent the cumulative probability distribution
///
/// @note This method is used to avoid Root dependencies afterwards by
/// decomposing the histogram
/// @param [in] hist The probability distribution
/// @param [in] integral Scaling factor of the distribution
/// @note If @p integral is less than the actual integral of the histogram then
/// the latter is used.
///
/// @return Pair containing the bin borders and the bin content
std::pair<std::vector<float>, std::vector<std::uint32_t>> buildMap(
    TH1F const* hist, double integral) {
  // Build the components
  std::tuple<std::vector<float>, std::vector<double>, double> map =
      buildNotNormalisedMap(hist);

  const int nBins = hist->GetNbinsX();
  std::vector<double>& histoContents = std::get<1>(map);

  // Fast exit if the histogram is empty
  if (histoContents.empty()) {
    return std::make_pair(std::get<0>(map), std::vector<std::uint32_t>());
  }

  // Set the bin content
  std::vector<std::uint32_t> normalisedHistoContents(nBins);
  const double invIntegral = 1. / std::max(integral, std::get<2>(map));
  for (int iBin = 0; iBin < nBins; ++iBin) {
    normalisedHistoContents[iBin] =
        static_cast<unsigned int>(std::numeric_limits<std::uint32_t>::max() *
                                  (histoContents[iBin] * invIntegral));
  }

  std::vector<float> histoBorders = std::get<0>(map);
  reduceMap(histoBorders, normalisedHistoContents);

  return std::make_pair(histoBorders, normalisedHistoContents);
}

/// @brief This method builds decomposed cumulative probability distributions
/// out of a vector of proability distributions
///
/// @param [in] histos Vector of probability distributions
///
/// @return Vector containing the decomposed cumulative probability
/// distributions
std::vector<std::pair<std::vector<float>, std::vector<std::uint32_t>>>
buildMaps(const std::vector<TH1F*>& histos) {
  std::vector<std::pair<std::vector<float>, std::vector<std::uint32_t>>> maps;
  for (auto& h : histos) {
    maps.push_back(buildMap(h));
  }
  return maps;
}

/// @brief This method parametrises and stores recursively a parametrisation of
/// the final state kinematics in a nuclear interaction
///
/// @param [in] eventFractionCollection The event storage
/// @param [in] interactionType The interaction type that will be parametrised
/// @param [in] cfg Configuration that steers the binning of histograms
inline void recordKinematicParametrisation(
    const std::vector<
        ActsExamples::detail::NuclearInteractionParametrisation::EventFraction>&
        eventFractionCollection,
    bool interactionType, unsigned int multiplicity,
    const ActsExamples::RootNuclearInteractionParametersWriter::Config& cfg) {
  namespace Parametrisation =
      ActsExamples::detail::NuclearInteractionParametrisation;
  gDirectory->mkdir(std::to_string(multiplicity).c_str());
  gDirectory->cd(std::to_string(multiplicity).c_str());

  // Parametrise the momentum und invarian mass distributions
  const auto momentumParameters = Parametrisation::buildMomentumParameters(
      eventFractionCollection, multiplicity, interactionType, cfg.momentumBins);
  std::vector<Parametrisation::CumulativeDistribution> distributionsMom =
      momentumParameters.second;

  const auto invariantMassParameters =
      Parametrisation::buildInvariantMassParameters(
          eventFractionCollection, multiplicity, interactionType,
          cfg.invariantMassBins);
  std::vector<Parametrisation::CumulativeDistribution> distributionsInvMass =
      invariantMassParameters.second;

  // Fast exit in case of no events
  if (!distributionsMom.empty() && !distributionsInvMass.empty()) {
    if (multiplicity > 1) {
      // Write the eigenspace components for the momenta
      Parametrisation::EigenspaceComponents esComponentsMom =
          momentumParameters.first;

      auto momEigenVal = std::get<0>(esComponentsMom);
      auto momEigenVec = std::get<1>(esComponentsMom);
      auto momMean = std::get<2>(esComponentsMom);
      std::vector<float> momVecVal(momEigenVal.data(),
                                   momEigenVal.data() + momEigenVal.size());
      std::vector<float> momVecVec(momEigenVec.data(),
                                   momEigenVec.data() + momEigenVec.size());
      std::vector<float> momVecMean(momMean.data(),
                                    momMean.data() + momMean.size());
      gDirectory->WriteObject(&momVecVal, "MomentumEigenvalues");
      gDirectory->WriteObject(&momVecVec, "MomentumEigenvectors");
      gDirectory->WriteObject(&momVecMean, "MomentumMean");

      // Write the eigenspace components for the invariant masses
      Parametrisation::EigenspaceComponents esComponentsInvMass =
          invariantMassParameters.first;

      auto invMassEigenVal = std::get<0>(esComponentsInvMass);
      auto invMassEigenVec = std::get<1>(esComponentsInvMass);
      auto invMassMean = std::get<2>(esComponentsInvMass);
      std::vector<float> invMassVecVal(
          invMassEigenVal.data(),
          invMassEigenVal.data() + invMassEigenVal.size());
      std::vector<float> invMassVecVec(
          invMassEigenVec.data(),
          invMassEigenVec.data() + invMassEigenVec.size());
      std::vector<float> invMassVecMean(
          invMassMean.data(), invMassMean.data() + invMassMean.size());
      gDirectory->WriteObject(&invMassVecVal, "InvariantMassEigenvalues");
      gDirectory->WriteObject(&invMassVecVec, "InvariantMassEigenvectors");
      gDirectory->WriteObject(&invMassVecMean, "InvariantMassMean");
    }

    const auto momDistributions = buildMaps(distributionsMom);
    const auto invMassDistributions = buildMaps(distributionsInvMass);

    // Write the distributions
    for (unsigned int i = 0; i <= multiplicity; i++) {
      if (cfg.writeOptionalHistograms) {
        gDirectory->WriteObject(
            distributionsMom[i],
            ("MomentumDistributionHistogram_" + std::to_string(i)).c_str());
      }
      gDirectory->WriteObject(
          &momDistributions[i].first,
          ("MomentumDistributionBinBorders_" + std::to_string(i)).c_str());
      gDirectory->WriteObject(
          &momDistributions[i].second,
          ("MomentumDistributionBinContents_" + std::to_string(i)).c_str());
    }
    for (unsigned int i = 0; i < multiplicity; i++) {
      if (cfg.writeOptionalHistograms) {
        gDirectory->WriteObject(
            distributionsInvMass[i],
            ("InvariantMassDistributionHistogram_" + std::to_string(i))
                .c_str());
      }
      gDirectory->WriteObject(
          &invMassDistributions[i].first,
          ("InvariantMassDistributionBinBorders_" + std::to_string(i)).c_str());
      gDirectory->WriteObject(
          &invMassDistributions[i].second,
          ("InvariantMassDistributionBinContents_" + std::to_string(i))
              .c_str());
    }
  }
  gDirectory->cd("..");
}
}  // namespace

ActsExamples::RootNuclearInteractionParametersWriter::
    RootNuclearInteractionParametersWriter(
        const ActsExamples::RootNuclearInteractionParametersWriter::Config&
            config,
        Acts::Logging::Level level)
    : WriterT(config.inputSimulationProcesses,
              "RootNuclearInteractionParametersWriter", level),
      m_cfg(config) {
  if (m_cfg.inputSimulationProcesses.empty()) {
    throw std::invalid_argument("Missing input collection");
  }
  if (m_cfg.filePath.empty()) {
    throw std::invalid_argument("Missing output filename for parameters");
  }
}

ActsExamples::RootNuclearInteractionParametersWriter::
    ~RootNuclearInteractionParametersWriter() = default;

ActsExamples::ProcessCode
ActsExamples::RootNuclearInteractionParametersWriter::finalize() {
  namespace Parametrisation = detail::NuclearInteractionParametrisation;
  if (m_eventFractionCollection.empty()) {
    return ProcessCode::ABORT;
  }

  // Exclusive access to the file while writing
  std::lock_guard<std::mutex> lock(m_writeMutex);

  // The file
  TFile* tf = TFile::Open(m_cfg.filePath.c_str(), m_cfg.fileMode.c_str());
  gDirectory->cd();
  gDirectory->mkdir(
      std::to_string(m_eventFractionCollection[0].initialParticle.pdg())
          .c_str());
  gDirectory->cd(
      std::to_string(m_eventFractionCollection[0].initialParticle.pdg())
          .c_str());
  gDirectory->mkdir(
      std::to_string(m_eventFractionCollection[0].initialMomentum).c_str());
  gDirectory->cd(
      std::to_string(m_eventFractionCollection[0].initialMomentum).c_str());
  gDirectory->mkdir("soft");
  gDirectory->mkdir("hard");

  // Write the nuclear interaction probability
  ACTS_DEBUG("Starting parametrisation of nuclear interaction probability");
  const auto nuclearInteractionProbability =
      Parametrisation::cumulativeNuclearInteractionProbability(
          m_eventFractionCollection, m_cfg.interactionProbabilityBins);

  if (m_cfg.writeOptionalHistograms) {
    gDirectory->WriteObject(nuclearInteractionProbability,
                            "NuclearInteractionHistogram");
  }
  const auto mapNIprob =
      buildMap(nuclearInteractionProbability, m_cfg.nSimulatedEvents);
  gDirectory->WriteObject(&mapNIprob.first, "NuclearInteractionBinBorders");
  gDirectory->WriteObject(&mapNIprob.second, "NuclearInteractionBinContents");
  ACTS_DEBUG("Nuclear interaction probability parametrised");

  ACTS_DEBUG("Starting calulcation of probability of interaction type");
  // Write the interaction type proability
  const auto softProbability =
      Parametrisation::softProbability(m_eventFractionCollection);

  gDirectory->WriteObject(&softProbability, "SoftInteraction");
  ACTS_DEBUG("Calulcation of probability of interaction type finished");

  // Write the PDG id production distribution
  ACTS_DEBUG(
      "Starting calulcation of transition probabilities between PDG IDs");
  const auto pdgIdMap =
      Parametrisation::cumulativePDGprobability(m_eventFractionCollection);
  std::vector<int> branchingPdgIds;
  std::vector<int> targetPdgIds;
  std::vector<float> targetPdgProbability;
  for (const auto& targetPdgIdMap : pdgIdMap) {
    for (const auto& producedPdgIdMap : targetPdgIdMap.second) {
      branchingPdgIds.push_back(targetPdgIdMap.first);
      targetPdgIds.push_back(producedPdgIdMap.first);
      targetPdgProbability.push_back(producedPdgIdMap.second);
    }
  }

  gDirectory->WriteObject(&branchingPdgIds, "BranchingPdgIds");
  gDirectory->WriteObject(&targetPdgIds, "TargetPdgIds");
  gDirectory->WriteObject(&targetPdgProbability, "TargetPdgProbability");
  ACTS_DEBUG(
      "Calulcation of transition probabilities between PDG IDs finished");

  // Write the multiplicity and kinematics distribution
  ACTS_DEBUG("Starting parametrisation of multiplicity probabilities");
  const auto multiplicity = Parametrisation::cumulativeMultiplicityProbability(
      m_eventFractionCollection, m_cfg.multiplicityMax);
  ACTS_DEBUG("Parametrisation of multiplicity probabilities finished");

  gDirectory->cd("soft");
  if (m_cfg.writeOptionalHistograms) {
    gDirectory->WriteObject(multiplicity.first, "MultiplicityHistogram");
  }
  const auto multProbSoft = buildMap(multiplicity.first);
  gDirectory->WriteObject(&multProbSoft.first, "MultiplicityBinBorders");
  gDirectory->WriteObject(&multProbSoft.second, "MultiplicityBinContents");
  for (unsigned int i = 1; i <= m_cfg.multiplicityMax; i++) {
    ACTS_DEBUG("Starting parametrisation of final state kinematics for soft " +
               std::to_string(i) + " particle(s) final state");
    recordKinematicParametrisation(m_eventFractionCollection, true, i, m_cfg);
    ACTS_DEBUG("Parametrisation of final state kinematics for soft " +
               std::to_string(i) + " particle(s) final state finished");
  }
  gDirectory->cd("../hard");
  if (m_cfg.writeOptionalHistograms) {
    gDirectory->WriteObject(multiplicity.second, "MultiplicityHistogram");
  }
  const auto multProbHard = buildMap(multiplicity.second);
  gDirectory->WriteObject(&multProbHard.first, "MultiplicityBinBorders");
  gDirectory->WriteObject(&multProbHard.second, "MultiplicityBinContents");

  for (unsigned int i = 1; i <= m_cfg.multiplicityMax; i++) {
    ACTS_DEBUG("Starting parametrisation of final state kinematics for hard " +
               std::to_string(i) + " particle(s) final state");
    recordKinematicParametrisation(m_eventFractionCollection, false, i, m_cfg);
    ACTS_DEBUG("Parametrisation of final state kinematics for hard " +
               std::to_string(i) + " particle(s) final state finished");
  }
  gDirectory->cd();
  tf->Write();
  tf->Close();
  return ProcessCode::SUCCESS;
}

ActsExamples::ProcessCode
ActsExamples::RootNuclearInteractionParametersWriter::writeT(
    const AlgorithmContext& /*ctx*/,
    const ExtractedSimulationProcessContainer& event) {
  // Convert the tuple to use additional categorisation variables
  std::vector<detail::NuclearInteractionParametrisation::EventFraction>
      eventFractions;
  eventFractions.reserve(event.size());
  for (const auto& e : event) {
    eventFractions.emplace_back(e);
  }
  // Categorise the event
  labelEvents(eventFractions);
  // Store the event
  m_eventFractionCollection.insert(m_eventFractionCollection.end(),
                                   eventFractions.begin(),
                                   eventFractions.end());

  return ProcessCode::SUCCESS;
}
