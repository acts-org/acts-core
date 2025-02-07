// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
