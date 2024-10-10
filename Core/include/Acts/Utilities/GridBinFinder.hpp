// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Concepts.hpp"
#include "Acts/Utilities/Grid.hpp"
#include "Acts/Utilities/Holders.hpp"
#include "Acts/Utilities/detail/grid_helper.hpp"

#include <variant>
#include <vector>

#include <boost/container/small_vector.hpp>

namespace Acts {

/// @class BinFinder
/// @tparam DIM Dimension of the Grid on which the GridBinFinder will be used
///
/// The BinFinder is used by the ISPGroupSelector. It can be
/// used to find both bins that could be bottom bins as well as bins that could
/// be top bins, which are assumed to be the same bins. Does not take
/// interaction region into account to limit z-bins.
template <std::size_t DIM>
class GridBinFinder {
 public:
  static constexpr std::size_t dimCubed = Acts::detail::ipow(3, DIM);
  /// @brief Constructor
  /// @tparam args ... Input parameters provided by the user
  ///
  /// @param [in] vals The input parameters that define how many neighbours we need to find
  ///
  /// @pre The provided paramers must be of type 'int', 'std::pair<int, int>' or 'std::vector<std::pair<int, int>>'
  /// no other type is allowed. The order of these parameters must correspond to
  /// the same ordering of the axes in the grid
  template <typename... args>
  explicit GridBinFinder(args&&... vals)
    requires(
        sizeof...(args) == DIM &&
        (Concepts::same_as_any_of<std::decay_t<args>, int, std::pair<int, int>,
                                  std::vector<std::pair<int, int>>> &&
         ...))
  {
    storeValue(std::forward<args>(vals)...);
  }

  /// @brief Retrieve the neighbouring bins given a local position in the grid
  ///
  /// Return all bins that could contain space points that can be used with the
  /// space points in the bin with the provided indices to create seeds.
  ///
  /// @tparam stored_t The type of elements stored in the Grid
  /// @tparam Axes ... The type of the axes of the grid
  ///
  /// @param [in] locPosition The N-dimentional local position in the grid
  /// @param [in] grid The grid
  /// @return The list of neighbouring bins
  ///
  /// @pre The provided local position must be a valid local bins configuration in the grid
  template <typename stored_t, class... Axes>
  boost::container::small_vector<std::size_t, dimCubed> findBins(
      const std::array<std::size_t, DIM>& locPosition,
      const Acts::Grid<stored_t, Axes...>& grid) const;

 private:
  /// @brief Store the values provided by the user for each axis in the grid
  /// @tparam first_value_t Type of the first value
  /// @tparam vals ... values of the remaining values
  ///
  /// @param [in] fv The first value in the list
  /// @param [in] others The remaining values in the list
  ///
  /// @pre both first_value_t and vals ... can be only int or std::vector<std::pair<int, int>>
  /// In the second case, the number of entries of the vector of pairs MUST be
  /// equal to the number of bins in that specific axis. Empty vectors are also
  /// allowed but in this case the value will be replaced with a 1 (integer),
  /// thus instructing the code to look for neighbours in the range {-1 ,1}
  template <typename first_value_t, typename... vals>
  void storeValue(first_value_t&& fv, vals&&... others);

  /// @brief Get the instructions for retrieving the neighbouring bins given a local position
  ///
  /// @param [in] locPosition The requested local position
  /// @return the instructions for retrieving the neighbouring bins for this local position
  ///
  /// @pre The local position must be a valid local bins configuration for the grid
  std::array<std::pair<int, int>, DIM> getSizePerAxis(
      const std::array<std::size_t, DIM>& locPosition) const;

  /// @brief Check the GridBinFinder configuration is compatible with the grid
  /// by checking the values of m_values against the axes of the grid
  /// This function is called only in debug mode
  ///
  /// @tparam stored_t The type of elements stored in the Grid
  /// @tparam Axes ... The type of the axes of the grid
  ///
  /// @param [in] grid The Grid
  /// @return If the GridBinFinder is compatible with the grid
  template <typename stored_t, class... Axes>
  bool isGridCompatible(const Acts::Grid<stored_t, Axes...>& grid) const;

 private:
  using stored_values_t =
      std::variant<int, std::pair<int, int>, std::vector<std::pair<int, int>>>;
  /// @brief the instructions for retrieving the nieghbouring bins for each given axis in the grid
  /// These values are provided by the user and can be ints, a pair of ints or a
  /// vector of pair of ints. In the first case, the neighbours will be +/- bins
  /// from the given local bin In the second case, the user defines how many
  /// bins in both directions should be provided
  ///
  /// @pre The list of entries of the vector of pairs MUST be equal to the number of bins in that specific
  /// axis. Empty vectors are also allowed  but in this case the value will be
  /// replaced with a 1 (integer), thus instructing the code to look for
  /// neighbours in the range {-1 ,1}
  std::array<stored_values_t, DIM> m_values{};
};

}  // namespace Acts
#include "Acts/Utilities/GridBinFinder.ipp"
