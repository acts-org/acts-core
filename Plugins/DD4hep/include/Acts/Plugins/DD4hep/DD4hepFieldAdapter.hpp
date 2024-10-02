// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/MagneticField/MagneticFieldProvider.hpp"

#include <memory>

namespace dd4hep {
class OverlayedField;
}

namespace Acts {

class DD4hepFieldAdapter : public Acts::MagneticFieldProvider {
  struct Cache {};

 public:
  DD4hepFieldAdapter(dd4hep::OverlayedField field);

  MagneticFieldProvider::Cache makeCache(
      const Acts::MagneticFieldContext& mctx) const override;

  Result<Vector3> getField(const Vector3& position,
                           MagneticFieldProvider::Cache& cache) const override;

  Result<Vector3> getFieldGradient(
      const Vector3& position, ActsMatrix<3, 3>& derivative,
      MagneticFieldProvider::Cache& cache) const override;

 private:
  double m_fieldConversionFactor;
  double m_lengthConversionFactor;
  std::unique_ptr<dd4hep::OverlayedField> m_field;
};

}  // namespace Acts
