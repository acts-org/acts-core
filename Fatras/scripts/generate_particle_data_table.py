#!/usr/bin/env python3
#
# use scikit-hep/particle to generate c++ code for the particle data table.
#

import io
import sys
import subprocess

from particle import Particle


def main(output_file):
    """
    Generate the code and write it to the given output file.
    """
    # extract relevant entries into a single table
    table = []
    for p in Particle.all():
        table.append((int(p.pdgid), int(p.three_charge), p.mass, p.name))
    # use the extracted table to generate the code
    code = generate_code(table)
    code = clang_format(code)
    output_file.write(code)


CODE_HEADER = """\
// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

// The entries within this file have been automatically created using the
// particle data files from the 2019 edition of the Review of Particle Physics
// by the Berkeley Particle Data Group.

#pragma once

#include <cstdint>
#include <limits>

// Rows within the particle data table are sorted by their signed PDG particle
// number and are then stored column-wise. Since the PDG particle number column
// is sorted it can be used to quickly search for the index of a particle
// within all column arrays.

"""


def generate_code(table):
    """
    Generate
    """
    # ensure the rows are sorted by the signed pdg number (first column)
    table = sorted(table, key=lambda _: _[0])
    num_rows = len(table)
    # name, c++ type, and output format for each column
    columns = [
        ("PdgNumber", "int32_t", "{}"),
        ("ThreeCharge", "int16_t", "{}"),
        ("MassMeV", "float", "{}f"),
        ("Name", "char* const   ", '"{}"'),
    ]
    lines = [
        CODE_HEADER,
        f"static constexpr uint32_t kParticlesCount = {num_rows}u;",
    ]
    # build a separate array for each column
    for i, (variable_name, type_name, value_format) in enumerate(columns):
        lines.append(
            f"static const {type_name} kParticles{variable_name}[kParticlesCount] = {{"
        )

        for row in table:
            if i < 3:
                lines.append(f"// {row[-1]}")
            if row[i] is None and type_name == "float":
                lines.append("  std::numeric_limits<float>::quiet_NaN(),")
            else:
                lines.append("  " + value_format.format(row[i]) + ",")

        lines.append("};")
    # ensure we end with a newline
    lines.append("")
    return "\n".join(lines)


def clang_format(content):
    """
    Format the given content using clang-format and return it.
    """
    args = [
        "clang-format",
        "--assume-filename=ParticleData.hpp",
        "-",
    ]
    process = subprocess.run(
        args,
        input=content,
        capture_output=True,
        check=True,
        encoding="utf-8",
        text=True,
    )
    return process.stdout


if __name__ == "__main__":
    if 2 < len(sys.argv):
        print("usage: {} [<output_file>]".format(sys.argv[0]))
        sys.exit(2)
    if len(sys.argv) == 1:
        output_file = sys.stdout
    else:
        # will overwrite existing file
        output_file = io.open(sys.argv[1], mode="wt", encoding="utf-8")
    main(output_file)
