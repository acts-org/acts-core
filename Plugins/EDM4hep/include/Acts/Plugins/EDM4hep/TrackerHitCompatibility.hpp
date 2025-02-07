// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

// Compatibility with EDM4hep < 0.99 and >= 0.99
#if __has_include(<edm4hep/TrackerHit3D.h>)
#include "edm4hep/TrackerHit3D.h"
#include "edm4hep/TrackerHit3DCollection.h"
#else
#include "edm4hep/TrackerHit.h"
#include "edm4hep/TrackerHitCollection.h"
namespace edm4hep {
using TrackerHit3DCollection = edm4hep::TrackerHitCollection;
using TrackerHit3D = edm4hep::TrackerHit;
}  // namespace edm4hep
#endif
