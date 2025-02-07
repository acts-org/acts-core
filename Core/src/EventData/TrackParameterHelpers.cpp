// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/EventData/TrackParameterHelpers.hpp"

#include "Acts/Utilities/AngleHelpers.hpp"
#include "Acts/Utilities/VectorHelpers.hpp"

#include <numbers>

bool Acts::isBoundVectorValid(const BoundVector& v, bool validateAngleRange,
                              double epsilon, double maxAbsEta) {
  constexpr auto pi = std::numbers::pi_v<double>;

  bool loc0Valid = std::isfinite(v[eBoundLoc0]);
  bool loc1Valid = std::isfinite(v[eBoundLoc1]);
  bool phiValid = std::isfinite(v[eBoundPhi]);
  bool thetaValid = std::isfinite(v[eBoundTheta]);
  bool qOverPValid = std::isfinite(v[eBoundQOverP]);
  bool timeValid = std::isfinite(v[eBoundTime]);

  if (validateAngleRange) {
    phiValid = phiValid && (v[eBoundPhi] + epsilon >= -pi) &&
               (v[eBoundPhi] - epsilon < pi);
    thetaValid = thetaValid && (v[eBoundTheta] + epsilon >= 0) &&
                 (v[eBoundTheta] - epsilon <= pi);
  }

  bool etaValid = true;
  if (std::isfinite(maxAbsEta)) {
    double eta = AngleHelpers::etaFromTheta(v[eBoundTheta]);
    etaValid = std::isfinite(eta) && (std::abs(eta) - epsilon <= maxAbsEta);
  }

  return loc0Valid && loc1Valid && phiValid && thetaValid && qOverPValid &&
         timeValid && etaValid;
}

bool Acts::isFreeVectorValid(const FreeVector& v, double epsilon,
                             double maxAbsEta) {
  bool pos0Valid = std::isfinite(v[eFreePos0]);
  bool pos1Valid = std::isfinite(v[eFreePos1]);
  bool pos2Valid = std::isfinite(v[eFreePos2]);
  bool dir0Valid = std::isfinite(v[eFreeDir0]);
  bool dir1Valid = std::isfinite(v[eFreeDir1]);
  bool dir2Valid = std::isfinite(v[eFreeDir2]);
  bool dirValid = (std::abs(v.segment<3>(eFreeDir0).norm() - 1) - epsilon <= 0);
  bool qOverPValid = std::isfinite(v[eFreeQOverP]);
  bool timeValid = std::isfinite(v[eFreeTime]);

  bool etaValid = true;
  if (std::isfinite(maxAbsEta)) {
    double eta = VectorHelpers::eta(v.segment<3>(eFreeDir0));
    etaValid = std::isfinite(eta) && (std::abs(eta) - epsilon <= maxAbsEta);
  }

  return pos0Valid && pos1Valid && pos2Valid && dir0Valid && dir1Valid &&
         dir2Valid && dirValid && qOverPValid && timeValid && etaValid;
}
