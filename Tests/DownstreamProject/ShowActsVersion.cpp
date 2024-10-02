// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <Acts/ActsVersion.hpp>

#include <iostream>

int main(void) {
  std::cout << "Using Acts version " << Acts::VersionMajor << "."
            << Acts::VersionMinor << "." << Acts::VersionPatch << " commit "
            << Acts::CommitHash << std::endl;

  if (Acts::VersionInfo::fromHeader() != Acts::VersionInfo::fromLibrary()) {
    std::cout << "WARNING: The version information is inconsistent!"
              << std::endl;
    std::cout << "Header: " << Acts::VersionInfo::fromHeader() << std::endl;
    std::cout << "Library: " << Acts::VersionInfo::fromLibrary() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
