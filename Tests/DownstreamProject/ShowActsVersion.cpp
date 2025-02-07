// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

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
