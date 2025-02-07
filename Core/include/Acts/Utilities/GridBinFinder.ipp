// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include <type_traits>

template <std::size_t DIM>
template <typename first_value_t, typename... vals>
void Acts::GridBinFinder<DIM>::storeValue(first_value_t&& fv,
                                          vals&&... others) {
  constexpr std::size_t N = sizeof...(vals);
  static_assert(N < DIM);
  /// Check the fist value is reasonable
  using decayed_value_t = typename std::decay_t<first_value_t>;
  if constexpr (std::is_same_v<int, decayed_value_t>) {
    /// if int -> value is positive
    assert(fv >= 0);
    m_values[DIM - N - 1ul] = fv;
  } else if constexpr (std::is_same_v<std::pair<int, int>, decayed_value_t>) {
    m_values[DIM - N - 1ul] = fv;
  } else {
    /// If vector of pairs -> also allow for empty vectors
    if (!fv.empty()) {
      m_values[DIM - N - 1ul] = std::forward<first_value_t>(fv);
    } else {
      m_values[DIM - N - 1ul] = 1;
    }
  }
  if constexpr (N != 0ul) {
    storeValue(std::forward<vals>(others)...);
  }
}

template <std::size_t DIM>
std::array<std::pair<int, int>, DIM> Acts::GridBinFinder<DIM>::getSizePerAxis(
    const std::array<std::size_t, DIM>& locPosition) const {
  std::array<std::pair<int, int>, DIM> output;
  for (std::size_t i(0ul); i < DIM; ++i) {
    output[i] = std::visit(
        [&locPosition, i](const auto& val) -> std::pair<int, int> {
          using value_t = typename std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<int, value_t>) {
            assert(val >= 0);
            return {-val, val};
          } else if constexpr (std::is_same_v<std::pair<int, int>, value_t>) {
            return {-val.first, val.second};
          } else {
            assert(locPosition.size() > i);
            assert(locPosition[i] > 0ul);
            assert(val.size() >= locPosition[i]);
            return val[locPosition[i] - 1ul];
          }
        },
        m_values[i]);
  }
  return output;
}

template <std::size_t DIM>
template <typename stored_t, class... Axes>
auto Acts::GridBinFinder<DIM>::findBins(
    const std::array<std::size_t, DIM>& locPosition,
    const Acts::Grid<stored_t, Axes...>& grid) const
    -> boost::container::small_vector<std::size_t, dimCubed> {
  static_assert(sizeof...(Axes) == DIM);
  assert(isGridCompatible(grid));
  std::array<std::pair<int, int>, DIM> sizePerAxis =
      getSizePerAxis(locPosition);
  return grid.neighborHoodIndices(locPosition, sizePerAxis).collect();
}

template <std::size_t DIM>
template <typename stored_t, class... Axes>
bool Acts::GridBinFinder<DIM>::isGridCompatible(
    const Acts::Grid<stored_t, Axes...>& grid) const {
  const std::array<std::size_t, DIM> nLocBins = grid.numLocalBins();
  for (std::size_t i(0ul); i < DIM; ++i) {
    std::size_t nBins = nLocBins[i];
    bool isCompabile = std::visit(
        [nBins](const auto& val) -> bool {
          using value_t = typename std::decay_t<decltype(val)>;
          if constexpr (std::is_same_v<int, value_t> ||
                        std::is_same_v<std::pair<int, int>, value_t>) {
            return true;
          } else {
            return val.size() == nBins;
          }
        },
        m_values[i]);
    if (!isCompabile) {
      return false;
    }
  }
  return true;
}
