// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

/// Get one element of a 2 dimensional matrix
#define ACTS_CUDA_MATRIX2D_ELEMENT(ARRAYPTR, SIZEX, SIZEY, INDEXX, INDEXY) \
  ARRAYPTR[INDEXX + INDEXY * SIZEX]

/// Get one element of a 3 dimensional matrix
#define ACTS_CUDA_MATRIX3D_ELEMENT(ARRAYPTR, SIZEX, SIZEY, SIZEZ, INDEXX, \
                                   INDEXY, INDEXZ)                        \
  ARRAYPTR[INDEXX + INDEXY * SIZEX + INDEXZ * SIZEX * SIZEY]
