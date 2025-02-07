// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/TGeo/TGeoSurfaceConverter.hpp"

#include "Acts/Definitions/Tolerance.hpp"
#include "Acts/Plugins/TGeo/TGeoPrimitivesHelper.hpp"
#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Surfaces/ConvexPolygonBounds.hpp"
#include "Acts/Surfaces/CylinderBounds.hpp"
#include "Acts/Surfaces/CylinderSurface.hpp"
#include "Acts/Surfaces/DiscSurface.hpp"
#include "Acts/Surfaces/PlaneSurface.hpp"
#include "Acts/Surfaces/RadialBounds.hpp"
#include "Acts/Surfaces/RectangleBounds.hpp"
#include "Acts/Surfaces/Surface.hpp"
#include "Acts/Surfaces/TrapezoidBounds.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <memory>
#include <numbers>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>

#include "RtypesCore.h"
#include "TGeoArb8.h"
#include "TGeoBBox.h"
#include "TGeoBoolNode.h"
#include "TGeoCompositeShape.h"
#include "TGeoMatrix.h"
#include "TGeoShape.h"
#include "TGeoTrd1.h"
#include "TGeoTrd2.h"
#include "TGeoTube.h"

std::tuple<std::shared_ptr<const Acts::CylinderBounds>, const Acts::Transform3,
           double>
Acts::TGeoSurfaceConverter::cylinderComponents(const TGeoShape& tgShape,
                                               const Double_t* rotation,
                                               const Double_t* translation,
                                               const std::string& axes,
                                               double scalor) noexcept(false) {
  std::shared_ptr<const CylinderBounds> bounds = nullptr;
  Transform3 transform = Transform3::Identity();
  double thickness = 0.;

  // Check if it's a tube (segment)
  auto tube = dynamic_cast<const TGeoTube*>(&tgShape);
  if (tube != nullptr) {
    if (!boost::istarts_with(axes, "XY") && !boost::istarts_with(axes, "YX")) {
      throw std::invalid_argument(
          "TGeoShape -> CylinderSurface (full): can only be converted with "
          "'(x/X)(y/Y)(*)' or '(y/Y)(x/X)(*) axes.");
    }

    // The sign of the axes
    int xs = std::islower(axes.at(0)) != 0 ? -1 : 1;
    int ys = std::islower(axes.at(1)) != 0 ? -1 : 1;

    // Create translation and rotation
    Vector3 t(scalor * translation[0], scalor * translation[1],
              scalor * translation[2]);
    bool flipxy = !boost::istarts_with(axes, "X");
    Vector3 ax = flipxy ? xs * Vector3(rotation[1], rotation[4], rotation[7])
                        : xs * Vector3(rotation[0], rotation[3], rotation[6]);
    Vector3 ay = flipxy ? ys * Vector3(rotation[0], rotation[3], rotation[6])
                        : ys * Vector3(rotation[1], rotation[4], rotation[7]);
    Vector3 az = ax.cross(ay);

    double minR = tube->GetRmin() * scalor;
    double maxR = tube->GetRmax() * scalor;
    double deltaR = maxR - minR;
    double medR = 0.5 * (minR + maxR);
    double halfZ = tube->GetDz() * scalor;
    if (halfZ > deltaR) {
      transform = TGeoPrimitivesHelper::makeTransform(ax, ay, az, t);
      double halfPhi = std::numbers::pi;
      double avgPhi = 0.;
      // Check if it's a segment
      auto tubeSeg = dynamic_cast<const TGeoTubeSeg*>(tube);
      if (tubeSeg != nullptr) {
        double phi1 = toRadian(tubeSeg->GetPhi1());
        double phi2 = toRadian(tubeSeg->GetPhi2());
        if (std::abs(phi2 - phi1) < std::numbers::pi * (1. - s_epsilon)) {
          if (!boost::starts_with(axes, "X")) {
            throw std::invalid_argument(
                "TGeoShape -> CylinderSurface (sectorial): can only be "
                "converted "
                "with "
                "'(X)(y/Y)(*)' axes.");
          }
          halfPhi = 0.5 * (std::max(phi1, phi2) - std::min(phi1, phi2));
          avgPhi = 0.5 * (phi1 + phi2);
        }
      }
      bounds = std::make_shared<CylinderBounds>(medR, halfZ, halfPhi, avgPhi);
      thickness = deltaR;
    }
  }
  return {bounds, transform, thickness};
}

std::tuple<std::shared_ptr<const Acts::DiscBounds>, const Acts::Transform3,
           double>
Acts::TGeoSurfaceConverter::discComponents(const TGeoShape& tgShape,
                                           const Double_t* rotation,
                                           const Double_t* translation,
                                           const std::string& axes,
                                           double scalor) noexcept(false) {
  using Line2D = Eigen::Hyperplane<double, 2>;
  std::shared_ptr<const DiscBounds> bounds = nullptr;
  Transform3 transform = Transform3::Identity();

  double thickness = 0.;
  // Special test for composite shape of silicon
  auto compShape = dynamic_cast<const TGeoCompositeShape*>(&tgShape);
  if (compShape != nullptr) {
    if (!boost::istarts_with(axes, "XY")) {
      throw std::invalid_argument(
          "TGeoShape -> DiscSurface (Annulus): can only be converted with "
          "'(x/X)(y/Y)(*)' "
          "axes");
    }

    // Create translation and rotation
    Vector3 t(scalor * translation[0], scalor * translation[1],
              scalor * translation[2]);
    Vector3 ax(rotation[0], rotation[3], rotation[6]);
    Vector3 ay(rotation[1], rotation[4], rotation[7]);
    Vector3 az(rotation[2], rotation[5], rotation[8]);

    transform = TGeoPrimitivesHelper::makeTransform(ax, ay, az, t);

    auto interNode = dynamic_cast<TGeoIntersection*>(compShape->GetBoolNode());
    if (interNode != nullptr) {
      auto baseTube = dynamic_cast<TGeoTubeSeg*>(interNode->GetLeftShape());
      if (baseTube != nullptr) {
        double rMin = baseTube->GetRmin() * scalor;
        double rMax = baseTube->GetRmax() * scalor;
        auto maskShape = dynamic_cast<TGeoArb8*>(interNode->GetRightShape());
        if (maskShape != nullptr) {
          auto maskTransform = interNode->GetRightMatrix();
          // Get the only vertices
          const Double_t* polyVrt = maskShape->GetVertices();
          // Apply the whole transformation stored for the
          // polyhedron, since there is a translation and
          // also a side flip that needs to be applied.
          // @TODO check that 3rd coordinate is not altered by
          // the transformation ?
          std::vector<Vector2> vertices;
          for (unsigned int v = 0; v < 8; v += 2) {
            std::array<double, 3> local{polyVrt[v + 0], polyVrt[v + 1], 0.};
            std::array<double, 3> global{};
            maskTransform->LocalToMaster(local.data(), global.data());
            Vector2 vtx = Vector2(global[0] * scalor, global[1] * scalor);
            vertices.push_back(vtx);
          }

          std::vector<std::pair<Vector2, Vector2>> boundLines;
          for (std::size_t i = 0; i < vertices.size(); ++i) {
            Vector2 a = vertices.at(i);
            Vector2 b = vertices.at((i + 1) % vertices.size());
            Vector2 ab = b - a;
            double phi = VectorHelpers::phi(ab);

            if (std::abs(phi) > 3 * std::numbers::pi / 4. ||
                std::abs(phi) < std::numbers::pi / 4.) {
              if (a.norm() < b.norm()) {
                boundLines.push_back(std::make_pair(a, b));
              } else {
                boundLines.push_back(std::make_pair(b, a));
              }
            }
          }

          if (boundLines.size() != 2) {
            throw std::logic_error(
                "Input DiscPoly bounds type does not have sensible edges.");
          }

          Line2D lA =
              Line2D::Through(boundLines[0].first, boundLines[0].second);
          Line2D lB =
              Line2D::Through(boundLines[1].first, boundLines[1].second);
          Vector2 ix = lA.intersection(lB);

          const Eigen::Translation3d originTranslation(ix.x(), ix.y(), 0.);
          const Vector2 originShift = -ix;

          // Update transform by prepending the origin shift translation
          transform = transform * originTranslation;
          // Transform phi line point to new origin and get phi
          double phi1 =
              VectorHelpers::phi(boundLines[0].second - boundLines[0].first);
          double phi2 =
              VectorHelpers::phi(boundLines[1].second - boundLines[1].first);
          double phiMax = std::max(phi1, phi2);
          double phiMin = std::min(phi1, phi2);
          double phiShift = 0.;

          // Create the bounds
          auto annulusBounds = std::make_shared<const AnnulusBounds>(
              rMin, rMax, phiMin, phiMax, originShift, phiShift);

          thickness = maskShape->GetDZ() * scalor;
          bounds = annulusBounds;
        }
      }
    }
  } else {
    // Check if it's a tube
    auto tube = dynamic_cast<const TGeoTube*>(&tgShape);
    if (tube != nullptr) {
      if (!boost::istarts_with(axes, "XY") &&
          !boost::istarts_with(axes, "YX")) {
        throw std::invalid_argument(
            "TGeoShape -> DiscSurface: can only be converted with "
            "'(x/X)(y/Y)(*)' or '(y/Y)(x/X)(*) axes.");
      }

      // The sign of the axes
      int xs = std::islower(axes.at(0)) != 0 ? -1 : 1;
      int ys = std::islower(axes.at(1)) != 0 ? -1 : 1;

      // Create translation and rotation
      Vector3 t(scalor * translation[0], scalor * translation[1],
                scalor * translation[2]);
      Vector3 ax = xs * Vector3(rotation[0], rotation[3], rotation[6]);
      Vector3 ay = ys * Vector3(rotation[1], rotation[4], rotation[7]);
      Vector3 az = ax.cross(ay);
      transform = TGeoPrimitivesHelper::makeTransform(ax, ay, az, t);

      double minR = tube->GetRmin() * scalor;
      double maxR = tube->GetRmax() * scalor;
      double halfZ = tube->GetDz() * scalor;
      double halfPhi = std::numbers::pi;
      double avgPhi = 0.;
      // Check if it's a segment
      auto tubeSeg = dynamic_cast<const TGeoTubeSeg*>(tube);
      if (tubeSeg != nullptr) {
        double phi1 = toRadian(tubeSeg->GetPhi1());
        double phi2 = toRadian(tubeSeg->GetPhi2());
        if (std::abs(phi2 - phi1) < 2 * std::numbers::pi * (1. - s_epsilon)) {
          if (!boost::starts_with(axes, "X")) {
            throw std::invalid_argument(
                "TGeoShape -> CylinderSurface (sectorial): can only be "
                "converted "
                "with "
                "'(X)(y/Y)(*)' axes.");
          }
          halfPhi = 0.5 * (std::max(phi1, phi2) - std::min(phi1, phi2));
          avgPhi = 0.5 * (phi1 + phi2);
        }
      }
      bounds = std::make_shared<RadialBounds>(minR, maxR, halfPhi, avgPhi);
      thickness = 2 * halfZ;
    }
  }
  return {bounds, transform, thickness};
}

std::tuple<std::shared_ptr<const Acts::PlanarBounds>, const Acts::Transform3,
           double>
Acts::TGeoSurfaceConverter::planeComponents(const TGeoShape& tgShape,
                                            const Double_t* rotation,
                                            const Double_t* translation,
                                            const std::string& axes,
                                            double scalor) noexcept(false) {
  // Create translation and rotation
  Vector3 t(scalor * translation[0], scalor * translation[1],
            scalor * translation[2]);
  Vector3 ax(rotation[0], rotation[3], rotation[6]);
  Vector3 ay(rotation[1], rotation[4], rotation[7]);
  Vector3 az(rotation[2], rotation[5], rotation[8]);

  std::shared_ptr<const PlanarBounds> bounds = nullptr;

  // Check if it's a box - always true, hence last ressort
  const TGeoBBox* box = dynamic_cast<const TGeoBBox*>(&tgShape);

  // Check if it's a trapezoid2
  const TGeoTrd1* trapezoid1 = dynamic_cast<const TGeoTrd1*>(&tgShape);
  if ((trapezoid1 != nullptr) && !boost::istarts_with(axes, "XZ")) {
    throw std::invalid_argument(
        "TGeoTrd1 -> PlaneSurface: can only be converted with '(x/X)(z/Z)(*)' "
        "axes");
  }

  // Check if it's a trapezoid2
  const TGeoTrd2* trapezoid2 = dynamic_cast<const TGeoTrd2*>(&tgShape);
  if (trapezoid2 != nullptr) {
    if (!boost::istarts_with(axes, "X") &&
        std::abs(trapezoid2->GetDx1() - trapezoid2->GetDx2()) > s_epsilon) {
      throw std::invalid_argument(
          "TGeoTrd2 -> PlaneSurface: dx1 must be be equal to dx2 if not taken "
          "as trapezoidal side.");
    } else if (!boost::istarts_with(axes, "Y") &&
               std::abs(trapezoid2->GetDy1() - trapezoid2->GetDy2()) >
                   s_epsilon) {
      throw std::invalid_argument(
          "TGeoTrd2 -> PlaneSurface: dy1 must be be equal to dy2 if not taken "
          "as trapezoidal side.");
    }
    // Not allowed
    if (boost::istarts_with(axes, "XY") || boost::istarts_with(axes, "YX")) {
      throw std::invalid_argument(
          "TGeoTrd2 -> PlaneSurface: only works with (x/X)(z/Z) and "
          "(y/Y)(z/Z).");
    }
  }

  // Check if it's a Arb8
  const TGeoArb8* polygon8c = dynamic_cast<const TGeoArb8*>(&tgShape);
  TGeoArb8* polygon8 = nullptr;
  if (polygon8c != nullptr) {
    // Needed otherwise you can access GetVertices
    polygon8 = const_cast<TGeoArb8*>(polygon8c);
  }

  if ((polygon8c != nullptr) &&
      !(boost::istarts_with(axes, "XY") || boost::istarts_with(axes, "YX"))) {
    throw std::invalid_argument(
        "TGeoArb8 -> PlaneSurface: dz must be normal component of Surface.");
  }

  // The thickness will be filled
  double thickness = 0.;

  // The sign of the axes
  int xs = std::islower(axes.at(0)) != 0 ? -1 : 1;
  int ys = std::islower(axes.at(1)) != 0 ? -1 : 1;

  // Set up the columns : only cyclic iterations are allowed
  Vector3 cx = xs * ax;
  Vector3 cy = ys * ay;
  if (boost::istarts_with(axes, "XY")) {
    if (trapezoid2 != nullptr) {
      double dx1 = (ys < 0) ? trapezoid1->GetDx2() : trapezoid1->GetDx1();
      double dx2 = (ys < 0) ? trapezoid1->GetDx1() : trapezoid1->GetDx2();
      bounds = std::make_shared<const TrapezoidBounds>(
          scalor * dx1, scalor * dx2, scalor * trapezoid2->GetDy1());
      thickness = 2 * scalor * trapezoid2->GetDz();
    } else if (polygon8 != nullptr) {
      Double_t* tgverts = polygon8->GetVertices();
      std::vector<Vector2> pVertices;
      for (unsigned int ivtx = 0; ivtx < 4; ++ivtx) {
        pVertices.push_back(Vector2(scalor * xs * tgverts[ivtx * 2],
                                    scalor * ys * tgverts[ivtx * 2 + 1]));
      }
      bounds = std::make_shared<ConvexPolygonBounds<4>>(pVertices);
      thickness = 2 * scalor * polygon8->GetDz();
    } else if (box != nullptr) {
      bounds = std::make_shared<const RectangleBounds>(scalor * box->GetDX(),
                                                       scalor * box->GetDY());
      thickness = 2 * scalor * box->GetDZ();
    }
  } else if (boost::istarts_with(axes, "YZ")) {
    cx = xs * ay;
    cy = ys * az;
    if (trapezoid1 != nullptr) {
      throw std::invalid_argument(
          "TGeoTrd1 can only be converted with '(x/X)(z/Z)(y/Y)' axes");
    } else if (trapezoid2 != nullptr) {
      double dx1 = (ys < 0) ? trapezoid2->GetDy2() : trapezoid2->GetDy1();
      double dx2 = (ys < 0) ? trapezoid2->GetDy1() : trapezoid2->GetDy2();
      bounds = std::make_shared<const TrapezoidBounds>(
          scalor * dx1, scalor * dx2, scalor * trapezoid2->GetDz());
      thickness = 2 * scalor * trapezoid2->GetDx1();
    } else if (box != nullptr) {
      bounds = std::make_shared<const RectangleBounds>(scalor * box->GetDY(),
                                                       scalor * box->GetDZ());
      thickness = 2 * scalor * box->GetDX();
    }
  } else if (boost::istarts_with(axes, "ZX")) {
    cx = xs * az;
    cy = ys * ax;
    if (box != nullptr) {
      bounds = std::make_shared<const RectangleBounds>(scalor * box->GetDZ(),
                                                       scalor * box->GetDX());
      thickness = 2 * scalor * box->GetDY();
    }
  } else if (boost::istarts_with(axes, "XZ")) {
    cx = xs * ax;
    cy = ys * az;
    if (trapezoid1 != nullptr) {
      double dx1 = (ys < 0) ? trapezoid1->GetDx2() : trapezoid1->GetDx1();
      double dx2 = (ys < 0) ? trapezoid1->GetDx1() : trapezoid1->GetDx2();
      bounds = std::make_shared<const TrapezoidBounds>(
          scalor * dx1, scalor * dx2, scalor * trapezoid1->GetDz());
      thickness = 2 * scalor * trapezoid1->GetDy();
    } else if (trapezoid2 != nullptr) {
      double dx1 = (ys < 0) ? trapezoid2->GetDx2() : trapezoid2->GetDx1();
      double dx2 = (ys < 0) ? trapezoid2->GetDx1() : trapezoid2->GetDx2();
      bounds = std::make_shared<const TrapezoidBounds>(
          scalor * dx1, scalor * dx2, scalor * trapezoid2->GetDz());
      thickness = 2 * scalor * trapezoid2->GetDy1();
    } else if (box != nullptr) {
      bounds = std::make_shared<const RectangleBounds>(scalor * box->GetDX(),
                                                       scalor * box->GetDZ());
      thickness = 2 * scalor * box->GetDY();
    }
  } else if (boost::istarts_with(axes, "YX")) {
    cx = xs * ay;
    cy = ys * ax;
    if (trapezoid2 != nullptr) {
      double dx1 = (ys < 0) ? trapezoid2->GetDy2() : trapezoid2->GetDy1();
      double dx2 = (ys < 0) ? trapezoid2->GetDy1() : trapezoid2->GetDy2();
      bounds = std::make_shared<const TrapezoidBounds>(
          scalor * dx1, scalor * dx2, scalor * trapezoid2->GetDx1());
      thickness = 2 * scalor * trapezoid2->GetDz();
    } else if (polygon8 != nullptr) {
      const Double_t* tgverts = polygon8->GetVertices();
      std::vector<Vector2> pVertices;
      for (unsigned int ivtx = 0; ivtx < 4; ++ivtx) {
        pVertices.push_back(Vector2(scalor * xs * tgverts[ivtx * 2 + 1],
                                    scalor * ys * tgverts[ivtx * 2]));
      }
      bounds = std::make_shared<ConvexPolygonBounds<4>>(pVertices);
      thickness = 2 * scalor * polygon8->GetDz();
    } else if (box != nullptr) {
      bounds = std::make_shared<const RectangleBounds>(scalor * box->GetDY(),
                                                       scalor * box->GetDX());
      thickness = 2 * scalor * box->GetDZ();
    }
  } else if (boost::istarts_with(axes, "ZY")) {
    cx = xs * az;
    cy = ys * ay;
    if (box != nullptr) {
      bounds = std::make_shared<const RectangleBounds>(scalor * box->GetDZ(),
                                                       scalor * box->GetDY());
      thickness = 2 * scalor * box->GetDX();
    }
  } else {
    throw std::invalid_argument(
        "TGeoConverter: axes definition must be permutation of "
        "'(x/X)(y/Y)(z/Z)'");
  }

  // Create the normal vector & the transform
  auto cz = cx.cross(cy);
  auto transform = TGeoPrimitivesHelper::makeTransform(cx, cy, cz, t);

  return {bounds, transform, thickness};
}

std::tuple<std::shared_ptr<Acts::Surface>, double>
Acts::TGeoSurfaceConverter::toSurface(const TGeoShape& tgShape,
                                      const TGeoMatrix& tgMatrix,
                                      const std::string& axes,
                                      double scalor) noexcept(false) {
  // Get the placement and orientation in respect to its mother
  const Double_t* rotation = tgMatrix.GetRotationMatrix();
  const Double_t* translation = tgMatrix.GetTranslation();

  auto [cBounds, cTransform, cThickness] =
      cylinderComponents(tgShape, rotation, translation, axes, scalor);
  if (cBounds != nullptr) {
    return {Surface::makeShared<CylinderSurface>(cTransform, cBounds),
            cThickness};
  }

  auto [dBounds, dTransform, dThickness] =
      discComponents(tgShape, rotation, translation, axes, scalor);
  if (dBounds != nullptr) {
    return {Surface::makeShared<DiscSurface>(dTransform, dBounds), dThickness};
  }

  auto [pBounds, pTransform, pThickness] =
      planeComponents(tgShape, rotation, translation, axes, scalor);
  if (pBounds != nullptr) {
    return {Surface::makeShared<PlaneSurface>(pTransform, pBounds), pThickness};
  }

  return {nullptr, 0.};
}
