// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

// CUDA plugin include(s).
#include "Acts/Plugins/Cuda/Seeding2/TripletFilterConfig.hpp"

/// Function setting up the pointers to the custom seed filtering functions
Acts::Cuda::TripletFilterConfig testDeviceCuts();
