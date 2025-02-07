// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Surfaces/detail/AnnulusBoundsHelper.hpp"

#include "Acts/Surfaces/AnnulusBounds.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <numbers>

namespace Acts::detail {

std::tuple<std::shared_ptr<AnnulusBounds>, Transform3>
AnnulusBoundsHelper::create(const Transform3& transform, double rMin,
                            double rMax, std::vector<Vector2> vertices) {
  using Line2D = Eigen::Hyperplane<double, 2>;

  // Construct the bound lines
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

  Line2D lA = Line2D::Through(boundLines[0].first, boundLines[0].second);
  Line2D lB = Line2D::Through(boundLines[1].first, boundLines[1].second);
  Vector2 ix = lA.intersection(lB);

  const Eigen::Translation3d originTranslation(ix.x(), ix.y(), 0.);
  const Vector2 originShift = -ix;

  // Update transform by prepending the origin shift translation
  Transform3 boundsTransform = transform * originTranslation;
  // Transform phi line point to new origin and get phi
  double phi1 = VectorHelpers::phi(boundLines[0].second - boundLines[0].first);
  double phi2 = VectorHelpers::phi(boundLines[1].second - boundLines[1].first);
  double phiMax = std::max(phi1, phi2);
  double phiMin = std::min(phi1, phi2);
  double phiShift = 0.;

  // Create the bounds
  auto annulusBounds = std::make_shared<AnnulusBounds>(
      rMin, rMax, phiMin, phiMax, originShift, phiShift);

  return {annulusBounds, boundsTransform};
}

}  // namespace Acts::detail
