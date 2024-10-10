// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/Algebra.hpp"

#include <bitset>
#include <optional>

#include "Eigen/Dense"

namespace Acts {

/// Convert a bitset to a matrix of integers, with each element set to the bit
/// value.
/// @note How the bits are assigned to matrix elements depends on the storage
/// type of the matrix being converted (row-major or col-major)
/// @tparam MatrixType Matrix type that is produced
/// @param bs The bitset to convert
/// @return A matrix with the integer values of the bits from @p bs
template <typename MatrixType>
MatrixType bitsetToMatrix(const std::bitset<MatrixType::RowsAtCompileTime *
                                            MatrixType::ColsAtCompileTime>
                              bs) {
  constexpr int rows = MatrixType::RowsAtCompileTime;
  constexpr int cols = MatrixType::ColsAtCompileTime;

  static_assert(rows != -1 && cols != -1,
                "bitsetToMatrix does not support dynamic matrices");

  MatrixType m;
  auto* p = m.data();
  for (std::size_t i = 0; i < rows * cols; i++) {
    p[i] = bs[rows * cols - 1 - i];
  }
  return m;
}

/// Convert an integer matrix to a bitset.
/// @note How the bits are ordered depends on the storage type of the matrix
/// being converted (row-major or col-major)
/// @tparam Derived Eigen base concrete type
/// @param m Matrix that is converted
/// @return The converted bitset.
template <typename Derived>
auto matrixToBitset(const Eigen::PlainObjectBase<Derived>& m) {
  using MatrixType = Eigen::PlainObjectBase<Derived>;
  constexpr std::size_t rows = MatrixType::RowsAtCompileTime;
  constexpr std::size_t cols = MatrixType::ColsAtCompileTime;

  std::bitset<rows * cols> res;

  auto* p = m.data();
  for (std::size_t i = 0; i < rows * cols; i++) {
    res[rows * cols - 1 - i] = static_cast<bool>(p[i]);
  }

  return res;
}

/// @brief Perform a blocked matrix multiplication, avoiding Eigen GEMM
/// methods.
///
/// @tparam A The type of the first matrix, which should be MxN
/// @tparam B The type of the second matrix, which should be NxP
///
/// @param[in] a An MxN matrix of type A
/// @param[in] b An NxP matrix of type P
///
/// @returns The product ab
template <typename A, typename B>
inline ActsMatrix<A::RowsAtCompileTime, B::ColsAtCompileTime> blockedMult(
    const A& a, const B& b) {
  // Extract the sizes of the matrix types that we receive as template
  // parameters.
  constexpr int M = A::RowsAtCompileTime;
  constexpr int N = A::ColsAtCompileTime;
  constexpr int P = B::ColsAtCompileTime;

  // Ensure that the second dimension of our first matrix equals the first
  // dimension of the second matrix, otherwise we cannot multiply.
  static_assert(N == B::RowsAtCompileTime);

  if constexpr (M <= 4 && N <= 4 && P <= 4) {
    // In cases where the matrices being multiplied are small, we can rely on
    // Eigen do to a good job, and we don't really need to do any blocking.
    return a * b;
  } else {
    // Here, we want to calculate the expression: C = AB, Eigen, natively,
    // doesn't do a great job at this if the matrices A and B are large
    // (roughly M >= 8, N >= 8, or P >= 8), and applies a slow GEMM operation.
    // We apply a blocked matrix multiplication operation to decompose the
    // multiplication into smaller operations, relying on the fact that:
    //
    // ┌         ┐   ┌         ┐ ┌         ┐
    // │ C₁₁ C₁₂ │ = │ A₁₁ A₁₂ │ │ B₁₁ B₁₂ │
    // │ C₂₁ C₂₂ │ = │ A₂₁ A₂₂ │ │ B₂₁ B₂₂ │
    // └         ┘   └         ┘ └         ┘
    //
    // where:
    //
    // C₁₁ = A₁₁ * B₁₁ + A₁₂ * B₂₁
    // C₁₂ = A₁₁ * B₁₂ + A₁₂ * B₂₂
    // C₂₁ = A₂₁ * B₁₁ + A₂₂ * B₂₁
    // C₂₂ = A₂₁ * B₁₂ + A₂₂ * B₂₂
    //
    // The sizes of these submatrices are roughly half (in each dimension) that
    // of the parent matrix. If the size of the parent matrix is even, we can
    // divide it exactly, If the size of the parent matrix is odd, then some
    // of the submatrices will be one larger than the others. In general, for
    // any matrix Q, the sizes of the submatrices are (where / denotes integer
    // division):
    //
    // Q₁₁ : M / 2 × P / 2
    // Q₁₂ : M / 2 × (P + 1) / 2
    // Q₂₁ : (M + 1) / 2 × P / 2
    // Q₂₂ : (M + 1) / 2 × (P + 1) / 2
    //
    // See https://csapp.cs.cmu.edu/public/waside/waside-blocking.pdf for a
    // more in-depth explanation of blocked matrix multiplication.
    constexpr int M1 = M / 2;
    constexpr int M2 = (M + 1) / 2;
    constexpr int N1 = N / 2;
    constexpr int N2 = (N + 1) / 2;
    constexpr int P1 = P / 2;
    constexpr int P2 = (P + 1) / 2;

    // Construct the end result in this matrix, which destroys a few of Eigen's
    // built-in optimization techniques, but sadly this is necessary.
    ActsMatrix<M, P> r;

    // C₁₁ = A₁₁ * B₁₁ + A₁₂ * B₂₁
    r.template topLeftCorner<M1, P1>().noalias() =
        a.template topLeftCorner<M1, N1>() *
            b.template topLeftCorner<N1, P1>() +
        a.template topRightCorner<M1, N2>() *
            b.template bottomLeftCorner<N2, P1>();

    // C₁₂ = A₁₁ * B₁₂ + A₁₂ * B₂₂
    r.template topRightCorner<M1, P2>().noalias() =
        a.template topLeftCorner<M1, N1>() *
            b.template topRightCorner<N1, P2>() +
        a.template topRightCorner<M1, N2>() *
            b.template bottomRightCorner<N2, P2>();

    // C₂₁ = A₂₁ * B₁₁ + A₂₂ * B₂₁
    r.template bottomLeftCorner<M2, P1>().noalias() =
        a.template bottomLeftCorner<M2, N1>() *
            b.template topLeftCorner<N1, P1>() +
        a.template bottomRightCorner<M2, N2>() *
            b.template bottomLeftCorner<N2, P1>();

    // C₂₂ = A₂₁ * B₁₂ + A₂₂ * B₂₂
    r.template bottomRightCorner<M2, P2>().noalias() =
        a.template bottomLeftCorner<M2, N1>() *
            b.template topRightCorner<N1, P2>() +
        a.template bottomRightCorner<M2, N2>() *
            b.template bottomRightCorner<N2, P2>();

    return r;
  }
}

/// FPE "safe" functions
///
/// Our main motivation for this is that users might have a strict FPE policy
/// which would flag every single occurrence as a failure and then somebody has
/// to investigate. Since we are processing a high number of events and floating
/// point numbers sometimes work in mysterious ways the caller of this function
/// might want to hide FPEs and handle them in a more controlled way.

/// Calculate the inverse of an Eigen matrix after checking if it can be
/// numerically inverted. This allows to catch potential FPEs before they occur.
/// For matrices up to 4x4, the inverse is computed directly. For larger
/// matrices, the FullPivLU is used.
///
/// @tparam Derived Eigen derived concrete type
/// @tparam Result Eigen result type defaulted to input type
///
/// @param m Eigen matrix to invert
///
/// @return The theta value
template <typename MatrixType, typename ResultType = MatrixType>
std::optional<ResultType> safeInverse(const MatrixType& m) noexcept {
  constexpr int rows = MatrixType::RowsAtCompileTime;
  constexpr int cols = MatrixType::ColsAtCompileTime;

  static_assert(rows == cols);
  static_assert(rows != -1);

  ResultType result;
  bool invertible = false;

  if constexpr (rows > 4) {
    Eigen::FullPivLU<MatrixType> mFullPivLU(m);
    if (mFullPivLU.isInvertible()) {
      invertible = true;
      result = mFullPivLU.inverse();
    }
  } else {
    m.computeInverseWithCheck(result, invertible);
  }

  if (invertible) {
    return result;
  }

  return std::nullopt;
}

/// Specialization of the exponent limit to be used for safe exponential,
/// depending on the floating point type.
/// See https://godbolt.org/z/z53Er6Mzf for reasoning for the concrete numbers.
template <typename T>
struct ExpSafeLimit {};
template <>
struct ExpSafeLimit<double> {
  constexpr static double value = 500.0;
};
template <>
struct ExpSafeLimit<float> {
  constexpr static float value = 50.0;
};

/// Calculate the exponential function while avoiding FPEs.
///
/// @param val argument for which the exponential function should be evaluated.
///
/// @return 0 in the case of underflow, std::numeric_limits<T>::infinity in the
/// case of overflow, std::exp(val) else
template <typename T>
constexpr T safeExp(T val) noexcept {
  constexpr T maxExponent = ExpSafeLimit<T>::value;
  constexpr T minExponent = -maxExponent;
  if (val < minExponent) {
    return 0.0;
  }

  if (val > maxExponent) {
    return std::numeric_limits<T>::infinity();
  }

  return std::exp(val);
}

}  // namespace Acts
