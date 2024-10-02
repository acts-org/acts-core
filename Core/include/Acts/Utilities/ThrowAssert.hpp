// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once
#include <exception>
#include <iostream>
#include <sstream>
#include <string>

namespace Acts {
/// @brief Exception type for assertion failures
/// This class captures the information available to the throw_assert macro
class AssertionFailureException : public std::exception {
 public:
  /// @brief Class which allows to use the << operator to assemble a string
  class StreamFormatter {
   private:
    std::ostringstream stream;

   public:
    /// @brief Converts to string
    operator std::string() const { return stream.str(); }

    /// @brief Stream operator which takes everything and forwards
    ///        it to the stringstream.
    /// @tparam T type of anything
    /// @param value const ref to anything
    template <typename T>
    StreamFormatter& operator<<(const T& value) {
      stream << value;
      return *this;
    }
  };

  /// @brief Construct an assertion failure exception, captures macro info
  /// @param expression The expression being asserted
  /// @param file The current file
  /// @param line The current line
  /// @param msg The message to print if assertion fails
  AssertionFailureException(const std::string& expression,
                            const std::string& file, int line,
                            const std::string& msg) {
    std::ostringstream os;

    if (!msg.empty()) {
      os << msg << ": ";
    }

    os << "Assertion '" << expression << "'";

    os << " failed in file '" << file << "' line " << line;
    report = os.str();
  }

  /// The assertion message
  const char* what() const throw() override { return report.c_str(); }

 private:
  std::string report;
};

}  // namespace Acts

#define throw_assert(EXPRESSION, MESSAGE)                                   \
  do {                                                                      \
    if (!(EXPRESSION)) {                                                    \
      throw Acts::AssertionFailureException(                                \
          #EXPRESSION, __FILE__, __LINE__,                                  \
          (Acts::AssertionFailureException::StreamFormatter() << MESSAGE)); \
    }                                                                       \
  } while (0)
