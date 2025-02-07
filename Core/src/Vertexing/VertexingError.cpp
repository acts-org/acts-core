// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Vertexing/VertexingError.hpp"

#include <string>

namespace {

class VertexingErrorCategory : public std::error_category {
 public:
  // Return a short descriptive name for the category.
  const char* name() const noexcept final { return "KalmanFitterError"; }

  // Return what each enum means in text.
  std::string message(int c) const final {
    using Acts::VertexingError;

    switch (static_cast<VertexingError>(c)) {
      case VertexingError::NumericFailure:
        return "Numeric failure in calculation.";
      case VertexingError::EmptyInput:
        return "Empty input provided.";
      case VertexingError::SeedingError:
        return "Error while finding vertex seed.";
      case VertexingError::NotConverged:
        return "Unable to converge.";
      case VertexingError::ElementNotFound:
        return "Unable to find element.";
      case VertexingError::NoCovariance:
        return "No covariance provided.";
      case VertexingError::SingularMatrix:
        return "Encountered non-invertible matrix.";
      case VertexingError::NonPositiveVariance:
        return "Encountered negative or zero variance.";
      case VertexingError::MatrixNotPositiveDefinite:
        return "Encountered a matrix that is not positive definite.";
      case VertexingError::InvalidInput:
        return "Invalid input provided.";
      default:
        return "unknown";
    }
  }
};

}  // namespace

std::error_code Acts::make_error_code(Acts::VertexingError e) {
  static VertexingErrorCategory c;
  return {static_cast<int>(e), c};
}
