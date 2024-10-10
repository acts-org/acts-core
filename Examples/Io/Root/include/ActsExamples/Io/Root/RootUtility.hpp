// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <algorithm>

#include <TMathBase.h>

namespace ActsExamples::RootUtility {

/// @brief Sorts an array of elements and outputs the indices of the sorted elements.
///
/// This function sorts an array `elements` containing `numElements` of generic
/// type `element_t`. It outputs an array `sortedIndices` of type `index_t` that
/// contains the indices of `elements` in sorted order. The sort order is
/// determined by the `sortDescending` flag; if `true`, the array is sorted in
/// descending order, otherwise in ascending order.
///
/// This is a stable version of `TMath::Sort` that preserves the relative order
/// of equal elements.
///
/// @tparam element_t The data type of the array elements to be sorted.
/// @tparam index_t The data type for indexing and counting elements in the arrays.
///
/// @param numElements The number of elements in the `elements` array.
/// @param elements Pointer to the array of type `element_t`
/// @param sortedIndices Pointer to an array of `index_t` type where the sorted indices will be stored.
/// @param sortDescending Boolean flag indicating the sort order. `true` for descending, `false` for ascending.
///
/// @note It is the caller's responsibility to ensure that the `sortedIndices` array is pre-allocated
/// with a length of at least `numElements`. Furthermore, the types of
/// `numElements` and `sortedIndices` must be consistent.
template <typename element_t, typename index_t>
void stableSort(index_t numElements, const element_t* elements,
                index_t* sortedIndices, Bool_t sortDescending) {
  for (index_t i = 0; i < numElements; i++) {
    sortedIndices[i] = i;
  }

  if (sortDescending) {
    std::stable_sort(sortedIndices, sortedIndices + numElements,
                     CompareDesc<const element_t*>(elements));
  } else {
    std::stable_sort(sortedIndices, sortedIndices + numElements,
                     CompareAsc<const element_t*>(elements));
  }
}

}  // namespace ActsExamples::RootUtility
