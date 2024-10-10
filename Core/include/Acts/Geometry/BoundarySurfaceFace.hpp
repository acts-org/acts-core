// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <iostream>

namespace Acts {

///  @enum BoundarySurfaceFace
///
///  Enum to describe the position of the BoundarySurface
///  respectively to the frame orientatin of the volume,
///  this is mainly meant for code readability.
///
///  The different numeration sequences can be found in the
///  documentation of the actual VolumeBounds implementations.
///
///  The order of faces is chosen to follow - as much as
///  possible - a circular structure.
enum BoundarySurfaceFace {
  negativeFaceXY = 0,
  positiveFaceXY = 1,
  negativeFaceYZ = 2,
  positiveFaceYZ = 3,
  negativeFaceZX = 4,
  positiveFaceZX = 5,
  cylinderCover = 2,
  tubeInnerCover = 3,
  tubeOuterCover = 2,
  tubeSectorNegativePhi = 4,
  tubeSectorPositivePhi = 5,
  tubeSectorInnerCover = 3,
  tubeSectorOuterCover = 2,
  trapezoidFaceAlpha = 2,
  trapezoidFaceBeta = 3,
  index0 = 0,
  index1 = 1,
  index2 = 2,
  index3 = 3,
  index4 = 4,
  index5 = 5,
  index6 = 6,
  index7 = 7,
  index8 = 8,
  index9 = 9,
  index10 = 10,
  index11 = 11,
  undefinedFace = 99

};

inline std::ostream& operator<<(std::ostream& os, BoundarySurfaceFace& face) {
  os << "BoundarySurfaceFace::";

  switch (face) {
    case negativeFaceXY:
      os << "negativeFaceXY";
      break;
    case positiveFaceXY:
      os << "positiveFaceXY";
      break;
    case negativeFaceYZ:
      os << "negativeFaceYZ|cylinderCover|tubeOuterCover|tubeSectorOuterCover|"
            "trapezoidFaceAlpha";
      break;
    case positiveFaceYZ:
      os << "positiveFaceYZ|tubeInnerCover|tubeSectorInnerCover|"
            "trapezoidFaceBeta";
      break;
    case negativeFaceZX:
      os << "negativeFaceZX|tubeSectorNegativePhi";
      break;
    case positiveFaceZX:
      os << "positiveFaceZX|tubeSectorPositivePhi";
      break;
    case undefinedFace:
      os << "undefinedFace";
      break;
    default:
      os << face;
  }

  return os;
}
}  // namespace Acts
