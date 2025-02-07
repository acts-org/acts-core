// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

#include "Acts/Definitions/Algebra.hpp"
#include "Acts/MagneticField/MagneticFieldProvider.hpp"

#include <memory>

namespace dd4hep {
class OverlayedField;
}

namespace Acts {

class DD4hepFieldAdapter : public Acts::MagneticFieldProvider {
  struct Cache {};

 public:
  explicit DD4hepFieldAdapter(dd4hep::OverlayedField field);

  MagneticFieldProvider::Cache makeCache(
      const Acts::MagneticFieldContext& mctx) const override;

  Result<Vector3> getField(const Vector3& position,
                           MagneticFieldProvider::Cache& cache) const override;

 private:
  double m_fieldConversionFactor;
  double m_lengthConversionFactor;
  std::unique_ptr<dd4hep::OverlayedField> m_field;
};

}  // namespace Acts
