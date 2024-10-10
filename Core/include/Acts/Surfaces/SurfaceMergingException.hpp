// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <exception>
#include <memory>
#include <string>

namespace Acts {
class Surface;

/// @brief Exception type failures to merge two surfaces
class SurfaceMergingException : public std::exception {
 public:
  SurfaceMergingException(std::weak_ptr<const Surface> surfaceA,
                          std::weak_ptr<const Surface> surfaceB,
                          const std::string& reason)
      : m_surfaceA(std::move(surfaceA)),
        m_surfaceB(std::move(surfaceB)),
        m_message(std::string{"Failure to merge surfaces: "} + reason) {}

  const char* what() const throw() override { return m_message.c_str(); }

 private:
  std::weak_ptr<const Surface> m_surfaceA;
  std::weak_ptr<const Surface> m_surfaceB;
  std::string m_message;
};
}  // namespace Acts
