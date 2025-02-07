// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#pragma once

namespace boost::program_options {
class options_description;
class variables_map;
}  // namespace boost::program_options

namespace ActsExamples::Options {
using Description = ::boost::program_options::options_description;
using Variables = ::boost::program_options::variables_map;
}  // namespace ActsExamples::Options
