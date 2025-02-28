// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <Eigen/Dense>

const char* __acts_internal_get_eigen_alignment_str() {
  static constexpr char _ACTS_EIGEN_ALIGN_STR[] = {
      'A',
      'C',
      'T',
      'S',
      ':',
      ':',
      'E',
      'i',
      'g',
      'e',
      'n',
      'A',
      'l',
      'i',
      'g',
      'n',
      'm',
      'e',
      'n',
      't',
      '=',
      ('0' + ((EIGEN_MAX_ALIGN_BYTES / 1000) % 10)),
      ('0' + ((EIGEN_MAX_ALIGN_BYTES / 100) % 10)),
      ('0' + ((EIGEN_MAX_ALIGN_BYTES / 10) % 10)),
      ('0' + ((EIGEN_MAX_ALIGN_BYTES) % 10))};

  return _ACTS_EIGEN_ALIGN_STR;
}
