// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/Json/GridJsonConverter.hpp"

#include "Acts/Plugins/Json/AlgebraJsonConverter.hpp"
#include "Acts/Plugins/Json/UtilitiesJsonConverter.hpp"
#include "Acts/Utilities/IAxis.hpp"

nlohmann::json Acts::AxisJsonConverter::toJson(const IAxis& ia) {
  nlohmann::json jAxis;

  jAxis["boundary_type"] = ia.getBoundaryType();
  // type, range, bins or boundaries
  if (ia.isEquidistant()) {
    jAxis["type"] = AxisType::Equidistant;
    jAxis["range"] = std::array<double, 2u>({ia.getMin(), ia.getMax()});
    jAxis["bins"] = ia.getNBins();
  } else {
    jAxis["type"] = AxisType::Variable;
    jAxis["boundaries"] = ia.getBinEdges();
  }
  return jAxis;
}

nlohmann::json Acts::AxisJsonConverter::toJsonDetray(const IAxis& ia) {
  nlohmann::json jAxis;
  jAxis["bounds"] =
      ia.getBoundaryType() == Acts::AxisBoundaryType::Bound ? 1 : 2;
  jAxis["binning"] = ia.isEquidistant() ? 0 : 1;
  jAxis["bins"] = ia.getNBins();
  if (ia.isEquidistant()) {
    std::array<double, 2u> range = {ia.getBinEdges().front(),
                                    ia.getBinEdges().back()};
    jAxis["edges"] = range;

  } else {
    jAxis["edges"] = ia.getBinEdges();
  }
  return jAxis;
}

namespace {

template <typename Subspace>
void encodeSubspace(
    nlohmann::json& jGlobalToGridLocal,
    const Acts::GridAccess::IGlobalToGridLocal& globalToGridLocal,
    const Subspace& /*subspace*/) {
  const Subspace* subspace = dynamic_cast<const Subspace*>(&globalToGridLocal);
  if (subspace != nullptr) {
    jGlobalToGridLocal["type"] = "subspace";
    jGlobalToGridLocal["accessors"] = subspace->axisDirs;
  }
}

template <typename Subspace>
void encodeTransformedSubspace(
    nlohmann::json& jGlobalToGridLocal,
    const Acts::GridAccess::IGlobalToGridLocal& globalToGridLocal,
    const Subspace& subscpace) {
  const Acts::GridAccess::Affine3Transformed<Subspace>* tsubspace =
      dynamic_cast<const Acts::GridAccess::Affine3Transformed<Subspace>*>(
          &globalToGridLocal);
  if (tsubspace != nullptr) {
    encodeSubspace(jGlobalToGridLocal, tsubspace->globalToGridLocal, subscpace);
    jGlobalToGridLocal["transform"] =
        Acts::Transform3JsonConverter::toJson(tsubspace->transform);
  }
}

template <typename... Args>
void encodeSubspaces(
    nlohmann::json& jGlobalToGridLocal,
    const Acts::GridAccess::IGlobalToGridLocal& globalToGridLocal,
    bool transformed, const std::tuple<Args...>& tAcessors) {
  if (transformed) {
    std::apply(
        [&](auto&&... vals) {
          (encodeTransformedSubspace(jGlobalToGridLocal, globalToGridLocal,
                                     vals),
           ...);
        },
        tAcessors);
  } else {
    std::apply(
        [&](auto&&... vals) {
          (encodeSubspace(jGlobalToGridLocal, globalToGridLocal, vals), ...);
        },
        tAcessors);
  }
}

template <Acts::AxisDirection... Args>
std::unique_ptr<const Acts::GridAccess::GlobalSubspace<Args...>> decodeSubspace(
    const nlohmann::json& /*j*/) {
  return std::make_unique<const Acts::GridAccess::GlobalSubspace<Args...>>();
}

template <Acts::AxisDirection... Args>
std::unique_ptr<const Acts::GridAccess::Affine3Transformed<
    Acts::GridAccess::GlobalSubspace<Args...>>>
decodeTransformedSubspace(const nlohmann::json& jGlobalToGridLocal) {
  Acts::Transform3 transform = Acts::Transform3JsonConverter::fromJson(
      jGlobalToGridLocal.at("transform"));
  Acts::GridAccess::GlobalSubspace<Args...> globalSubspace;
  return std::make_unique<const Acts::GridAccess::Affine3Transformed<
      Acts::GridAccess::GlobalSubspace<Args...>>>(std::move(globalSubspace),
                                                  transform);
}

template <Acts::AxisDirection... Args>
std::unique_ptr<const Acts::GridAccess::IGlobalToGridLocal>
decodeGeneralSubspace(const nlohmann::json& jGlobalToGridLocal) {
  if (jGlobalToGridLocal.find("transform") != jGlobalToGridLocal.end()) {
    return decodeTransformedSubspace<Args...>(jGlobalToGridLocal);
  }
  return decodeSubspace<Args...>(jGlobalToGridLocal);
}

template <typename Delegate, Acts::AxisDirection... Args>
void decorateGlobalDelegate(Delegate& delegate,
                            const nlohmann::json& jGlobalToGridLocal) {
  // The delegate has already been connected
  if (delegate.connected()) {
    return;
  }

  // Get the transform for json
  bool hasTransform =
      jGlobalToGridLocal.find("transform") != jGlobalToGridLocal.end();

  // Get the accessors
  std::vector<Acts::AxisDirection> accessors =
      jGlobalToGridLocal.at("accessors")
          .get<std::vector<Acts::AxisDirection>>();

  // One dimensional setting
  if constexpr (sizeof...(Args) == 1u) {
    if (std::get<0>(std::forward_as_tuple(Args...)) == accessors[0]) {
      if (hasTransform) {
        using TransformedSubspace = Acts::GridAccess::Affine3Transformed<
            Acts::GridAccess::GlobalSubspace<Args...>>;
        auto globalToGridLocal =
            decodeTransformedSubspace<Args...>(jGlobalToGridLocal);
        delegate.template connect<&TransformedSubspace::toGridLocal>(
            std::move(globalToGridLocal));
      } else {
        auto globalToGridLocal = decodeSubspace<Args...>(jGlobalToGridLocal);
        delegate.template connect<
            &Acts::GridAccess::GlobalSubspace<Args...>::toGridLocal>(
            std::move(globalToGridLocal));
      }
    }
  }

  // Two-dimensional setting
  if constexpr (sizeof...(Args) == 2u) {
    if (std::get<0>(std::forward_as_tuple(Args...)) == accessors[0] &&
        std::get<1>(std::forward_as_tuple(Args...)) == accessors[1]) {
      if (hasTransform) {
        using TransformedSubspace = Acts::GridAccess::Affine3Transformed<
            Acts::GridAccess::GlobalSubspace<Args...>>;
        auto globalToGridLocal =
            decodeTransformedSubspace<Args...>(jGlobalToGridLocal);
        delegate.template connect<&TransformedSubspace::toGridLocal>(
            std::move(globalToGridLocal));
      } else {
        auto globalToGridLocal = decodeSubspace<Args...>(jGlobalToGridLocal);
        delegate.template connect<
            &Acts::GridAccess::GlobalSubspace<Args...>::toGridLocal>(
            std::move(globalToGridLocal));
      }
    }
  }
}

template <Acts::AxisDirection... Args>
void decorateGlobal1DimDelegate(
    Acts::GridAccess::GlobalToGridLocal1DimDelegate& delegate,
    const nlohmann::json& jGlobalToGridLocal) {
  (((decorateGlobalDelegate<Acts::GridAccess::GlobalToGridLocal1DimDelegate,
                            Args>(delegate, jGlobalToGridLocal))),
   ...);
}

}  // namespace

nlohmann::json Acts::GridAccessJsonConverter::toJson(
    const GridAccess::IGlobalToGridLocal& globalToGridLocal) {
  nlohmann::json jGlobalToGridLocal;

  std::array<bool, 2u> transformOptions = {false, true};

  // One dimensional sub spaces
  const std::tuple<GridAccess::GlobalSubspace<AxisDirection::AxisX>,
                   GridAccess::GlobalSubspace<AxisDirection::AxisY>,
                   GridAccess::GlobalSubspace<AxisDirection::AxisZ>,
                   GridAccess::GlobalSubspace<AxisDirection::AxisR>,
                   GridAccess::GlobalSubspace<AxisDirection::AxisPhi>,
                   GridAccess::GlobalSubspace<AxisDirection::AxisEta>>
      oneDimSubspaces = {};

  for (bool transform : transformOptions) {
    encodeSubspaces(jGlobalToGridLocal, globalToGridLocal, transform,
                    oneDimSubspaces);
    if (!jGlobalToGridLocal.empty()) {
      return jGlobalToGridLocal;
    }
  }

  // Useful two dimensional sub spaces
  const std::tuple<
      GridAccess::GlobalSubspace<AxisDirection::AxisX, AxisDirection::AxisY>,
      GridAccess::GlobalSubspace<AxisDirection::AxisY, AxisDirection::AxisX>,
      GridAccess::GlobalSubspace<AxisDirection::AxisX, AxisDirection::AxisZ>,
      GridAccess::GlobalSubspace<AxisDirection::AxisZ, AxisDirection::AxisX>,
      GridAccess::GlobalSubspace<AxisDirection::AxisY, AxisDirection::AxisZ>,
      GridAccess::GlobalSubspace<AxisDirection::AxisZ, AxisDirection::AxisY>,
      GridAccess::GlobalSubspace<AxisDirection::AxisR, AxisDirection::AxisPhi>,
      GridAccess::GlobalSubspace<AxisDirection::AxisPhi, AxisDirection::AxisR>,
      GridAccess::GlobalSubspace<AxisDirection::AxisZ, AxisDirection::AxisPhi>,
      GridAccess::GlobalSubspace<AxisDirection::AxisPhi, AxisDirection::AxisZ>>
      twoDimSubspaces = {};

  for (bool transform : transformOptions) {
    encodeSubspaces(jGlobalToGridLocal, globalToGridLocal, transform,
                    twoDimSubspaces);
    if (!jGlobalToGridLocal.empty()) {
      return jGlobalToGridLocal;
    }
  }
  return jGlobalToGridLocal;
}

std::unique_ptr<const Acts::GridAccess::IGlobalToGridLocal>
Acts::GridAccessJsonConverter::globalToGridLocalFromJson(
    const nlohmann::json& jGlobalToGridLocal) {
  std::unique_ptr<const Acts::GridAccess::IGlobalToGridLocal>
      globalToGridLocal = nullptr;

  std::vector<AxisDirection> accessors =
      jGlobalToGridLocal.at("accessors").get<std::vector<AxisDirection>>();

  // Switch and fill for 1D
  if (accessors.size() == 1u) {
    switch (accessors[0]) {
      case AxisDirection::AxisX:
        globalToGridLocal =
            decodeGeneralSubspace<AxisDirection::AxisX>(jGlobalToGridLocal);
        break;
      case AxisDirection::AxisY:
        globalToGridLocal =
            decodeGeneralSubspace<AxisDirection::AxisY>(jGlobalToGridLocal);
        break;
      case AxisDirection::AxisZ:
        globalToGridLocal =
            decodeGeneralSubspace<AxisDirection::AxisZ>(jGlobalToGridLocal);
        break;
      case AxisDirection::AxisR:
        globalToGridLocal =
            decodeGeneralSubspace<AxisDirection::AxisR>(jGlobalToGridLocal);
        break;
      case AxisDirection::AxisPhi:
        globalToGridLocal =
            decodeGeneralSubspace<AxisDirection::AxisPhi>(jGlobalToGridLocal);
        break;
      case AxisDirection::AxisEta:
        globalToGridLocal =
            decodeGeneralSubspace<AxisDirection::AxisEta>(jGlobalToGridLocal);
        break;
      default:
        // globalToGridLocal = nullptr;
        break;
    }
  }

  // Switch and fill for 2D
  if (accessors.size() == 2u) {
    if (accessors == std::vector<AxisDirection>{AxisDirection::AxisX,
                                                AxisDirection::AxisY}) {
      globalToGridLocal =
          decodeGeneralSubspace<AxisDirection::AxisX, AxisDirection::AxisY>(
              jGlobalToGridLocal);
    } else if (accessors == std::vector<AxisDirection>{AxisDirection::AxisY,
                                                       AxisDirection::AxisX}) {
      globalToGridLocal =
          decodeGeneralSubspace<AxisDirection::AxisY, AxisDirection::AxisX>(
              jGlobalToGridLocal);
    } else if (accessors == std::vector<AxisDirection>{AxisDirection::AxisX,
                                                       AxisDirection::AxisZ}) {
      globalToGridLocal =
          decodeGeneralSubspace<AxisDirection::AxisX, AxisDirection::AxisZ>(
              jGlobalToGridLocal);
    } else if (accessors == std::vector<AxisDirection>{AxisDirection::AxisZ,
                                                       AxisDirection::AxisX}) {
      globalToGridLocal =
          decodeGeneralSubspace<AxisDirection::AxisZ, AxisDirection::AxisX>(
              jGlobalToGridLocal);
    } else if (accessors == std::vector<AxisDirection>{AxisDirection::AxisY,
                                                       AxisDirection::AxisZ}) {
      globalToGridLocal =
          decodeGeneralSubspace<AxisDirection::AxisY, AxisDirection::AxisZ>(
              jGlobalToGridLocal);
    } else if (accessors == std::vector<AxisDirection>{AxisDirection::AxisZ,
                                                       AxisDirection::AxisY}) {
      globalToGridLocal =
          decodeGeneralSubspace<AxisDirection::AxisZ, AxisDirection::AxisY>(
              jGlobalToGridLocal);
    } else if (accessors == std::vector<AxisDirection>{
                                AxisDirection::AxisR, AxisDirection::AxisPhi}) {
      globalToGridLocal =
          decodeGeneralSubspace<AxisDirection::AxisR, AxisDirection::AxisPhi>(
              jGlobalToGridLocal);
    } else if (accessors == std::vector<AxisDirection>{AxisDirection::AxisPhi,
                                                       AxisDirection::AxisR}) {
      globalToGridLocal =
          decodeGeneralSubspace<AxisDirection::AxisPhi, AxisDirection::AxisR>(
              jGlobalToGridLocal);
    } else if (accessors == std::vector<AxisDirection>{
                                AxisDirection::AxisZ, AxisDirection::AxisPhi}) {
      globalToGridLocal =
          decodeGeneralSubspace<AxisDirection::AxisZ, AxisDirection::AxisPhi>(
              jGlobalToGridLocal);
    } else if (accessors == std::vector<AxisDirection>{AxisDirection::AxisPhi,
                                                       AxisDirection::AxisZ}) {
      globalToGridLocal =
          decodeGeneralSubspace<AxisDirection::AxisPhi, AxisDirection::AxisZ>(
              jGlobalToGridLocal);
    }
    // else globalToGridLocal = nullptr;
  }
  return globalToGridLocal;
}

Acts::GridAccess::GlobalToGridLocal1DimDelegate
Acts::GridAccessJsonConverter::globalToGridLocal1DimDelegateFromJson(
    const nlohmann::json& jGlobalToGridLocal) {
  // Peek into json to check the right dimension
  if (jGlobalToGridLocal.at("accessors").size() != 1u) {
    throw std::invalid_argument(
        "GridAccessJsonConverter: json input does not describe 1D case.");
  }
  // Unroll the decoration
  Acts::GridAccess::GlobalToGridLocal1DimDelegate delegate;
  decorateGlobal1DimDelegate<AxisDirection::AxisX, AxisDirection::AxisY,
                             AxisDirection::AxisZ, AxisDirection::AxisR,
                             AxisDirection::AxisPhi, AxisDirection::AxisEta>(
      delegate, jGlobalToGridLocal);
  return delegate;
}

Acts::GridAccess::GlobalToGridLocal2DimDelegate
Acts::GridAccessJsonConverter::globalToGridLocal2DimDelegateFromJson(
    const nlohmann::json& jGlobalToGridLocal) {
  // Peek into json to check the right dimension
  if (jGlobalToGridLocal.at("accessors").size() != 2u) {
    throw std::invalid_argument(
        "GridAccessJsonConverter: json input does not describe 2D case.");
  }
  // Unroll the decoration
  Acts::GridAccess::GlobalToGridLocal2DimDelegate delegate;
  // Only the matching one will be applied, matching condition is checked inside
  // the call - may unroll this es well
  decorateGlobalDelegate<Acts::GridAccess::GlobalToGridLocal2DimDelegate,
                         AxisDirection::AxisX, AxisDirection::AxisY>(
      delegate, jGlobalToGridLocal);
  decorateGlobalDelegate<Acts::GridAccess::GlobalToGridLocal2DimDelegate,
                         AxisDirection::AxisY, AxisDirection::AxisX>(
      delegate, jGlobalToGridLocal);
  decorateGlobalDelegate<Acts::GridAccess::GlobalToGridLocal2DimDelegate,
                         AxisDirection::AxisX, AxisDirection::AxisZ>(
      delegate, jGlobalToGridLocal);
  decorateGlobalDelegate<Acts::GridAccess::GlobalToGridLocal2DimDelegate,
                         AxisDirection::AxisZ, AxisDirection::AxisX>(
      delegate, jGlobalToGridLocal);
  decorateGlobalDelegate<Acts::GridAccess::GlobalToGridLocal2DimDelegate,
                         AxisDirection::AxisY, AxisDirection::AxisZ>(
      delegate, jGlobalToGridLocal);
  decorateGlobalDelegate<Acts::GridAccess::GlobalToGridLocal2DimDelegate,
                         AxisDirection::AxisZ, AxisDirection::AxisY>(
      delegate, jGlobalToGridLocal);
  decorateGlobalDelegate<Acts::GridAccess::GlobalToGridLocal2DimDelegate,
                         AxisDirection::AxisR, AxisDirection::AxisPhi>(
      delegate, jGlobalToGridLocal);
  decorateGlobalDelegate<Acts::GridAccess::GlobalToGridLocal2DimDelegate,
                         AxisDirection::AxisPhi, AxisDirection::AxisR>(
      delegate, jGlobalToGridLocal);
  decorateGlobalDelegate<Acts::GridAccess::GlobalToGridLocal2DimDelegate,
                         AxisDirection::AxisZ, AxisDirection::AxisPhi>(
      delegate, jGlobalToGridLocal);
  decorateGlobalDelegate<Acts::GridAccess::GlobalToGridLocal2DimDelegate,
                         AxisDirection::AxisPhi, AxisDirection::AxisZ>(
      delegate, jGlobalToGridLocal);
  return delegate;
}

nlohmann::json Acts::GridAccessJsonConverter::toJson(
    const GridAccess::IBoundToGridLocal& boundToGridLocal) {
  nlohmann::json jBoundToGridLocal;

  auto localSubSpace0 =
      dynamic_cast<const GridAccess::LocalSubspace<0u>*>(&boundToGridLocal);
  if (localSubSpace0 != nullptr) {
    jBoundToGridLocal["type"] = "subspace";
    jBoundToGridLocal["accessors"] = localSubSpace0->accessors;
  }

  auto localSubSpace1 =
      dynamic_cast<const GridAccess::LocalSubspace<1u>*>(&boundToGridLocal);
  if (localSubSpace1 != nullptr) {
    jBoundToGridLocal["type"] = "subspace";
    jBoundToGridLocal["accessors"] = localSubSpace1->accessors;
  }

  auto localSubSpace01 =
      dynamic_cast<const GridAccess::LocalSubspace<0u, 1u>*>(&boundToGridLocal);
  if (localSubSpace01 != nullptr) {
    jBoundToGridLocal["type"] = "subspace";
    jBoundToGridLocal["accessors"] = localSubSpace01->accessors;
  }

  auto localSubSpace10 =
      dynamic_cast<const GridAccess::LocalSubspace<1u, 0u>*>(&boundToGridLocal);
  if (localSubSpace10 != nullptr) {
    jBoundToGridLocal["type"] = "subspace";
    jBoundToGridLocal["accessors"] = localSubSpace10->accessors;
  }

  auto boundCylinderToZPhi =
      dynamic_cast<const GridAccess::BoundCylinderToZPhi*>(&boundToGridLocal);
  if (boundCylinderToZPhi != nullptr) {
    jBoundToGridLocal["type"] = "cylinder_to_zphi";
    jBoundToGridLocal["radius"] = boundCylinderToZPhi->radius;
    jBoundToGridLocal["shift"] = boundCylinderToZPhi->shift;
  }

  if (jBoundToGridLocal.empty()) {
    throw std::invalid_argument(
        "GridAccessJsonConverter: boundToGridLocal type not supported.");
  }

  return jBoundToGridLocal;
}

std::unique_ptr<Acts::GridAccess::IBoundToGridLocal>
Acts::GridAccessJsonConverter::boundToGridLocalFromJson(
    const nlohmann::json& jBoundToGridLocal) {
  std::unique_ptr<Acts::GridAccess::IBoundToGridLocal> boundToGridLocal =
      nullptr;
  std::string type = jBoundToGridLocal.at("type").get<std::string>();
  if (type == "subspace") {
    std::vector<std::size_t> accessors =
        jBoundToGridLocal.at("accessors").get<std::vector<std::size_t>>();
    if (accessors.size() == 1 && accessors[0] == 0) {
      boundToGridLocal =
          std::make_unique<Acts::GridAccess::LocalSubspace<0u>>();
    } else if (accessors.size() == 1 && accessors[0] == 1) {
      boundToGridLocal =
          std::make_unique<Acts::GridAccess::LocalSubspace<1u>>();
    } else if (accessors.size() == 2 && accessors[0] == 0 &&
               accessors[1] == 1) {
      boundToGridLocal =
          std::make_unique<Acts::GridAccess::LocalSubspace<0u, 1u>>();
    } else if (accessors.size() == 2 && accessors[0] == 1 &&
               accessors[1] == 0) {
      boundToGridLocal =
          std::make_unique<Acts::GridAccess::LocalSubspace<1u, 0u>>();
    }
  } else if (type == "cylinder_to_zphi") {
    double radius = jBoundToGridLocal.at("radius").get<double>();
    double shift = jBoundToGridLocal.at("shift").get<double>();
    boundToGridLocal =
        std::make_unique<Acts::GridAccess::BoundCylinderToZPhi>(radius, shift);
  }
  return boundToGridLocal;
}

Acts::GridAccess::BoundToGridLocal1DimDelegate
Acts::GridAccessJsonConverter::boundToGridLocal1DimDelegateFromJson(
    const nlohmann::json& jBoundToGridLocal) {
  Acts::GridAccess::BoundToGridLocal1DimDelegate delegate;

  std::string type = jBoundToGridLocal.at("type").get<std::string>();
  if (type == "subspace") {
    std::vector<std::size_t> accessors =
        jBoundToGridLocal.at("accessors").get<std::vector<std::size_t>>();
    // Safety check
    if (accessors.size() != 1u) {
      throw std::invalid_argument(
          "GridAccessJsonConverter: json input does not describe 1D case.");
    }
    // Specify the type
    if (accessors[0] == 0) {
      auto boundToGridLocal =
          std::make_unique<const Acts::GridAccess::LocalSubspace<0u>>();
      delegate.connect<&Acts::GridAccess::LocalSubspace<0u>::toGridLocal>(
          std::move(boundToGridLocal));
    } else if (accessors[0] == 1) {
      auto boundToGridLocal =
          std::make_unique<const Acts::GridAccess::LocalSubspace<1u>>();
      delegate.connect<&Acts::GridAccess::LocalSubspace<1u>::toGridLocal>(
          std::move(boundToGridLocal));
    }
  }
  return delegate;
}

Acts::GridAccess::BoundToGridLocal2DimDelegate
Acts::GridAccessJsonConverter::boundToGridLocal2DimDelegateFromJson(
    const nlohmann::json& jBoundToGridLocal) {
  Acts::GridAccess::BoundToGridLocal2DimDelegate delegate;

  std::string type = jBoundToGridLocal.at("type").get<std::string>();
  if (type == "subspace") {
    std::vector<std::size_t> accessors =
        jBoundToGridLocal.at("accessors").get<std::vector<std::size_t>>();

    // Safety check
    if (accessors.size() != 2u) {
      throw std::invalid_argument(
          "GridAccessJsonConverter: json input does not describe 2D case.");
    }
    if (accessors[0] == 0u && accessors[1] == 1u) {
      auto boundToGridLocal =
          std::make_unique<const Acts::GridAccess::LocalSubspace<0u, 1u>>();
      delegate.connect<&Acts::GridAccess::LocalSubspace<0u, 1u>::toGridLocal>(
          std::move(boundToGridLocal));
    } else if (accessors[0] == 1u && accessors[1] == 0u) {
      auto boundToGridLocal =
          std::make_unique<const Acts::GridAccess::LocalSubspace<1u, 0u>>();
      delegate.connect<&Acts::GridAccess::LocalSubspace<1u, 0u>::toGridLocal>(
          std::move(boundToGridLocal));
    }
  } else if (type == "cylinder_to_zphi") {
    double radius = jBoundToGridLocal.at("radius").get<double>();
    double shift = jBoundToGridLocal.at("shift").get<double>();
    auto boundToGridLocal =
        std::make_unique<const Acts::GridAccess::BoundCylinderToZPhi>(radius,
                                                                      shift);
    delegate.connect<&Acts::GridAccess::BoundCylinderToZPhi::toGridLocal>(
        std::move(boundToGridLocal));
  }

  return delegate;
}
