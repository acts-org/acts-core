// This file is part of the Acts project.
//
// Copyright (C) 2020-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include <string>

struct CommandLineArguments {
  void parse(int argc, char** argv);
  bool allgroup = false;
  bool onlyGpu = false;
  bool matches = false;
  unsigned int groups = 500;
  bool inpFileExists = false;
  bool dirExists = false;
  bool outFileExists = false;
  std::string deviceName = "";
  std::string inpFileName = "";
  bool csvFormat = false;
};
