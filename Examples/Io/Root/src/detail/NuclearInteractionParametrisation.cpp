// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Io/Root/detail/NuclearInteractionParametrisation.hpp"

#include "Acts/Definitions/Common.hpp"

#include <cmath>
#include <iterator>
#include <limits>

#include <Eigen/Eigenvalues>
#include <TMath.h>
#include <TVectorF.h>
#include <TVectorT.h>

namespace ActsExamples::detail::NuclearInteractionParametrisation {

namespace {

/// @brief Evaluate the location in a standard normal distribution for a value
/// from a probability distribution
///
/// @param [in] histo The probability distribution
/// @param [in] mom The abscissa value in @p histo
///
/// @return The location in a standard normal distribution
float gaussianValue(TH1F const* histo, const float mom) {
  // Get the cumulative probability distribution

  // DrawNormalized and GetCumulative return pointers to histograms where ROOT
  // transfers ownership to the caller.
  std::unique_ptr<TH1F> normalised(
      dynamic_cast<TH1F*>(histo->DrawNormalized()));
  std::unique_ptr<TH1F> cumulative(
      dynamic_cast<TH1F*>(normalised->GetCumulative()));
  assert(cumulative);
  assert(normalised);
  // Find the cumulative probability
  const float binContent = cumulative->GetBinContent(cumulative->FindBin(mom));
  // Transform the probability to an entry in a standard normal distribution
  const float value = TMath::ErfInverse(2. * binContent - 1.);

  return value;
}

/// @brief Evaluate the invariant mass of two four vectors
///
/// @param [in] fourVector1 The one four vector
/// @param [in] fourVector2 The other four vector
///
/// @return The invariant mass
float invariantMass(const Acts::Vector4& fourVector1,
                    const Acts::Vector4& fourVector2) {
  Acts::Vector4 sum = fourVector1 + fourVector2;
  const double energy = sum[Acts::eEnergy];
  double momentum = sum.template segment<3>(Acts::eMom0).norm();
  return std::sqrt(energy * energy - momentum * momentum);
}

}  // namespace

std::pair<Vector, Matrix> calculateMeanAndCovariance(
    unsigned int multiplicity, const EventProperties& events) {
  // Calculate the mean
  Vector mean = Vector::Zero(multiplicity);
  for (const std::vector<float>& event : events) {
    for (unsigned int j = 0; j < multiplicity; j++) {
      mean[j] += event[j];
    }
  }
  mean /= events.size();

  // Calculate the covariance matrix
  Matrix covariance = Matrix::Zero(multiplicity, multiplicity);
  for (unsigned int i = 0; i < multiplicity; i++) {
    for (unsigned int j = 0; j < multiplicity; j++) {
      for (unsigned int k = 0; k < events.size(); k++) {
        covariance(i, j) += (events[k][i] - mean[i]) * (events[k][j] - mean[j]);
      }
    }
  }
  covariance /= events.size();

  return {mean, covariance};
}

EigenspaceComponents calculateEigenspace(const Vector& mean,
                                         const Matrix& covariance) {
  // Calculate eigenvalues and eigenvectors
  Eigen::EigenSolver<Matrix> es(covariance);
  Vector eigenvalues = es.eigenvalues().real();
  Matrix eigenvectors = es.eigenvectors().real();
  // Transform the mean vector into eigenspace
  Vector meanEigenspace = eigenvectors * mean;

  return {eigenvalues, eigenvectors, meanEigenspace};
}

Parametrisation buildMomentumParameters(const EventCollection& events,
                                        unsigned int multiplicity, bool soft,
                                        unsigned int nBins) {
  // Strip off data
  auto momenta = prepareMomenta(events, multiplicity, soft);
  if (momenta.empty()) {
    return Parametrisation();
  }

  // Build histos
  ProbabilityDistributions histos = buildMomPerMult(momenta, nBins);

  // Build normal distribution
  auto momentaGaussian = convertEventToGaussian(histos, momenta);
  auto meanAndCovariance =
      calculateMeanAndCovariance(multiplicity + 1, momentaGaussian);
  // Calculate the transformation into the eigenspace of the covariance matrix
  EigenspaceComponents eigenspaceElements =
      calculateEigenspace(meanAndCovariance.first, meanAndCovariance.second);
  // Calculate the cumulative distributions
  return {eigenspaceElements, histos};
}

EventProperties prepareMomenta(const EventCollection& events,
                               unsigned int multiplicity,
                               bool soft)  // TODO: build enum instead of bool
{
  EventProperties result;
  // Loop over all events
  for (const EventFraction& event : events) {
    // Test the multiplicity and type of the event
    if (event.multiplicity == multiplicity && event.soft == soft) {
      const float initialMomentum = event.initialParticle.absoluteMomentum();
      float sum = 0.;
      std::vector<float> momenta;
      momenta.reserve(multiplicity + 1);
      // Fill the vector with the scaled momenta
      for (const ActsExamples::SimParticle& p : event.finalParticles) {
        sum += p.absoluteMomentum();
        momenta.push_back(p.absoluteMomentum() / initialMomentum);
      }
      // Add the scaled sum of momenta
      momenta.push_back(sum / initialMomentum);
      result.push_back(std::move(momenta));
    }
  }
  return result;
}

ProbabilityDistributions buildMomPerMult(const EventProperties& events,
                                         unsigned int nBins) {
  // Fast exit
  if (events.empty()) {
    return {};
  }
  const unsigned int multMax = events[0].size();

  // Find the range of each histogram
  std::vector<float> min(multMax, std::numeric_limits<float>::max());
  std::vector<float> max(multMax, 0);
  for (const std::vector<float>& event : events) {
    for (unsigned int i = 0; i < multMax; i++) {
      min[i] = std::min(event[i], min[i]);
      max[i] = std::max(event[i], max[i]);
    }
  }

  // Evaluate the range of the histograms
  // This is used to avoid entries in over-/underflow bins
  std::vector<float> diff(multMax);
  for (unsigned int i = 0; i < multMax; i++) {
    diff[i] = (max[i] - min[i]) * 0.1;
  }

  // Build the histograms
  ProbabilityDistributions histos(multMax);
  for (unsigned int i = 0; i < multMax; i++) {
    histos[i] = new TH1F("", "", nBins, min[i] - diff[i], max[i] + diff[i]);
  }

  // Fill the histograms
  for (const std::vector<float>& event : events) {
    for (unsigned int i = 0; i < multMax; i++) {
      histos[i]->Fill(event[i]);
    }
  }
  return histos;
}

EventProperties convertEventToGaussian(const ProbabilityDistributions& histos,
                                       const EventProperties& events) {
  // Fast exit
  if (events.empty()) {
    return {};
  }
  const unsigned int multMax = events[0].size();

  // Loop over the events
  EventProperties gaussianEvents;
  for (const std::vector<float>& event : events) {
    // Transform the properties in the events
    std::vector<float> gaussianEvent;
    for (unsigned int i = 0; i < multMax; i++) {
      gaussianEvent.push_back(gaussianValue(histos[i], event[i]));
    }
    // Store the transformed event
    gaussianEvents.push_back(gaussianEvent);
  }
  return gaussianEvents;
}

EventProperties prepareInvariantMasses(const EventCollection& events,
                                       unsigned int multiplicity, bool soft) {
  EventProperties result;
  // Loop over all events
  for (const EventFraction& event : events) {
    // Test the multiplicity and type of the event
    if (event.multiplicity == multiplicity && event.soft == soft) {
      const auto fourVectorBefore = event.interactingParticle.fourMomentum();
      std::vector<float> invariantMasses;
      invariantMasses.reserve(multiplicity);
      // Fill the vector with the invariant masses
      for (const ActsExamples::SimParticle& p : event.finalParticles) {
        const auto fourVector = p.fourMomentum();
        invariantMasses.push_back(invariantMass(fourVectorBefore, fourVector));
      }
      result.push_back(invariantMasses);
    }
  }
  return result;
}

Parametrisation buildInvariantMassParameters(const EventCollection& events,
                                             unsigned int multiplicity,
                                             bool soft, unsigned int nBins) {
  // Strip off data
  auto invariantMasses = prepareInvariantMasses(events, multiplicity, soft);
  if (invariantMasses.empty()) {
    return Parametrisation();
  }

  // Build histos
  ProbabilityDistributions histos = buildMomPerMult(invariantMasses, nBins);

  // Build normal distribution
  auto invariantMassesGaussian =
      convertEventToGaussian(histos, invariantMasses);
  auto meanAndCovariance =
      calculateMeanAndCovariance(multiplicity, invariantMassesGaussian);
  // Calculate the transformation into the eigenspace of the covariance matrix
  EigenspaceComponents eigenspaceElements =
      calculateEigenspace(meanAndCovariance.first, meanAndCovariance.second);
  // Calculate the cumulative distributions
  return {eigenspaceElements, histos};
}

std::unordered_map<int, std::unordered_map<int, float>>
cumulativePDGprobability(const EventCollection& events) {
  std::unordered_map<int, std::unordered_map<int, float>> counter;

  // Count how many and which particles were created by which particle
  for (const EventFraction& event : events) {
    if (event.finalParticles.empty()) {
      continue;
    }
    if (!event.soft) {
      counter[event.initialParticle.pdg()][event.finalParticles[0].pdg()]++;
    }
    for (unsigned int i = 1; i < event.multiplicity; i++) {
      counter[event.finalParticles[i - 1].pdg()]
             [event.finalParticles[i].pdg()]++;
    }
  }

  // Build a cumulative distribution
  for (const auto& element : counter) {
    float sum = 0;
    auto prevIt = counter[element.first].begin();
    for (auto it1 = counter[element.first].begin();
         it1 != counter[element.first].end(); it1++) {
      float binEntry = 0;
      if (it1 == counter[element.first].begin()) {
        binEntry = it1->second;
        prevIt = it1;
      } else {
        binEntry = it1->second - prevIt->second;
        prevIt = it1;
      }
      // Add content to next bins
      for (auto it2 = std::next(it1, 1); it2 != counter[element.first].end();
           it2++) {
        it2->second += binEntry;
        sum = it2->second;
      }
    }
    // Normalise the entry
    for (auto it1 = counter[element.first].begin();
         it1 != counter[element.first].end(); it1++) {
      it1->second /= sum;
    }
  }
  return counter;
}

std::pair<CumulativeDistribution, CumulativeDistribution>
cumulativeMultiplicityProbability(const EventCollection& events,
                                  unsigned int multiplicityMax) {
  // Find the range of both histogram
  unsigned int minSoft = std::numeric_limits<unsigned int>::max();
  unsigned int maxSoft = 0;
  unsigned int minHard = std::numeric_limits<unsigned int>::max();
  unsigned int maxHard = 0;
  for (const EventFraction& event : events) {
    if (event.soft) {
      minSoft = std::min(event.multiplicity, minSoft);
      maxSoft = std::max(event.multiplicity, maxSoft);
    } else {
      minHard = std::min(event.multiplicity, minHard);
      maxHard = std::max(event.multiplicity, maxHard);
    }
  }

  // Build and fill the histograms
  TH1F* softHisto =
      new TH1F("", "", std::min(maxSoft, multiplicityMax) + 1 - minSoft,
               minSoft, std::min(maxSoft, multiplicityMax) + 1);
  TH1F* hardHisto =
      new TH1F("", "", std::min(maxHard, multiplicityMax) + 1 - minHard,
               minHard, std::min(maxHard, multiplicityMax) + 1);
  for (const EventFraction& event : events) {
    if (event.multiplicity <= multiplicityMax) {
      if (event.soft) {
        softHisto->Fill(event.multiplicity);
      } else {
        hardHisto->Fill(event.multiplicity);
      }
    }
  }

  return {softHisto, hardHisto};
}

TVectorF softProbability(const EventCollection& events) {
  std::size_t countSoft = 0;
  // Count the soft events
  for (const EventFraction& event : events) {
    if (event.soft) {
      countSoft++;
    }
  }

  TVectorF result(1);
  result[0] = static_cast<float>(countSoft) / events.size();
  return result;
}

CumulativeDistribution cumulativeNuclearInteractionProbability(
    const EventCollection& events, unsigned int interactionProbabilityBins) {
  // Find the limits of the histogram
  float min = std::numeric_limits<float>::max();
  float max = 0.;
  for (const EventFraction& event : events) {
    min =
        std::min(static_cast<float>(event.interactingParticle.pathInL0()), min);
    max =
        std::max(static_cast<float>(event.interactingParticle.pathInL0()), max);
  }

  // Fill the histogram
  TH1F* histo = new TH1F("", "", interactionProbabilityBins, min, max);
  for (const EventFraction& event : events) {
    histo->Fill(event.interactingParticle.pathInL0());
  }

  // Build the distributions
  return histo;  // TODO: in this case the normalisation is not taking into
                 // account
}

}  // namespace ActsExamples::detail::NuclearInteractionParametrisation
