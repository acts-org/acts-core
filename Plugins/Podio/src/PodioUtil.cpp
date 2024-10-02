// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "Acts/Plugins/Podio//PodioUtil.hpp"

#include "Acts/Geometry/GeometryContext.hpp"
#include "Acts/Geometry/GeometryIdentifier.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/ConeSurface.hpp"
#include "Acts/Surfaces/ConvexPolygonBounds.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiamondBounds.hpp"
#include "Acts/Surfaces/DiscBounds.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/DiscTrapezoidBounds.hpp"
#include "Acts/Surfaces/EllipseBounds.hpp"
#include "Acts/Surfaces/PerigeeSurface.hpp"
#include "Acts/Surfaces/PlanarBounds.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/StrawSurface.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Utilities/ThrowAssert.hpp"
#include "Acts/Utilities/TypeList.hpp"
#include "ActsPodioEdm/Surface.h"

#include <limits>
#include <memory>

namespace Acts {
namespace PodioUtil {

namespace {
template <typename bounds_t>
std::shared_ptr<const bounds_t> createBounds(
    const ActsPodioEdm::Surface& surface) {
  constexpr std::size_t S = bounds_t::eSize;
  throw_assert(surface.boundValuesSize == S,
               "Unexpected number of bound values");

  std::array<double, S> values{};
  for (std::size_t i = 0; i < S; i++) {
    values.at(i) = surface.boundValues.at(i);
  }
  return std::make_shared<bounds_t>(values);
}
}  // namespace

ActsPodioEdm::Surface convertSurfaceToPodio(const ConversionHelper& helper,
                                            const Acts::Surface& surface) {
  ActsPodioEdm::Surface result;

  std::optional<Identifier> identifier = helper.surfaceToIdentifier(surface);
  if (identifier.has_value()) {
    result.identifier = identifier.value();
  } else {
    result.identifier = kNoIdentifier;
    assert(surface.associatedDetectorElement() == nullptr &&
           "Unidentified surface does not have detector element");
    // @TODO: Surface type is not well-defined for curvilinear surface: looks like any plane surface
    result.surfaceType = surface.type();
    // @TODO: Test line bounds, does not have bounds, so nullptr
    result.boundsType = surface.bounds().type();
    result.geometryId = surface.geometryId().value();
    auto values = surface.bounds().values();

    if (values.size() > result.boundValues.size()) {
      throw std::runtime_error{"Too many bound values to store"};
    }

    for (std::size_t i = 0; i < values.size(); i++) {
      result.boundValues.at(i) = values.at(i);
    }
    result.boundValuesSize = values.size();

    Eigen::Map<ActsSquareMatrix<4>> trf{result.transform.data()};

    // This is safe ONLY(!) if there is no associated detector element, since
    // the surface will not inspect the geometry context at all by itself.
    Acts::GeometryContext gctx;
    trf = surface.transform(gctx).matrix();
  }

  return result;
}

std::shared_ptr<const Surface> convertSurfaceFromPodio(
    const ConversionHelper& helper, const ActsPodioEdm::Surface& surface) {
  if (surface.surfaceType == kNoSurface) {
    return nullptr;
  }

  Eigen::Map<const ActsSquareMatrix<4>> mat{surface.transform.data()};
  Transform3 transform{mat};

  using T = Surface::SurfaceType;
  using B = SurfaceBounds;

  std::shared_ptr<const Surface> result;

  if (const Surface* srf = helper.identifierToSurface(surface.identifier);
      srf != nullptr) {
    result = srf->getSharedPtr();
  }

  if (result) {
    return result;
  }

  switch (surface.surfaceType) {
    default:
      throw std::runtime_error{"Invalid surface type encountered"};

    case T::Cone:
      throw_assert(surface.boundsType == B::eCone, "Unexpected bounds type");
      result = Acts::Surface::makeShared<ConeSurface>(
          transform, createBounds<ConeBounds>(surface));
      break;

    case T::Cylinder:
      throw_assert(surface.boundsType == B::eCylinder,
                   "Unexpected bounds type");
      result = Acts::Surface::makeShared<CylinderSurface>(
          transform, createBounds<CylinderBounds>(surface));
      break;

    case T::Disc: {
      std::shared_ptr<const DiscBounds> dBounds;
      switch (surface.boundsType) {
        default:
          throw std::runtime_error{"Invalid bounds type encountered"};

        case B::eDisc:
          dBounds = createBounds<RadialBounds>(surface);
          break;

        case B::eAnnulus:
          dBounds = createBounds<AnnulusBounds>(surface);
          break;

        case B::eDiscTrapezoid:
          dBounds = createBounds<DiscTrapezoidBounds>(surface);
          break;
      }
      result = Acts::Surface::makeShared<DiscSurface>(transform, dBounds);
      break;
    }

    case T::Perigee:
      throw_assert(surface.boundsType == B::eBoundless,
                   "Unexpected bounds type");
      result = Acts::Surface::makeShared<PerigeeSurface>(transform);
      break;

    case T::Plane: {
      std::shared_ptr<const PlanarBounds> pBounds;
      switch (surface.boundsType) {
        default:
          throw std::runtime_error{"Invalid bounds type encountered"};

        case B::eDiamond:
          pBounds = createBounds<DiamondBounds>(surface);
          break;
        case B::eEllipse:
          pBounds = createBounds<EllipseBounds>(surface);
          break;
        case B::eRectangle:
          pBounds = createBounds<RectangleBounds>(surface);
          break;
        case B::eConvexPolygon:
          template_switch_lambda<6, 32>(surface.boundValuesSize, [&](auto N) {
            constexpr std::size_t nValues = decltype(N)::value;
            constexpr std::size_t nVertices = nValues / 2;
            pBounds = createBounds<ConvexPolygonBounds<nVertices>>(surface);
          });
          // @TODO: Maybe handle dynamic convex polygons?
          break;
      }
      assert(pBounds && "No PlanarBounds");
      result = Acts::Surface::makeShared<PlaneSurface>(transform, pBounds);

      break;
    }

    case T::Straw:
      throw_assert(surface.boundsType == B::eLine, "Unexpected bounds type");
      result = Acts::Surface::makeShared<StrawSurface>(
          transform, createBounds<LineBounds>(surface));
      break;

    case T::Curvilinear:
      throw_assert(surface.boundsType == B::eBoundless,
                   "Unexpected bounds type");
      result = Acts::Surface::makeShared<PlaneSurface>(transform);
      break;
  }

  return result;
}

}  // namespace PodioUtil
namespace podio_detail {

template <typename F, typename... Args>
void apply(F&& f, TypeList<Args...> /*unused*/) {
  f(Args{}...);
}

void recoverDynamicColumns(
    const podio::Frame& frame, const std::string& stem,
    std::unordered_map<HashedString,
                       std::unique_ptr<podio_detail::ConstDynamicColumnBase>>&
        dynamic) {
  // See
  // https://github.com/AIDASoft/podio/blob/858c0ff0b841705d1b18aafd57569fcbd1beda91/include/podio/UserDataCollection.h#L30-L31
  using types = TypeList<float, double, std::int8_t, std::int16_t, std::int32_t,
                         std::int64_t, std::uint8_t, std::uint16_t,
                         std::uint32_t, std::uint64_t>;

  std::vector<std::string> available = frame.getAvailableCollections();

  for (const auto& col : available) {
    std::string prefix = stem + "_extra__";
    std::size_t p = col.find(prefix);
    if (p == std::string::npos) {
      continue;
    }
    std::string dynName = col.substr(prefix.size());
    const podio::CollectionBase* coll = frame.get(col);

    std::unique_ptr<podio_detail::ConstDynamicColumnBase> up;

    apply(
        [&](auto... args) {
          auto inner = [&](auto arg) {
            if (up) {
              return;
            }
            using T = decltype(arg);
            const auto* dyn =
                dynamic_cast<const podio::UserDataCollection<T>*>(coll);
            if (dyn == nullptr) {
              return;
            }
            up = std::make_unique<podio_detail::ConstDynamicColumn<T>>(dynName,
                                                                       *dyn);
          };

          ((inner(args)), ...);
        },
        types{});

    if (!up) {
      throw std::runtime_error{"Dynamic column '" + dynName +
                               "' is not of allowed type"};
    }

    HashedString hashedKey = hashString(dynName);
    dynamic.insert({hashedKey, std::move(up)});
  }
}

}  // namespace podio_detail
}  // namespace Acts
