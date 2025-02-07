// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/PdgParticle.hpp"
#include "Acts/EventData/GenericBoundTrackParameters.hpp"
#include "Acts/EventData/detail/TrackParametersUtils.hpp"
#include "Acts/Plugins/Json/SurfaceJsonConverter.hpp"

#include <nlohmann/json.hpp>

namespace Acts {
NLOHMANN_JSON_SERIALIZE_ENUM(Acts::PdgParticle,

                             {{Acts::PdgParticle::eInvalid, "Invalid"},
                              {Acts::PdgParticle::eElectron, "Electron"},
                              {Acts::PdgParticle::eAntiElectron,
                               "AntiElectron"},
                              {Acts::PdgParticle::ePositron, "Positron"},
                              {Acts::PdgParticle::eMuon, "Muon"},
                              {Acts::PdgParticle::eAntiMuon, "AntiMuon"},
                              {Acts::PdgParticle::eTau, "Tau"},
                              {Acts::PdgParticle::eAntiTau, "AntiTau"},
                              {Acts::PdgParticle::eGamma, "Gamma"},
                              {Acts::PdgParticle::ePionZero, "PionZero"},
                              {Acts::PdgParticle::ePionPlus, "PionPlus"},
                              {Acts::PdgParticle::ePionMinus, "PionMinus"},
                              {Acts::PdgParticle::eKaonPlus, "KaonPlus"},
                              {Acts::PdgParticle::eKaonMinus, "KaonMinus"},
                              {Acts::PdgParticle::eNeutron, "Neutron"},
                              {Acts::PdgParticle::eAntiNeutron, "AntiNeutron"},
                              {Acts::PdgParticle::eProton, "Proton"},
                              {Acts::PdgParticle::eAntiProton, "AntiProton"},
                              {Acts::PdgParticle::eLead, "Lead"}}

)
}

namespace nlohmann {

/// @brief Serialize a track parameters object to json
///
/// nlohmann::json serializer specialized for track parameters
/// as they are not default constructible. Is able to serialize
/// either bound or free track parameters given that the constructor
/// convention is followed.
///
/// @tparam parameters_t The track parameters type
template <Acts::detail::isBoundOrFreeTrackParams parameters_t>
struct adl_serializer<parameters_t> {
  /// Covariance matrix type attached to the parameters
  using CovarianceMatrix = typename parameters_t::CovarianceMatrix;

  /// @brief Serialize track parameters object to json
  ///
  /// @param j Json object to write to
  /// @param t Track parameters object to serialize
  static void to_json(nlohmann::json& j, const parameters_t& t) {
    // Serialize parameters
    // common to all track parameters
    j["direction"] = t.direction();
    j["qOverP"] = t.qOverP();
    j["particleHypothesis"] = t.particleHypothesis().absolutePdg();

    // Covariance is optional
    j["covariance"];
    if (t.covariance().has_value()) {
      // Extract covariance matrix
      // parameters and serialize
      auto cov = t.covariance().value();
      constexpr unsigned int size = cov.rows();
      std::array<double, size * size> covData{};
      for (std::size_t n = 0; n < size; ++n) {
        for (std::size_t m = 0; m < size; ++m) {
          covData[n * size + m] = cov(n, m);
        }
      }
      j["covariance"] = covData;
    }
    // Bound track parameters have
    // reference surface attached
    // and position takes a geometry context
    if constexpr (Acts::detail::isGenericBoundTrackParams<parameters_t>) {
      Acts::GeometryContext gctx;
      j["position"] = t.fourPosition(gctx);

      j["referenceSurface"] =
          Acts::SurfaceJsonConverter::toJson(gctx, t.referenceSurface());
    } else {
      j["position"] = t.fourPosition();
    }
  }

  /// @brief Deserialize track parameters object from json
  ///
  /// @param j Json object to read from
  /// @return Track parameters object
  static parameters_t from_json(const nlohmann::json& j) {
    // Extract common parameters
    std::array<double, 4> posData = j.at("position");
    Acts::Vector4 position(posData[0], posData[1], posData[2], posData[3]);

    std::array<double, 3> dirData = j.at("direction");
    Acts::Vector3 direction(dirData[0], dirData[1], dirData[2]);

    double qOverP = j.at("qOverP");
    Acts::PdgParticle absPdg = j.at("particleHypothesis");

    // Covariance is optional
    std::optional<CovarianceMatrix> cov;
    if (j.at("covariance").is_null()) {
      cov = std::nullopt;
    } else {
      // Extract covariance matrix
      // parameters and deserialize
      CovarianceMatrix mat;
      constexpr unsigned int size = mat.rows();
      std::array<double, size * size> covData = j.at("covariance");
      for (std::size_t n = 0; n < size; ++n) {
        for (std::size_t m = 0; m < size; ++m) {
          mat(n, m) = covData[n * size + m];
        }
      }
      cov.emplace(std::move(mat));
    }

    // Create particle hypothesis
    typename parameters_t::ParticleHypothesis particle(absPdg);

    // Bound track parameters have
    // reference surface attached
    // and constructor is hidden
    // behind a factory method
    if constexpr (Acts::detail::isGenericBoundTrackParams<parameters_t>) {
      Acts::GeometryContext gctx;
      auto referenceSurface =
          Acts::SurfaceJsonConverter::fromJson(j.at("referenceSurface"));

      auto res = parameters_t::create(referenceSurface, gctx, position,
                                      direction, qOverP, cov, particle);

      if (!res.ok()) {
        throw std::invalid_argument("Invalid bound track parameters");
      }
      return res.value();
    } else {
      return parameters_t(position, direction, qOverP, cov, particle);
    }
  }
};

/// @brief Serialize a shared pointer to track parameters object to json
///
/// nlohmann::json serializer specialized for shared pointers to track
/// parameters as they are not default constructible. Is able to serialize
/// either bound or free track parameters given that the constructor
/// convention is followed.
///
/// @tparam parameters_t The track parameters type
template <Acts::detail::isBoundOrFreeTrackParams parameters_t>
struct adl_serializer<std::shared_ptr<parameters_t>> {
  using CovarianceMatrix = typename parameters_t::CovarianceMatrix;
  static void to_json(nlohmann::json& j,
                      const std::shared_ptr<parameters_t>& t) {
    if (t == nullptr) {
      return;
    }
    j = *t;
  }

  static std::shared_ptr<parameters_t> from_json(const nlohmann::json& j) {
    return std::make_shared<parameters_t>(j.get<parameters_t>());
  }
};

/// @brief Serialize a unique pointer to track parameters object to json
///
/// nlohmann::json serializer specialized for unique pointers to track
/// parameters as they are not default constructible. Is able to serialize
/// either bound or free track parameters given that the constructor
/// convention is followed.
///
/// @tparam parameters_t The track parameters type
template <Acts::detail::isBoundOrFreeTrackParams parameters_t>
struct adl_serializer<std::unique_ptr<parameters_t>> {
  using CovarianceMatrix = typename parameters_t::CovarianceMatrix;
  static void to_json(nlohmann::json& j,
                      const std::unique_ptr<parameters_t>& t) {
    if (t == nullptr) {
      return;
    }
    j = *t;
  }

  static std::unique_ptr<parameters_t> from_json(const nlohmann::json& j) {
    return std::make_unique<parameters_t>(j.get<parameters_t>());
  }
};

}  // namespace nlohmann
